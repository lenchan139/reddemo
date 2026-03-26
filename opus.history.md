# Chat History — RegDemo VB4 to Win32 C Port

## User

Examine this old source code, we need to port it to modern Win32 with
NT4 compatibility at a minimum converted/rewrote C version should
be functional and visual equivalent to original VB version.

## Assistant

[Examined all VB source files: REGDEMO.VBP, REGMODUL.BAS, REGDEMO.FRM, FORM2.FRM, regdemo_check]

Now I have a complete understanding of the VB4 application. Let me create the Win32 C port.

The application is a registry editor with:
- **Form1**: Main window with two listboxes (keys left, values right), labels, and an Exit button
- **Form2**: Edit dialog for REG_SZ and REG_DWORD values with hex/decimal radio buttons
- **REGMODUL.BAS**: Registry API wrappers (enumerate keys, enumerate values, get/set values)

### Files created:
- **regdemo.h** — Header with resource IDs, constants, globals, and prototypes
- **regdemo.c** — Complete Win32 C port (~1380 lines)
- **regdemo.rc** — Resource file (icon only)
- **build.bat** — MSVC build script
- **Makefile** — Supports both MSVC (`nmake`) and MinGW (`make TOOLCHAIN=mingw`)

### Architecture of the port:

| VB4 Source | C Equivalent |
|---|---|
| `REGDEMO.FRM` (Form1) | `MainWndProc`, `MainOnCreate`, `MainOnSize`, `MainOnList1Click/DblClick` |
| `FORM2.FRM` (Form2 edit dialog) | `EditDlgProc` + in-memory `DLGTEMPLATE` built by `BuildEditDialog()` |
| `REGMODUL.BAS` (registry functions) | `RegEnumKeysToList`, `RegEnumValuesToList`, `EditRegValue`, `RegSetValueByType`, `GetMainKey`, `RtnRegError` |

### Key design decisions:
- **NT4 compatible**: Uses ANSI Win32 APIs only (`*A` suffixes), no common controls v6, no `DLGTEMPLATE_EX`
- **Edit dialog built in memory** via `DLGTEMPLATE` — avoids needing the dialog defined in `.rc` (which is cleaner for deployment and matches the VB approach of a code-defined form)
- **Flat appearance** with `COLOR_BTNFACE` background, Courier New for listboxes, MS Sans Serif Bold for labels — matching the VB original's flat Win95-era look
- **All VB behaviors preserved**: tree expand/collapse via double-click with `+`/`*` indicators, value display formatting (REG_SZ quoted, REG_DWORD as `&Hxx (decimal)`, REG_BINARY as hex bytes), hex/decimal radio toggle in DWORD editor, horizontal scrollbars, resize handling, hourglass cursor during enumeration
- **No icon file included** — you'll need to provide `regdemo.ico` or the build will skip the resource. The build script handles this gracefully

To build: run `build.bat` from a Visual Studio Developer Command Prompt, or `make -f Makefile TOOLCHAIN=mingw` for MinGW.

---

### Source Files Created

#### regdemo.h

```c
/*
 * regdemo.h - Win32 Registry Demo
 *
 * Port of VB4 RegDemo by Don Bradner (1995) to Win32 C.
 * NT4 compatible. No common controls v6 dependency.
 */

#ifndef REGDEMO_H
#define REGDEMO_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Resource IDs ---- */
#define IDI_REGDEMO         100

/* Main form controls */
#define IDC_LIST1           1001
#define IDC_LIST2           1002
#define IDC_LABEL1          1003
#define IDC_LABEL2          1004
#define IDC_LABEL3          1005
#define IDC_LABEL4          1006
#define IDC_LABEL5_0        1007
#define IDC_LABEL5_1        1008
#define IDC_EXIT_BTN        1009

/* Edit dialog (Form2) */
#define IDD_EDITVALUE       2000
#define IDC_EDIT_VALNAME    2001
#define IDC_EDIT_VALDATA    2002
#define IDC_EDIT_OK         2003
#define IDC_EDIT_CANCEL     2004
#define IDC_FRAME_BASE      2005
#define IDC_RADIO_HEX       2006
#define IDC_RADIO_DEC       2007
/* Hidden storage text boxes */
#define IDC_EDIT_HKEY       2008
#define IDC_EDIT_SUBKEY     2009

/* ---- Constants ---- */
#define LIST_INDENT         2
#define MAX_VALUE_DISPLAY   20000

/* ---- Windows version detection ---- */
#define WINVER_NT    3
#define WINVER_9X    2

/* ---- Global state ---- */
extern int      g_iWinVers;
extern HCURSOR  g_hWaitCursor;
extern DWORD    g_lCurrentType;
extern int      g_iList2Tab;
extern HFONT    g_hFont;
extern HFONT    g_hBoldFont;
extern HWND     g_hList1;
extern HWND     g_hList2;
extern HWND     g_hMainWnd;
extern HINSTANCE g_hInst;

/* ---- Function prototypes ---- */

/* Registry operations */
LONG RegEnumKeysToList(HKEY hMainKey, LPCSTR szSubKey, int iLevels, int iStartList);
LONG RegEnumValuesToList(HKEY hMainKey, LPCSTR szSubKey);
void EditRegValue(HKEY hMainKey, LPCSTR szSubKey, int iRegIndex);
LONG RegSetValueByType(HKEY hMainKey, LPCSTR szSubKey, LPCSTR szValueName, LPCSTR szValue);
LPCSTR RtnRegError(LONG errorcode);
HKEY GetMainKey(LPCSTR szKeyName);

/* UI helpers */
void CenterWindow(HWND hwnd);
void UpdateListHScroll(HWND hList, HDC hDC);

/* Window procedures */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif /* REGDEMO_H */
```

#### regdemo.rc

```rc
/* regdemo.rc - Resource file for Win32 Registry Demo */

#include <windows.h>
#include "regdemo.h"

IDI_REGDEMO ICON "regdemo.ico"
```

#### build.bat

```bat
@echo off
REM Build script for Win32 RegDemo using MSVC
REM Run from a Visual Studio Developer Command Prompt

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
```

#### Makefile

```makefile
# Makefile for Win32 RegDemo
# Supports MSVC (nmake) and MinGW (make)
#
# MSVC:  nmake -f Makefile
# MinGW: make -f Makefile TOOLCHAIN=mingw

TOOLCHAIN ?= msvc

ifeq ($(TOOLCHAIN),mingw)
# ---- MinGW ----
CC       = gcc
RC       = windres
CFLAGS   = -Wall -Wextra -O2 -mwindows -D_WIN32_WINNT=0x0400
LDFLAGS  = -mwindows
LIBS     = -ladvapi32 -luser32 -lgdi32 -lkernel32
TARGET   = regdemo.exe
OBJS     = regdemo.o regdemo_res.o

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

regdemo.o: regdemo.c regdemo.h
	$(CC) $(CFLAGS) -c regdemo.c -o regdemo.o

regdemo_res.o: regdemo.rc regdemo.h
	$(RC) regdemo.rc regdemo_res.o

clean:
	del /Q $(TARGET) $(OBJS) 2>NUL || rm -f $(TARGET) $(OBJS)

else
# ---- MSVC (nmake) ----
CC       = cl
RC_CMD   = rc
LINK     = link
CFLAGS   = /nologo /W3 /O2 /D_WIN32_WINNT=0x0400 /DWIN32 /D_WINDOWS
LDFLAGS  = /nologo /subsystem:windows
LIBS     = advapi32.lib user32.lib gdi32.lib kernel32.lib
TARGET   = regdemo.exe
OBJS     = regdemo.obj
RES      = regdemo.res

$(TARGET): $(OBJS) $(RES)
	$(LINK) $(LDFLAGS) /out:$(TARGET) $(OBJS) $(RES) $(LIBS)

regdemo.obj: regdemo.c regdemo.h
	$(CC) $(CFLAGS) /c regdemo.c

regdemo.res: regdemo.rc regdemo.h
	$(RC_CMD) /fo regdemo.res regdemo.rc

clean:
	del /Q $(TARGET) $(OBJS) $(RES) 2>NUL

endif

.PHONY: clean
```
