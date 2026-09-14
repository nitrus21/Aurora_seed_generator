param([ValidateSet('waveshare-p4', 'waveshare-p4-rev1')][string] $Target = 'waveshare-p4')
$ErrorActionPreference = 'Stop'
# Get the existing 8.3 alias of THIS folder. No copy, junction, or other workspace.
$auroraFolder = (New-Object -ComObject Scripting.FileSystemObject).GetFolder($PSScriptRoot)
$auroraBuildPath = $auroraFolder.ShortPath
if ($auroraBuildPath.Contains(' ')) {
    throw 'ESP-IDF requires a path without spaces; Windows has no usable short alias for this folder. No files were moved. Enable an appropriate short name before building.'
}
Write-Host "Compilation dans le même dossier : $PSScriptRoot"
Write-Host "Alias Windows utilisé par ESP-IDF : $auroraBuildPath"
# Both silicon profiles share managed dependencies. Do not let two component
# manager processes replace those dependencies while the other is compiling.
[void][System.IO.Directory]::CreateDirectory((Join-Path $PSScriptRoot '.pio'))
try {
    $auroraBuildLock = [System.IO.File]::Open((Join-Path $PSScriptRoot '.pio/aurora-build.lock'),
        [System.IO.FileMode]::OpenOrCreate, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
} catch [System.IO.IOException] {
    throw 'Une compilation P4 est déjà en cours. Attendez sa fin avant de lancer un autre profil.'
}
try {
    & pio run --project-dir $auroraBuildPath --environment $Target
    $auroraBuildResult = $LASTEXITCODE
} finally {
    $auroraBuildLock.Dispose()
}
exit $auroraBuildResult
