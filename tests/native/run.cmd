@echo off
setlocal
pushd "%~dp0..\.."
if not exist "tmp\entropy-tests" mkdir "tmp\entropy-tests"
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "AURORA_VS_PATH=%%i"
if not defined AURORA_VS_PATH exit /b 1
call "%AURORA_VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /Itests\native\stubs /Iinclude tests\native\test_entropy.cpp src\hardware_rng.cpp /Fotmp\entropy-tests\ /Fetmp\entropy-tests\test_entropy.exe /link bcrypt.lib
if errorlevel 1 exit /b 1
tmp\entropy-tests\test_entropy.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /DAURORA_BOARD_P4 /DAURORA_NATIVE_TEST /Itests\native\stubs /Iinclude tests\native\test_entropy.cpp src\hardware_rng.cpp /Fotmp\entropy-tests\ /Fetmp\entropy-tests\test_entropy_p4.exe /link bcrypt.lib
if errorlevel 1 exit /b 1
tmp\entropy-tests\test_entropy_p4.exe
set "AURORA_TEST_RESULT=%ERRORLEVEL%"
popd
exit /b %AURORA_TEST_RESULT%
