$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskOutput = Join-Path $taskRoot 'tmp/lvgl-oom-native'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskResponsePath = Join-Path $taskRoot 'tmp/ui-p4-native/link-ui.rsp'
if (-not (Test-Path -LiteralPath $taskResponsePath)) { throw 'Run tests/ui/run.py first.' }
# Match the UI test/P4 build source lock; managed dependencies cannot change mid-test.
$taskLock = [IO.File]::Open((Join-Path $taskRoot 'targets/waveshare_p4/.pio/aurora-build.lock'),
    [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
try {
    & python (Join-Path $taskRoot 'tools/patch_lvgl.py') --lvgl-root (Join-Path $taskRoot 'targets/waveshare_p4/managed_components/lvgl__lvgl')
    if ($LASTEXITCODE -ne 0) { throw 'LVGL hardening verification failed.' }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath).Trim()
    $vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    $taskCompilerEnvironment = & cmd /d /c ('call "{0}" >nul && set' -f $vcvars)
    foreach ($entry in $taskCompilerEnvironment) {
        if ($entry -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
        }
    }
    $taskLines = Get-Content -LiteralPath $taskResponsePath
    $taskCommon = @($taskLines | Where-Object { $_ -match '^/(I|D|utf-8|nologo)' })
    if ($taskCommon -notcontains '/DAURORA_BOARD_P4') { $taskCommon += '/DAURORA_BOARD_P4' }
    $taskCArgs = $taskCommon + @('/c', '/std:c11', '/O1', '/w',
        ('/Fo"{0}\textarea_fault.obj"' -f $taskOutput), ('"{0}\textarea_fault.c"' -f $PSScriptRoot))
    [IO.File]::WriteAllLines((Join-Path $taskOutput 'fault.rsp'), $taskCArgs)
    & cl.exe ('@' + (Join-Path $taskOutput 'fault.rsp'))
    if ($LASTEXITCODE -ne 0) { throw 'Fault module compilation failed.' }
    # Build the current production allocator too; never rely on a stale object.
    $taskCArgs = $taskCommon + @('/c', '/std:c11', '/O1', '/w',
        ('/Fo"{0}\secure_lvgl_memory.obj"' -f $taskOutput), ('"{0}\src\secure_lvgl_memory.c"' -f $taskRoot))
    [IO.File]::WriteAllLines((Join-Path $taskOutput 'allocator.rsp'), $taskCArgs)
    & cl.exe ('@' + (Join-Path $taskOutput 'allocator.rsp'))
    if ($LASTEXITCODE -ne 0) { throw 'Allocator compilation failed.' }
    $taskObjects = @($taskLines | Where-Object { $_ -match '\.obj"$' })
    $taskObjects = $taskObjects | ForEach-Object {
        $_.Replace((Join-Path $taskRoot 'tmp/ui-p4-native/objects/lv_textarea.obj'), (Join-Path $taskOutput 'textarea_fault.obj')).
           Replace((Join-Path $taskRoot 'tmp/ui-p4-native/objects/secure_lvgl_memory.obj'), (Join-Path $taskOutput 'secure_lvgl_memory.obj'))
    }
    $taskLinkArgs = $taskCommon + @('/std:c++20', '/EHsc', '/UNDEBUG',
        ('/Fo"{0}\test_oom.obj"' -f $taskOutput), ('/Fe"{0}\test_oom.exe"' -f $taskOutput),
        ('"{0}\test_oom.cpp"' -f $PSScriptRoot)) + $taskObjects + @('bcrypt.lib')
    [IO.File]::WriteAllLines((Join-Path $taskOutput 'link.rsp'), $taskLinkArgs)
    & cl.exe ('@' + (Join-Path $taskOutput 'link.rsp'))
    if ($LASTEXITCODE -ne 0) { throw 'OOM test compilation failed.' }
    & (Join-Path $taskOutput 'test_oom.exe')
    if ($LASTEXITCODE -ne 0) { throw 'OOM test failed.' }
} finally { $taskLock.Dispose() }
