@echo off
setlocal

cd /d "%~dp0.."

set "TCC=C:\tcc\tcc.exe"

if not exist build mkdir build

for %%F in (
    source\capture-detector.c
    source\_test_protected.c
) do (
    "%TCC%" ^
        -DPROCESS_QUERY_LIMITED_INFORMATION=0x1000 ^
        -DWDA_MONITOR=0x00000001 ^
        -DWDA_EXCLUDEFROMCAPTURE=0x00000011 ^
        -DENABLE_VIRTUAL_TERMINAL_PROCESSING=0x0004 ^
        "%%F" ^
        -o "build\%%~nF.exe" ^
        -luser32 ^
        -lkernel32
)

pause