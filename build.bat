@echo off
REM Build script for Win32 RegDemo using MSVC
REM Run from a Visual Studio Developer Command Prompt
REM
REM Usage: build.bat
REM   or for MinGW: make -f Makefile TOOLCHAIN=mingw

cl /nologo /W3 /O2 /D_WIN32_WINNT=0x0400 /DWIN32 /D_WINDOWS /c regdemo.c
if errorlevel 1 goto :fail

if exist regdemo.ico (
    rc /nologo /fo regdemo.res regdemo.rc
    if errorlevel 1 goto :nores
    link /nologo /subsystem:windows /out:regdemo.exe regdemo.obj regdemo.res advapi32.lib user32.lib gdi32.lib kernel32.lib
) else (
    echo Note: regdemo.ico not found, building without icon resource
    link /nologo /subsystem:windows /out:regdemo.exe regdemo.obj advapi32.lib user32.lib gdi32.lib kernel32.lib
)
if errorlevel 1 goto :fail

echo.
echo Build successful: regdemo.exe
goto :end

:nores
echo Warning: resource compilation failed, building without icon
link /nologo /subsystem:windows /out:regdemo.exe regdemo.obj advapi32.lib user32.lib gdi32.lib kernel32.lib
if errorlevel 1 goto :fail
echo.
echo Build successful: regdemo.exe (no icon)
goto :end

:fail
echo.
echo BUILD FAILED
exit /b 1

:end
