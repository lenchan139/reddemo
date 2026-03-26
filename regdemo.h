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
