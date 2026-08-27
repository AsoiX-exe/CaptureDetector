@echo off
setlocal

cd /d "%~dp0.."

where zig.exe >nul 2>&1

if errorlevel 1 (
    echo.
    echo ERROR: zig.exe not found.
    echo.
    pause
    exit /b 1
)

echo Using Zig:
zig version
echo.

if not exist build mkdir build

set "FLAGS=-O3 -target x86_64-windows-gnu -static"

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
    echo.
    echo Compiling %%F...

    zig cc %FLAGS% "%%F" ^
        -o "build\%%~nF.exe" ^
        -luser32 ^
        -lkernel32

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