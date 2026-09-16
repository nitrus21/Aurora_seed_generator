param([switch] $ReleasedArtifactsOnly)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-Release([bool] $condition, [string] $message) {
    if (-not $condition) { throw $message }
}

$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$webRoot = Join-Path $taskRoot 'webflasher'
$firmwareRoot = Join-Path $webRoot 'firmware'
$versionHeader = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'include\version.h')
$versionMatch = [regex]::Match($versionHeader, 'AURORA_CYD_FIRMWARE_VERSION\s+"(\d+\.\d+\.\d+)"')
$manifest = Get-Content -Raw -LiteralPath (Join-Path $webRoot 'manifest.json') | ConvertFrom-Json
if ($ReleasedArtifactsOnly) {
    $version = $manifest.version
} else {
    Assert-Release $versionMatch.Success 'A final CYD firmware version is required. Use -ReleasedArtifactsOnly to verify preserved release binaries on a development branch.'
    $version = $versionMatch.Groups[1].Value
    $p4VersionMatch = [regex]::Match($versionHeader, 'AURORA_P4_FIRMWARE_VERSION\s+"(\d+\.\d+\.\d+)"')
    Assert-Release ($p4VersionMatch.Success -and $p4VersionMatch.Groups[1].Value -eq '2.0.3') 'P4 source/release version mismatch.'
}
$layout = Get-Content -Raw -LiteralPath (Join-Path $firmwareRoot 'flash-layout.json') | ConvertFrom-Json
$factoryName = "aurora-$version-esp32-2432s028r.factory.bin"

Assert-Release ($manifest.version -eq $version) 'Manifest/source version mismatch.'
Assert-Release ($manifest.builds.Count -eq 1) 'Expected one ESP32 build.'
$build = $manifest.builds[0]
Assert-Release ($build.chipFamily -eq 'ESP32' -and $build.parts.Count -eq 1) 'Unexpected build layout.'
Assert-Release ($build.parts[0].path -eq "firmware/$factoryName" -and $build.parts[0].offset -eq 0) 'Incorrect installer image.'
Assert-Release ($layout.webFlasherImage.file -eq $factoryName -and $layout.webFlasherImage.offsetDecimal -eq 0) 'Incorrect factory layout.'
Assert-Release ($layout.flashMode -eq 'dio' -and $layout.flashFrequency -eq '40m' -and $layout.flashSize -eq '4MB') 'Unexpected flash settings.'

$factoryPath = Join-Path $firmwareRoot $factoryName
$factory = [System.IO.File]::ReadAllBytes($factoryPath)
$expectedOffsets = @{ 'bootloader.bin' = 0x1000; 'partitions.bin' = 0x8000; 'boot_app0.bin' = 0xE000; 'firmware.bin' = 0x10000 }
Assert-Release ($layout.parts.Count -eq $expectedOffsets.Count) 'Unexpected part count.'
$seenParts = @{}
$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    foreach ($part in $layout.parts) {
        Assert-Release ($expectedOffsets.ContainsKey($part.file) -and -not $seenParts.ContainsKey($part.file)) 'Unknown or duplicate flash part.'
        $seenParts[$part.file] = $true
        $offset = [int] $part.offsetDecimal
        Assert-Release ($offset -eq $expectedOffsets[$part.file] -and $offset -eq [Convert]::ToInt32($part.offset, 16)) 'Incorrect part offset.'
        $partPath = Join-Path $firmwareRoot $part.file
        $length = [int] (Get-Item -LiteralPath $partPath).Length
        Assert-Release ($offset + $length -le $factory.Length) 'Truncated factory image.'
        $sliceHash = [BitConverter]::ToString($sha.ComputeHash($factory, $offset, $length)).Replace('-', '')
        Assert-Release ($sliceHash -eq (Get-FileHash -Algorithm SHA256 -LiteralPath $partPath).Hash) "Factory part mismatch: $($part.file)"
    }
} finally { $sha.Dispose() }

$checksums = @{}
foreach ($line in (Get-Content -LiteralPath (Join-Path $firmwareRoot 'SHA256SUMS.txt'))) {
    $entry = [regex]::Match($line, '^([A-Fa-f0-9]{64})  ([^\\/]+\.bin)$')
    Assert-Release $entry.Success 'Invalid checksum entry.'
    $name = $entry.Groups[2].Value
    Assert-Release (-not $checksums.ContainsKey($name)) 'Duplicate checksum entry.'
    $checksums[$name] = $entry.Groups[1].Value
    $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $firmwareRoot $name)).Hash
    Assert-Release ($actualHash -eq $checksums[$name]) "Checksum mismatch: $name"
}
foreach ($binary in (Get-ChildItem -LiteralPath $firmwareRoot -Filter '*.bin' -File)) {
    Assert-Release ($checksums.ContainsKey($binary.Name)) "Missing checksum: $($binary.Name)"
}

$factoryHash = $checksums[$factoryName]
$appJs = Get-Content -Raw -LiteralPath (Join-Path $webRoot 'assets\app.js')
$index = Get-Content -Raw -LiteralPath (Join-Path $webRoot 'index.html')
$readme = Get-Content -Raw -LiteralPath (Join-Path $taskRoot 'README.md')
Assert-Release ($appJs.Contains($factoryHash) -and $index.Contains($factoryHash)) 'Website factory hash mismatch.'
Assert-Release ($index.Contains("Installer AURORA ESP32-2432S028R v$version")) 'Website version mismatch.'
Assert-Release ($readme.Contains('webflasher/README.md')) 'README must link to installation and checksum guidance.'
Assert-Release (Test-Path -LiteralPath (Join-Path $webRoot 'CHANGELOG.md')) 'Missing release notes.'

$variants = @(
    @{ Id = 'cyd-1.9.7'; Manifest = 'manifest.json'; Chip = 'ESP32'; Image = 'aurora-1.9.7-esp32-2432s028r.factory.bin' },
    @{ Id = 'cyd-1.7.5'; Manifest = 'manifests\cyd-1.7.5.json'; Chip = 'ESP32'; Image = 'aurora-1.7.5-esp32-2432s028r.factory.bin' },
    @{ Id = 'p4-rev1-2.0.3'; Manifest = 'manifests\p4-rev1-2.0.3.json'; Chip = 'ESP32-P4'; Image = 'aurora-2.0.3-esp32-p4-rev1.factory.bin' },
    @{ Id = 'p4-rev3-2.0.3'; Manifest = 'manifests\p4-rev3-2.0.3.json'; Chip = 'ESP32-P4'; Image = 'aurora-2.0.3-esp32-p4-rev3.factory.bin' }
)
foreach ($variant in $variants) {
    $variantManifest = Get-Content -Raw -LiteralPath (Join-Path $webRoot $variant.Manifest) | ConvertFrom-Json
    Assert-Release ($variantManifest.builds.Count -eq 1) "Unexpected build count: $($variant.Id)"
    $variantBuild = $variantManifest.builds[0]
    Assert-Release ($variantBuild.chipFamily -eq $variant.Chip -and $variantBuild.parts.Count -eq 1) "Incorrect chip family: $($variant.Id)"
    Assert-Release ($variantBuild.parts[0].offset -eq 0 -and $variantBuild.parts[0].path.EndsWith("firmware/$($variant.Image)")) "Incorrect image path: $($variant.Id)"
    Assert-Release ($checksums.ContainsKey($variant.Image)) "Missing variant checksum: $($variant.Id)"
    Assert-Release ($appJs.Contains($checksums[$variant.Image]) -and $index.Contains($variant.Id)) "Website variant missing: $($variant.Id)"
}

& python (Join-Path $PSScriptRoot 'verify_images.py')
Assert-Release ($LASTEXITCODE -eq 0) 'Embedded image/revision verification failed.'
& node (Join-Path $PSScriptRoot 'test_webflasher.cjs')
Assert-Release ($LASTEXITCODE -eq 0) 'Web Flasher selection tests failed.'
Write-Output "PASS: CYD 1.9.7/1.7.5 and P4 2.0.3 (1.x/3.x): manifests, images, offsets, hashes and selection"
