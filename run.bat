@echo off
REM Double-click this to run capscan and keep the window open.
chcp 65001 >nul
"%~dp0capscan.exe" %*
echo.
pause
