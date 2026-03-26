@echo off
setlocal

set SRC=regdemo_win32_nt4.c
set EXE=regdemo_win32_nt4.exe

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /W3 /O2 /DWIN32 /D_WINDOWS %SRC% /link advapi32.lib user32.lib gdi32.lib /OUT:%EXE%
    exit /b %errorlevel%
)

where gcc >nul 2>nul
if %errorlevel%==0 (
    gcc -O2 -Wall -Wextra -mwindows %SRC% -ladvapi32 -luser32 -lgdi32 -o %EXE%
    exit /b %errorlevel%
)

echo No supported Windows compiler found.
echo Use Visual C++ (cl.exe) or MinGW gcc.
exit /b 1
