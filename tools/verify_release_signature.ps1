[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Manifest,

    [string]$Signature = "$Manifest.sig",

    [string]$AllowedSigners
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($AllowedSigners)) {
    $AllowedSigners = Join-Path $PSScriptRoot "..\keys\aurora-release-allowed_signers"
}
$sshKeygen = (Get-Command ssh-keygen.exe -ErrorAction Stop).Source
$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$signaturePath = (Resolve-Path -LiteralPath $Signature).Path
$allowedSignersPath = (Resolve-Path -LiteralPath $AllowedSigners).Path

$arguments = @(
    "-Y", "verify",
    "-f", ('"' + $allowedSignersPath + '"'),
    "-I", "aurora-release",
    "-n", "aurora-release",
    "-s", ('"' + $signaturePath + '"')
)

# RedirectStandardInput passes the manifest bytes unchanged. A PowerShell native
# pipe would decode and re-encode them, which can invalidate a correct signature.
$process = Start-Process -FilePath $sshKeygen -ArgumentList $arguments -NoNewWindow -Wait -PassThru -RedirectStandardInput $manifestPath
if ($process.ExitCode -ne 0) {
    throw "Signature AURORA invalide (code $($process.ExitCode))."
}

Write-Host "Signature AURORA valide : $manifestPath" -ForegroundColor Green
