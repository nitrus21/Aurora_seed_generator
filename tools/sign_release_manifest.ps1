[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Manifest,

    [Parameter(Mandatory = $true)]
    [string]$KeyPath,

    [switch]$Force
)

$ErrorActionPreference = "Stop"
$sshKeygen = (Get-Command ssh-keygen.exe -ErrorAction Stop).Source
$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$key = (Resolve-Path -LiteralPath $KeyPath).Path
$signaturePath = "$manifestPath.sig"

if ((Test-Path -LiteralPath $signaturePath) -and -not $Force) {
    throw "La signature existe déjà : $signaturePath (utilisez -Force pour la remplacer)."
}
if ($Force -and (Test-Path -LiteralPath $signaturePath)) {
    Remove-Item -LiteralPath $signaturePath -Force
}

& $sshKeygen -Y sign -f $key -n aurora-release $manifestPath
if ($LASTEXITCODE -ne 0) {
    throw "La signature du manifeste a échoué (code $LASTEXITCODE)."
}

& (Join-Path $PSScriptRoot "verify_release_signature.ps1") -Manifest $manifestPath
if ($LASTEXITCODE -ne 0) {
    throw "La vérification immédiate de la signature a échoué."
}

Write-Host "Signature créée et vérifiée : $signaturePath" -ForegroundColor Green
