@echo off
setlocal

cd /d "%~dp0.."

where clang-cl.exe >nul 2>&1

if errorlevel 1 (
    echo clang-cl.exe not found.
    echo.
    echo Install LLVM/Clang or add the LLVM-bin Folder to PATH
    echo.
    pause
    exit /b 1
)

echo Using:
clang-cl.exe --version
echo.

if not exist build mkdir build

set "FLAGS=/nologo /MT /O2"

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
    echo.
    echo Compiling %%F...

    clang-cl.exe "%%F" %FLAGS% ^
        /Fo"build\%%~nF.obj" ^
        /Fe"build\%%~nF.exe" ^
        /link user32.lib kernel32.lib

    if errorlevel 1 (
        echo.
        echo Build failed: %%F
        pause
        exit /b 1
    )
)

echo.
echo ========================================
echo Build successful.
echo ========================================
echo.

pause
