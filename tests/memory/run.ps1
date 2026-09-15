param([switch]$Optimized)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskOutput = Join-Path $taskRoot 'tmp/memory-native'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$taskCompilerEnvironment = & cmd /d /c ('call "{0}" >nul && set' -f $vcvars)
foreach ($entry in $taskCompilerEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
$taskOptimization = if ($Optimized) { '/O2' } else { '/Od' }
$taskArgs = @('/nologo', '/utf-8', '/std:c11', $taskOptimization, '/UNDEBUG', '/DAURORA_BOARD_P4',
    '/DESP_PLATFORM', '/DAURORA_MEMORY_TEST', '/D_CRT_SECURE_NO_WARNINGS',
    ('/I"{0}\stubs"' -f $PSScriptRoot), ('/I"{0}\include"' -f $taskRoot),
    ('/Fo"{0}\test_memory.obj"' -f $taskOutput), ('/Fe"{0}\test_memory.exe"' -f $taskOutput),
    ('"{0}\test_memory.c"' -f $PSScriptRoot))
[IO.File]::WriteAllLines((Join-Path $taskOutput 'build.rsp'), $taskArgs)
& cl.exe ('@' + (Join-Path $taskOutput 'build.rsp'))
if ($LASTEXITCODE -ne 0) { throw 'Memory test compilation failed.' }
& (Join-Path $taskOutput 'test_memory.exe')
if ($LASTEXITCODE -ne 0) { throw 'Memory test failed.' }
