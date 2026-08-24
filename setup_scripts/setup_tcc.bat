@echo off
setlocal EnableExtensions

title TCC 0.9.27 Setup

set "TCC_DIR=C:\tcc"
set "TCC_EXE=%TCC_DIR%\tcc.exe"
set "TMP=%TEMP%\tcc_setup"

echo ==========================================
echo        TCC 0.9.27 Windows Setup
echo ==========================================
echo.

:: --------------------------------------------------
:: 1. Check for administrator privileges
:: --------------------------------------------------
net session >nul 2>&1
if not "%errorlevel%"=="0" (
    echo [ERROR] Please run setup_tcc.bat as Administrator.
    pause
    exit /b 1
)

:: --------------------------------------------------
:: 2. Create required directories
:: --------------------------------------------------
if not exist "%TCC_DIR%" mkdir "%TCC_DIR%"
if not exist "%TMP%" mkdir "%TMP%"

:: --------------------------------------------------
:: 3. Download TCC
:: --------------------------------------------------
echo [1/5] Downloading TCC...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$ProgressPreference='SilentlyContinue';" ^
    "Invoke-WebRequest 'https://download-mirror.savannah.gnu.org/releases/tinycc/tcc-0.9.27-win64-bin.zip' -OutFile '%TMP%\tcc.zip'"

if not exist "%TMP%\tcc.zip" (
    echo [ERROR] Failed to download TCC.
    pause
    exit /b 1
)

:: --------------------------------------------------
:: 4. Extract and install TCC
:: --------------------------------------------------
echo [2/5] Installing TCC...

powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "Expand-Archive -Force '%TMP%\tcc.zip' '%TMP%\tcc_extract'"

:: Find tcc.exe inside the extracted archive
for /r "%TMP%\tcc_extract" %%F in (tcc.exe) do (
    if not defined FOUND_TCC set "FOUND_TCC=%%~dpF"
)

if not defined FOUND_TCC (
    echo [ERROR] tcc.exe was not found in the archive.
    pause
    exit /b 1
)

xcopy /E /I /Y "%FOUND_TCC%*" "%TCC_DIR%\" >nul

if not exist "%TCC_EXE%" (
    echo [ERROR] TCC installation failed.
    pause
    exit /b 1
)

:: --------------------------------------------------
:: 5. Generate Windows API import definitions
:: --------------------------------------------------
echo [3/5] Generating Windows API import definitions...

cd /d "%TCC_DIR%"

"%TCC_EXE%" -impdef "%SystemRoot%\System32\kernel32.dll"
if errorlevel 1 (
    echo [WARNING] Failed to generate kernel32.def.
)

"%TCC_EXE%" -impdef "%SystemRoot%\System32\user32.dll"
if errorlevel 1 (
    echo [WARNING] Failed to generate user32.def.
)

if exist "%TCC_DIR%\kernel32.def" (
    copy /Y "%TCC_DIR%\kernel32.def" "%TCC_DIR%\lib\kernel32.def" >nul
)

if exist "%TCC_DIR%\user32.def" (
    copy /Y "%TCC_DIR%\user32.def" "%TCC_DIR%\lib\user32.def" >nul
)

:: --------------------------------------------------
:: 6. Add TCC to PATH
:: --------------------------------------------------
echo [4/5] Updating PATH...

setx PATH "%PATH%;%TCC_DIR%" >nul 2>&1

:: --------------------------------------------------
:: 7. Test installation
:: --------------------------------------------------
echo [5/5] Testing installation...
echo.

"%TCC_EXE%" -v

if errorlevel 1 (
    echo.
    echo [ERROR] TCC test failed.
    pause
    exit /b 1
)

echo.
echo ==========================================
echo       TCC installed successfully!
echo ==========================================
echo.
echo TCC: %TCC_EXE%
echo.
echo Windows API import definitions have been
echo generated from the installed System32 DLLs.
echo.
echo You can now run your build_tcc.bat.
echo.

rmdir /S /Q "%TMP%" >nul 2>&1

pause