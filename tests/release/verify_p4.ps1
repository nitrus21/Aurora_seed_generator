$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-P4([bool] $condition, [string] $message) {
    if (-not $condition) { throw $message }
}

$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$web = Join-Path $root 'webflasher'
$versionHeader = Get-Content -Raw -LiteralPath (Join-Path $root 'include\version.h')
$versionMatch = [regex]::Match($versionHeader, 'AURORA_P4_FIRMWARE_VERSION\s+"(\d+\.\d+\.\d+)"')
Assert-P4 ($versionMatch.Success -and $versionMatch.Groups[1].Value -eq '2.0.12') 'P4 source must match the newest release candidate image.'

$app = Get-Content -Raw -LiteralPath (Join-Path $web 'assets\app.js')
$index = Get-Content -Raw -LiteralPath (Join-Path $web 'index.html')
$sums = Get-Content -Raw -LiteralPath (Join-Path $web 'firmware\SHA256SUMS.txt')
$variants = @(
    @{ Profile='waveshare-p4-rev1'; Id='p4-rev1-2.0.12'; Manifest='manifests\p4-rev1-2.0.12.json'; Image='aurora-2.0.12-esp32-p4-rev1.factory.bin' },
    @{ Profile='waveshare-p4'; Id='p4-rev3-2.0.12'; Manifest='manifests\p4-rev3-2.0.12.json'; Image='aurora-2.0.12-esp32-p4-rev3.factory.bin' }
)
foreach ($variant in $variants) {
    $manifest = Get-Content -Raw -LiteralPath (Join-Path $web $variant.Manifest) | ConvertFrom-Json
    Assert-P4 ($manifest.version -eq '2.0.12' -and $manifest.builds.Count -eq 1) "Manifest invalide: $($variant.Id)"
    Assert-P4 ($manifest.builds[0].chipFamily -eq 'ESP32-P4' -and $manifest.builds[0].parts[0].offset -eq 0) "Cible invalide: $($variant.Id)"
    Assert-P4 ($manifest.builds[0].parts[0].path.EndsWith("firmware/$($variant.Image)")) "Image invalide: $($variant.Id)"
    $image = Join-Path $web "firmware\$($variant.Image)"
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $image).Hash
    Assert-P4 ($sums.Contains("$hash  $($variant.Image)")) "SHA256SUMS absent: $($variant.Image)"
    Assert-P4 ($app.Contains($hash) -and $app.Contains($variant.Id) -and $index.Contains($variant.Id)) "Web Flasher incomplet: $($variant.Id)"
    & python (Join-Path $PSScriptRoot '..\crypto\verify_p4_images.py') --profile $variant.Profile --factory $image --version 2.0.12
    Assert-P4 ($LASTEXITCODE -eq 0) "Image P4 invalide: $($variant.Profile)"
}
Assert-P4 (Test-Path -LiteralPath (Join-Path $web 'releases\2.0.12.md')) 'Notes 2.0.12 absentes.'
$oldMode = [Environment]::GetEnvironmentVariable('AURORA_P4_ONLY', 'Process')
try {
    $env:AURORA_P4_ONLY = '1'
    & node (Join-Path $PSScriptRoot 'test_webflasher.cjs')
    Assert-P4 ($LASTEXITCODE -eq 0) 'Sélection Web Flasher P4 invalide.'
} finally {
    [Environment]::SetEnvironmentVariable('AURORA_P4_ONLY', $oldMode, 'Process')
}
& node (Join-Path $PSScriptRoot 'test_vendor.cjs')
Assert-P4 ($LASTEXITCODE -eq 0) 'Dépendance Web Flasher invalide.'
Write-Output 'PASS: P4 2.0.12 rev1/rev3, manifests, images, SHA-256, notes and Web Flasher'
