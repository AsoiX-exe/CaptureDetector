@echo off
setlocal

cd /d "%~dp0.."

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq delims=" %%V in (`
    "%VSWHERE%" -latest -products * ^
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
    -property installationPath
`) do (
    call "%%V\VC\Auxiliary\Build\vcvars64.bat"
)

if not exist build mkdir build

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
    echo Compiling %%F...

    cl "%%F" %FLAGS% /MT /O2 ^
        /Fo:"build\%%~nF.obj" ^
        /Fe:"build\%%~nF.exe" ^
        /link user32.lib kernel32.lib

    if errorlevel 1 (
        echo.
        echo Build failed: %%F
        pause
        exit /b 1
    )
)

echo.
echo Build successful.
pause