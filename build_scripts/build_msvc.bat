@echo off

cd /d "%~dp0.."

for /f "usebackq delims=" %%V in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    call "%%V\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist build mkdir build

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
    cl "%%F" %FLAGS% /MT /O2 /Fo:"build\%%~nF.obj" /Fe:"build\%%~nF.exe" /link user32.lib
)

pause