@echo off
setlocal

cd /d "%~dp0.."

if not exist build mkdir build

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
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