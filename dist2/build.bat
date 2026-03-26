@echo off
setlocal

set SRC=regdemo_win32.c
set EXE=regdemo_win32.exe

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /W4 /O2 /DUNICODE /D_UNICODE %SRC% /link advapi32.lib gdi32.lib user32.lib /OUT:%EXE%
    exit /b %errorlevel%
)

where gcc >nul 2>nul
if %errorlevel%==0 (
    gcc -municode -O2 -Wall -Wextra -DUNICODE -D_UNICODE %SRC% -ladvapi32 -lgdi32 -luser32 -o %EXE%
    exit /b %errorlevel%
)

echo No supported Windows C compiler was found in PATH.
echo Use either MSVC cl.exe or MinGW gcc on a Windows machine.
exit /b 1