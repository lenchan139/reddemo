/*
 * regdemo.c - Win32 Registry Demo
 *
 * Port of VB4 RegDemo by Don Bradner (1995) to Win32 C.
 * NT4 compatible. Pure Win32 API, no common controls v6.
 *
 * Original placed in the public domain by Don Bradner.
 * Warning: Editing registry values can seriously impact your
 *   computer's operations.
 */

#include "regdemo.h"

/* ---- Globals ---- */
int      g_iWinVers      = 0;
HCURSOR  g_hWaitCursor   = NULL;
DWORD    g_lCurrentType   = REG_SZ;
int      g_iList2Tab      = 0;
HFONT    g_hFont          = NULL;
HFONT    g_hBoldFont      = NULL;
HWND     g_hList1         = NULL;
HWND     g_hList2         = NULL;
HWND     g_hMainWnd       = NULL;
HINSTANCE g_hInst         = NULL;

/* State carried between List1 click and dblclick, mirroring the VB globals */
static HKEY  s_lMainKey    = 0;
static char  s_sSubKey[4096];
static int   s_iLevel      = 0;
static int   s_iListIdx    = 0;

/* Forward declarations for internal helpers */
static void  MainOnCreate(HWND hwnd);
static void  MainOnSize(HWND hwnd, int cx, int cy);
static void  MainOnList1Click(HWND hwnd);
static void  MainOnList1DblClick(HWND hwnd);
static void  MainOnList2DblClick(HWND hwnd);

/* Forward declaration for EDITPARAMS and ShowEditDialog */
typedef struct tagEDITPARAMS {
    HKEY    hMainKey;
    char    szSubKey[4096];
    char    szValueName[2048];
    char    szValueData[4096];
    DWORD   dwValueType;
    BOOL    bOK;
} EDITPARAMS;
static INT_PTR ShowEditDialog(HWND hParent, EDITPARAMS *pEP);

/* ================================================================
 *  Registry helper: return human-readable error string
 * ================================================================ */
LPCSTR RtnRegError(LONG errorcode)
{
    switch (errorcode) {
    case 1009: case 1015:
        return "The Registry Database is corrupt!";
    case 2: case 1010:
        return "Bad Key Name!";
    case 1011:
        return "Can't Open Key";
    case 4: case 1012:
        return "Can't Read Key";
    case 5:
        return "Access to this key is denied.";
    case 1013:
        return "Can't Write Key";
    case 8: case 14:
        return "Out of memory";
    case 87:
        return "Invalid Parameter";
    case 234:
        return "Error - There is more data than the buffer can handle!";
    default: {
        static char buf[80];
        wsprintfA(buf, "Undefined Key Error Code %ld!", errorcode);
        return buf;
    }
    }
}

/* ================================================================
 *  Convert main key name string to HKEY constant
 * ================================================================ */
HKEY GetMainKey(LPCSTR szKeyName)
{
    if (lstrcmpA(szKeyName, "HKEY_CLASSES_ROOT") == 0)      return HKEY_CLASSES_ROOT;
    if (lstrcmpA(szKeyName, "HKEY_CURRENT_USER") == 0)      return HKEY_CURRENT_USER;
    if (lstrcmpA(szKeyName, "HKEY_LOCAL_MACHINE") == 0)      return HKEY_LOCAL_MACHINE;
    if (lstrcmpA(szKeyName, "HKEY_USERS") == 0)              return HKEY_USERS;
    if (lstrcmpA(szKeyName, "HKEY_PERFORMANCE_DATA") == 0)   return HKEY_PERFORMANCE_DATA;
    if (lstrcmpA(szKeyName, "HKEY_CURRENT_CONFIG") == 0)     return HKEY_CURRENT_CONFIG;
    if (lstrcmpA(szKeyName, "HKEY_DYN_DATA") == 0)           return HKEY_DYN_DATA;
    return 0;
}

/* ================================================================
 *  Center a window on screen
 * ================================================================ */
void CenterWindow(HWND hwnd)
{
    RECT rc;
    int cx, cy;
    GetWindowRect(hwnd, &rc);
    cx = GetSystemMetrics(SM_CXSCREEN);
    cy = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, NULL,
        (cx - (rc.right - rc.left)) / 2,
        (cy - (rc.bottom - rc.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* ================================================================
 *  Update horizontal scrollbar extent for a listbox
 * ================================================================ */
void UpdateListHScroll(HWND hList, HDC hDC)
{
    int i, count, maxWidth = 0;
    SIZE sz;
    char buf[4096];

    count = (int)SendMessageA(hList, LB_GETCOUNT, 0, 0);
    for (i = 0; i < count; i++) {
        int len = (int)SendMessageA(hList, LB_GETTEXT, (WPARAM)i, (LPARAM)buf);
        if (len > 0) {
            GetTextExtentPoint32A(hDC, buf, len, &sz);
            if (sz.cx > maxWidth) maxWidth = sz.cx;
        }
    }
    SendMessageA(hList, LB_SETHORIZONTALEXTENT, (WPARAM)maxWidth, 0);
}

/* ================================================================
 *  RegEnumKeysToList - enumerate subkeys into List1
 *  Mirrors VB RegEnumKeys function
 * ================================================================ */
LONG RegEnumKeysToList(HKEY hMainKey, LPCSTR szSubKey, int iLevels, int iStartList)
{
    LONG lRtn, lRet;
    HKEY hKey = NULL, hKey2 = NULL;
    DWORD dwIndex = 0;
    FILETIME ftLastWrite;
    char szClassName[256];
    DWORD dwClassLen, dwSubKeys, dwMaxSubKey, dwMaxClass;
    DWORD dwValues, dwMaxValueName, dwMaxValueData, dwSecDesc;
    DWORD dwSubKeys2, dwMaxSubKey2, dwMaxClass2;
    char szSubKeyName[512];
    DWORD dwLenSubKey;
    char szClassString[256];
    DWORD dwLenClass;

    SetCursor(g_hWaitCursor);

    lRtn = RegOpenKeyExA(hMainKey, szSubKey, 0, KEY_READ, &hKey);
    if (lRtn != ERROR_SUCCESS) {
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
        return lRtn;
    }

    dwClassLen = sizeof(szClassName);
    lRet = RegQueryInfoKeyA(hKey, szClassName, &dwClassLen, NULL,
        &dwSubKeys, &dwMaxSubKey, &dwMaxClass,
        &dwValues, &dwMaxValueName, &dwMaxValueData,
        &dwSecDesc, &ftLastWrite);

    dwIndex = 0;
    lRtn = ERROR_SUCCESS;

    while (lRtn == ERROR_SUCCESS) {
        dwLenSubKey = dwMaxSubKey + 1;
        if (dwLenSubKey < sizeof(szSubKeyName))
            ; /* ok */
        else
            dwLenSubKey = sizeof(szSubKeyName) - 1;
        dwLenClass = dwMaxClass + 1;
        if (dwLenClass > sizeof(szClassString))
            dwLenClass = sizeof(szClassString) - 1;

RetryKeyEnum:
        ZeroMemory(szSubKeyName, sizeof(szSubKeyName));
        dwLenSubKey = dwMaxSubKey + 1;
        dwLenClass = dwMaxClass + 1;

        lRtn = RegEnumKeyExA(hKey, dwIndex, szSubKeyName, &dwLenSubKey,
            NULL, szClassString, &dwLenClass, &ftLastWrite);

        if (lRtn == ERROR_SUCCESS) {
            char szSubKey2[4096];
            char szEntry[4096];
            int nSpaces = (iLevels + 1) * LIST_INDENT;

            /* Build full subkey path */
            if (szSubKey[0] != '\0')
                wsprintfA(szSubKey2, "%s\\%s", szSubKey, szSubKeyName);
            else
                lstrcpyA(szSubKey2, szSubKeyName);

            /* Check if this key has subkeys */
            dwSubKeys2 = 0;
            lRet = RegOpenKeyExA(hMainKey, szSubKey2, 0, KEY_READ, &hKey2);
            if (lRet == ERROR_SUCCESS) {
                char cn2[256];
                DWORD cl2 = sizeof(cn2);
                DWORD v2, vn2, vd2, sd2;
                FILETIME ft2;
                RegQueryInfoKeyA(hKey2, cn2, &cl2, NULL,
                    &dwSubKeys2, &dwMaxSubKey2, &dwMaxClass2,
                    &v2, &vn2, &vd2, &sd2, &ft2);
                RegCloseKey(hKey2);
                hKey2 = NULL;
            }

            /* Build indented string with +/* prefix */
            memset(szEntry, ' ', nSpaces);
            szEntry[nSpaces] = '\0';

            if (lRet == ERROR_SUCCESS && dwSubKeys2 > 0) {
                lstrcatA(szEntry, "+");
            } else if (lRet == ERROR_ACCESS_DENIED) {
                lstrcatA(szEntry, "*");
            }
            lstrcatA(szEntry, szSubKeyName);

            SendMessageA(g_hList1, LB_INSERTSTRING, (WPARAM)iStartList, (LPARAM)szEntry);
            iStartList++;
            dwIndex++;
        } else if (lRtn == ERROR_MORE_DATA) {
            dwMaxSubKey += 5;
            dwMaxClass += 5;
            lRtn = ERROR_SUCCESS; /* reset so loop continues */
            goto RetryKeyEnum;
        } else if (lRtn == ERROR_NO_MORE_ITEMS) {
            lRtn = ERROR_SUCCESS;
            break;
        } else {
            MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
            break;
        }
    }

    RegCloseKey(hKey);
    return lRtn;
}

/* ================================================================
 *  RegEnumValuesToList - enumerate values into List2
 *  Mirrors VB RegEnumValues function
 * ================================================================ */
LONG RegEnumValuesToList(HKEY hMainKey, LPCSTR szSubKey)
{
    LONG lRtn;
    HKEY hKey = NULL;
    DWORD dwIndex = 0;
    char szClassName[256];
    DWORD dwClassLen, dwSubKeys, dwMaxSubKey, dwMaxClass;
    DWORD dwValues, dwMaxValueName, dwMaxValueData, dwSecDesc;
    FILETIME ftLastWrite;
    int iListWidth = 0;
    HDC hDC;

    SetCursor(g_hWaitCursor);

    lRtn = RegOpenKeyExA(hMainKey, szSubKey, 0, KEY_READ, &hKey);
    if (lRtn != ERROR_SUCCESS) {
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
        return lRtn;
    }

    dwClassLen = sizeof(szClassName);
    lRtn = RegQueryInfoKeyA(hKey, szClassName, &dwClassLen, NULL,
        &dwSubKeys, &dwMaxSubKey, &dwMaxClass,
        &dwValues, &dwMaxValueName, &dwMaxValueData,
        &dwSecDesc, &ftLastWrite);

    /* Reset horizontal scrollbar */
    SendMessageA(g_hList2, LB_SETHORIZONTALEXTENT, 0, 0);

    hDC = GetDC(g_hMainWnd);
    SelectObject(hDC, g_hFont);

    dwIndex = 0;
    lRtn = ERROR_SUCCESS;

    while (lRtn == ERROR_SUCCESS) {
        char szValueName[2048];
        DWORD dwLenValueName;
        BYTE *pValueData = NULL;
        DWORD dwLenValue;
        DWORD dwValueType;
        char szEntry[8192];
        char szDisplayValue[4096];

RetryValueEnum:
        /* Cap data size for display */
        if (dwMaxValueData > MAX_VALUE_DISPLAY)
            dwMaxValueData = MAX_VALUE_DISPLAY;

        dwLenValueName = dwMaxValueName + 2;
        if (dwLenValueName > sizeof(szValueName))
            dwLenValueName = sizeof(szValueName);
        ZeroMemory(szValueName, sizeof(szValueName));

        dwLenValue = dwMaxValueData + 2;
        pValueData = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLenValue + 1);
        if (!pValueData) break;

        lRtn = RegEnumValueA(hKey, dwIndex, szValueName, &dwLenValueName,
            NULL, &dwValueType, pValueData, &dwLenValue);

        if (lRtn == ERROR_SUCCESS) {
            const char *pName;
            DWORD dwNameLen;

            /* Default values have no name */
            if (dwLenValueName == 0) {
                pName = "(Default)";
                dwNameLen = 9;
            } else {
                szValueName[dwLenValueName] = '\0';
                pName = szValueName;
                dwNameLen = dwLenValueName;
            }

            if (dwMaxValueName < dwNameLen)
                dwMaxValueName = dwNameLen;

            szDisplayValue[0] = '\0';

            if (dwLenValue < MAX_VALUE_DISPLAY) {
                switch (dwValueType) {
                case REG_MULTI_SZ: {
                    /* Replace embedded NULs with spaces */
                    DWORD k;
                    char *p = (char *)pValueData;
                    for (k = 0; k < dwLenValue; k++) {
                        if (p[k] == '\0') p[k] = ' ';
                    }
                    if (dwLenValue > 0) p[dwLenValue - 1] = '\0';
                    lstrcpynA(szDisplayValue, p, sizeof(szDisplayValue));
                    break;
                }
                case REG_SZ: {
                    char *p = (char *)pValueData;
                    if (dwLenValue > 0 && p[dwLenValue - 1] == '\0')
                        dwLenValue--;
                    p[dwLenValue] = '\0';
                    wsprintfA(szDisplayValue, "\"%s\"", p);
                    break;
                }
                case REG_EXPAND_SZ: {
                    char *p = (char *)pValueData;
                    if (dwLenValue > 0 && p[dwLenValue - 1] == '\0')
                        dwLenValue--;
                    p[dwLenValue] = '\0';
                    wsprintfA(szDisplayValue, "\"%s\"", p);
                    break;
                }
                case REG_FULL_RESOURCE_DESCRIPTOR:
                    lstrcpyA(szDisplayValue, "REG_FULL_RESOURCE_DESCRIPTOR");
                    break;
                case REG_DWORD: {
                    DWORD dwVal = 0;
                    if (dwLenValue >= 4)
                        dwVal = *(DWORD *)pValueData;
                    wsprintfA(szDisplayValue, "&H%X (%u)", dwVal, dwVal);
                    break;
                }
                case REG_BINARY: {
                    DWORD k;
                    char *p = szDisplayValue;
                    for (k = 0; k < dwLenValue && (p - szDisplayValue) < (int)sizeof(szDisplayValue) - 4; k++) {
                        wsprintfA(p, "%02X ", pValueData[k]);
                        p += 3;
                    }
                    if (p > szDisplayValue) *(p - 1) = '\0';
                    break;
                }
                default:
                    wsprintfA(szDisplayValue, "(type %u)", dwValueType);
                    break;
                }

                if (szDisplayValue[0] == '\0')
                    lstrcpyA(szDisplayValue, "(value not set)");
            } else {
                lstrcpyA(szDisplayValue, "(Value too large to display)");
            }

            /* Build tab-aligned entry: name + spaces + value */
            {
                int pad = (int)dwMaxValueName - (int)dwNameLen + 1;
                if (pad < 1) pad = 1;
                wsprintfA(szEntry, "%s", pName);
                memset(szEntry + lstrlenA(szEntry), ' ', pad);
                szEntry[lstrlenA(pName) + pad] = '\0';
                lstrcatA(szEntry, szDisplayValue);
            }

            SendMessageA(g_hList2, LB_ADDSTRING, 0, (LPARAM)szEntry);

            /* Update horizontal scroll */
            {
                SIZE sz;
                GetTextExtentPoint32A(hDC, szEntry, lstrlenA(szEntry), &sz);
                if (sz.cx > iListWidth) {
                    iListWidth = sz.cx;
                    SendMessageA(g_hList2, LB_SETHORIZONTALEXTENT, (WPARAM)iListWidth, 0);
                }
            }

            dwIndex++;
        } else if (lRtn == ERROR_MORE_DATA) {
            dwMaxValueData += 5;
            dwMaxValueName += 5;
            lRtn = ERROR_SUCCESS;
            HeapFree(GetProcessHeap(), 0, pValueData);
            pValueData = NULL;
            goto RetryValueEnum;
        } else if (lRtn == ERROR_NO_MORE_ITEMS) {
            lRtn = ERROR_SUCCESS;
            HeapFree(GetProcessHeap(), 0, pValueData);
            break;
        } else {
            MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
            HeapFree(GetProcessHeap(), 0, pValueData);
            break;
        }

        if (pValueData) HeapFree(GetProcessHeap(), 0, pValueData);
    }

    g_iList2Tab = (int)dwMaxValueName;

    ReleaseDC(g_hMainWnd, hDC);
    RegCloseKey(hKey);
    return lRtn;
}

/* ================================================================
 *  RegSetValueByType - write REG_SZ or REG_DWORD value
 *  Mirrors VB RegSetValue function
 * ================================================================ */
LONG RegSetValueByType(HKEY hMainKey, LPCSTR szSubKey, LPCSTR szValueName, LPCSTR szValue)
{
    LONG lRtn;
    HKEY hKey = NULL;

    if (g_lCurrentType != REG_SZ && g_lCurrentType != REG_DWORD) {
        MessageBoxA(g_hMainWnd,
            "This demo only supports writing keys of the types REG_SZ and REG_DWORD.",
            "RegDemo", MB_OK | MB_ICONWARNING);
        return ERROR_INVALID_PARAMETER;
    }

    lRtn = RegOpenKeyExA(hMainKey, szSubKey, 0, KEY_WRITE, &hKey);
    if (lRtn != ERROR_SUCCESS) {
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
        return lRtn;
    }

    if (g_lCurrentType == REG_DWORD) {
        /* Parse hex string to DWORD */
        DWORD dwVal;
        char szHex[20];
        lstrcpynA(szHex, szValue, sizeof(szHex));
        /* If it doesn't start with 0x, prepend */
        if (szHex[0] == '0' && (szHex[1] == 'x' || szHex[1] == 'X'))
            dwVal = strtoul(szHex, NULL, 16);
        else
            dwVal = strtoul(szHex, NULL, 16);
        lRtn = RegSetValueExA(hKey, szValueName, 0, REG_DWORD, (const BYTE *)&dwVal, sizeof(DWORD));
    } else {
        lRtn = RegSetValueExA(hKey, szValueName, 0, REG_SZ, (const BYTE *)szValue, lstrlenA(szValue));
    }

    if (lRtn != ERROR_SUCCESS)
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);

    RegCloseKey(hKey);
    return lRtn;
}

/* ================================================================
 *  EditRegValue - open Form2 edit dialog for a specific value
 *  Mirrors VB EditRegValue sub
 * ================================================================ */
void EditRegValue(HKEY hMainKey, LPCSTR szSubKey, int iRegIndex)
{
    LONG lRtn;
    HKEY hKey = NULL;
    char szClassName[256];
    DWORD dwClassLen, dwSubKeys, dwMaxSubKey, dwMaxClass;
    DWORD dwValues, dwMaxValueName, dwMaxValueData, dwSecDesc;
    FILETIME ftLastWrite;
    char szValueName[2048];
    DWORD dwLenValueName;
    BYTE *pValueData = NULL;
    DWORD dwLenValue;
    DWORD dwValueType;

    lRtn = RegOpenKeyExA(hMainKey, szSubKey, 0, KEY_READ, &hKey);
    if (lRtn != ERROR_SUCCESS) {
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
        return;
    }

    dwClassLen = sizeof(szClassName);
    RegQueryInfoKeyA(hKey, szClassName, &dwClassLen, NULL,
        &dwSubKeys, &dwMaxSubKey, &dwMaxClass,
        &dwValues, &dwMaxValueName, &dwMaxValueData,
        &dwSecDesc, &ftLastWrite);

RetryEditValue:
    if (dwMaxValueData > MAX_VALUE_DISPLAY)
        dwMaxValueData = MAX_VALUE_DISPLAY;

    dwLenValueName = dwMaxValueName + 2;
    if (dwLenValueName > sizeof(szValueName))
        dwLenValueName = sizeof(szValueName);
    ZeroMemory(szValueName, sizeof(szValueName));

    dwLenValue = dwMaxValueData + 2;
    pValueData = (BYTE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLenValue + 4);
    if (!pValueData) { RegCloseKey(hKey); return; }

    lRtn = RegEnumValueA(hKey, (DWORD)iRegIndex, szValueName, &dwLenValueName,
        NULL, &dwValueType, pValueData, &dwLenValue);

    if (lRtn == ERROR_SUCCESS) {
        if (dwValueType != REG_SZ && dwValueType != REG_DWORD) {
            /* Not editable */
            const char *pTypeName = "unknown";
            char msg[512];
            switch (dwValueType) {
            case 2:  pTypeName = "REG_EXPAND_SZ"; break;
            case 3:  pTypeName = "REG_BINARY"; break;
            case 5:  pTypeName = "REG_DWORD_BIG_ENDIAN"; break;
            case 6:  pTypeName = "REG_LINK"; break;
            case 7:  pTypeName = "REG_MULTI_SZ"; break;
            case 8:  pTypeName = "REG_RESOURCE_LIST"; break;
            case 9:  pTypeName = "REG_FULL_RESOURCE_DESCRIPTOR"; break;
            case 10: pTypeName = "REG_RESOURCE_REQUIREMENTS_LIST"; break;
            }
            wsprintfA(msg,
                "This Demo only supports editing of values with types of "
                "REG_SZ and REG_DWORD. This value is of type %s.", pTypeName);
            MessageBoxA(g_hMainWnd, msg, "RegDemo", MB_OK | MB_ICONINFORMATION);
        } else {
            /* Editable - prepare data for the dialog */
            EDITPARAMS ep;
            ZeroMemory(&ep, sizeof(ep));
            ep.hMainKey = hMainKey;
            lstrcpynA(ep.szSubKey, szSubKey, sizeof(ep.szSubKey));
            ep.dwValueType = dwValueType;

            if (dwLenValueName == 0)
                lstrcpyA(ep.szValueName, "(Default)");
            else {
                szValueName[dwLenValueName] = '\0';
                lstrcpynA(ep.szValueName, szValueName, sizeof(ep.szValueName));
            }

            if (dwValueType == REG_DWORD) {
                DWORD dwVal = 0;
                if (dwLenValue >= 4)
                    dwVal = *(DWORD *)pValueData;
                wsprintfA(ep.szValueData, "%X", dwVal);
            } else {
                /* REG_SZ */
                char *p = (char *)pValueData;
                if (dwLenValue > 0 && p[dwLenValue - 1] == '\0')
                    dwLenValue--;
                p[dwLenValue] = '\0';
                lstrcpynA(ep.szValueData, p, sizeof(ep.szValueData));
            }

            g_lCurrentType = dwValueType;

            /* Show the edit dialog (in-memory template) */
            ShowEditDialog(g_hMainWnd, &ep);
        }
    } else if (lRtn == ERROR_MORE_DATA) {
        if (dwMaxValueData >= MAX_VALUE_DISPLAY) {
            MessageBoxA(g_hMainWnd, "Value is too large for this editor!", "RegDemo", MB_OK);
        } else {
            dwMaxValueData += 5;
            dwMaxValueName += 5;
            HeapFree(GetProcessHeap(), 0, pValueData);
            pValueData = NULL;
            goto RetryEditValue;
        }
    } else {
        MessageBoxA(g_hMainWnd, RtnRegError(lRtn), "RegDemo", MB_OK | MB_ICONERROR);
    }

    if (pValueData) HeapFree(GetProcessHeap(), 0, pValueData);
    RegCloseKey(hKey);
}

/* ================================================================
 *  Edit dialog procedure (Form2)
 * ================================================================ */

/* The edit params struct is forward-declared above as EDITPARAMS */

INT_PTR CALLBACK EditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    EDITPARAMS *pEP;

    switch (msg) {
    case WM_INITDIALOG:
        pEP = (EDITPARAMS *)lParam;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)pEP);

        /* Set title based on type */
        if (pEP->dwValueType == REG_DWORD)
            SetWindowTextA(hwnd, "Edit DWORD Value");
        else
            SetWindowTextA(hwnd, "Edit String Value");

        /* Fill fields */
        SetDlgItemTextA(hwnd, IDC_EDIT_VALNAME, pEP->szValueName);
        SetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, pEP->szValueData);

        /* Show/hide base radio frame */
        if (pEP->dwValueType == REG_DWORD) {
            ShowWindow(GetDlgItem(hwnd, IDC_FRAME_BASE), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_RADIO_HEX), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_RADIO_DEC), SW_SHOW);
            CheckRadioButton(hwnd, IDC_RADIO_HEX, IDC_RADIO_DEC, IDC_RADIO_HEX);
        } else {
            ShowWindow(GetDlgItem(hwnd, IDC_FRAME_BASE), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_RADIO_HEX), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, IDC_RADIO_DEC), SW_HIDE);
        }

        /* Select all text in value data */
        SendDlgItemMessageA(hwnd, IDC_EDIT_VALDATA, EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_VALDATA));

        CenterWindow(hwnd);
        return FALSE; /* we set focus manually */

    case WM_COMMAND:
        pEP = (EDITPARAMS *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

        switch (LOWORD(wParam)) {
        case IDC_EDIT_OK: {
            /* OK button */
            char szNewValue[4096];
            char szActualName[2048];
            LONG nRtn;

            GetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, szNewValue, sizeof(szNewValue));

            /* Get actual value name (empty string for Default) */
            lstrcpynA(szActualName, pEP->szValueName, sizeof(szActualName));
            if (lstrcmpA(szActualName, "(Default)") == 0)
                szActualName[0] = '\0';

            if (g_lCurrentType == REG_DWORD) {
                /* Convert to hex string for RegSetValueByType */
                BOOL bHex = (IsDlgButtonChecked(hwnd, IDC_RADIO_HEX) == BST_CHECKED);
                DWORD dwVal;
                char szHexVal[20];

                if (bHex) {
                    dwVal = strtoul(szNewValue, NULL, 16);
                } else {
                    /* Parse as decimal, could be > 2^31 */
                    double dv = strtod(szNewValue, NULL);
                    dwVal = (DWORD)dv;
                }
                wsprintfA(szHexVal, "%X", dwVal);

                nRtn = RegSetValueByType(pEP->hMainKey, pEP->szSubKey, szActualName, szHexVal);

                if (nRtn == 0) {
                    /* Update list2 entry */
                    char szDisplay[4096];
                    int idx = (int)SendMessageA(g_hList2, LB_GETCURSEL, 0, 0);
                    int pad = g_iList2Tab - lstrlenA(szActualName[0] ? szActualName : "(Default)") + 1;
                    if (pad < 1) pad = 1;

                    wsprintfA(szDisplay, "%s", pEP->szValueName);
                    {
                        int len = lstrlenA(szDisplay);
                        memset(szDisplay + len, ' ', pad);
                        szDisplay[len + pad] = '\0';
                    }
                    {
                        char valpart[128];
                        wsprintfA(valpart, "&H%X (%u)", dwVal, dwVal);
                        lstrcatA(szDisplay, valpart);
                    }

                    if (idx != LB_ERR) {
                        SendMessageA(g_hList2, LB_DELETESTRING, (WPARAM)idx, 0);
                        SendMessageA(g_hList2, LB_INSERTSTRING, (WPARAM)idx, (LPARAM)szDisplay);
                        SendMessageA(g_hList2, LB_SETCURSEL, (WPARAM)idx, 0);
                    }
                    EndDialog(hwnd, IDOK);
                }
            } else {
                /* REG_SZ */
                nRtn = RegSetValueByType(pEP->hMainKey, pEP->szSubKey, szActualName, szNewValue);

                if (nRtn == 0) {
                    char szDisplay[8192];
                    int idx = (int)SendMessageA(g_hList2, LB_GETCURSEL, 0, 0);
                    int pad = g_iList2Tab - lstrlenA(szActualName[0] ? szActualName : "(Default)") + 1;
                    if (pad < 1) pad = 1;

                    wsprintfA(szDisplay, "%s", pEP->szValueName);
                    {
                        int len = lstrlenA(szDisplay);
                        memset(szDisplay + len, ' ', pad);
                        szDisplay[len + pad] = '\0';
                    }
                    {
                        char valpart[4096];
                        wsprintfA(valpart, "\"%s\"", szNewValue);
                        lstrcatA(szDisplay, valpart);
                    }

                    if (idx != LB_ERR) {
                        SendMessageA(g_hList2, LB_DELETESTRING, (WPARAM)idx, 0);
                        SendMessageA(g_hList2, LB_INSERTSTRING, (WPARAM)idx, (LPARAM)szDisplay);
                        SendMessageA(g_hList2, LB_SETCURSEL, (WPARAM)idx, 0);
                    }
                    EndDialog(hwnd, IDOK);
                }
            }
            return TRUE;
        }

        case IDC_EDIT_CANCEL:
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;

        case IDC_RADIO_HEX:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Convert decimal -> hex */
                char buf[64];
                double dv;
                DWORD dwVal;
                GetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, buf, sizeof(buf));
                dv = strtod(buf, NULL);
                dwVal = (DWORD)dv;
                wsprintfA(buf, "%X", dwVal);
                SetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, buf);
            }
            return TRUE;

        case IDC_RADIO_DEC:
            if (HIWORD(wParam) == BN_CLICKED) {
                /* Convert hex -> decimal */
                char buf[64];
                DWORD dwVal;
                GetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, buf, sizeof(buf));
                dwVal = strtoul(buf, NULL, 16);
                wsprintfA(buf, "%u", dwVal);
                SetDlgItemTextA(hwnd, IDC_EDIT_VALDATA, buf);
            }
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

/* ================================================================
 *  Main form - List1 click handler
 *  Mirrors VB List1_Click
 * ================================================================ */
static void MainOnList1Click(HWND hwnd)
{
    int idx, iLevel_local;
    char szItem[4096];
    char szSub[4096];

    SendMessageA(g_hList2, LB_RESETCONTENT, 0, 0);

    idx = (int)SendMessageA(g_hList1, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) return;

    s_iListIdx = idx;
    SendMessageA(g_hList1, LB_GETTEXT, (WPARAM)idx, (LPARAM)szItem);

    iLevel_local = 0;
    s_sSubKey[0] = '\0';

    if (szItem[0] != ' ') {
        /* Top-level key */
        s_lMainKey = GetMainKey(szItem);
        s_iLevel = 0;
    } else {
        /* Subkey - count leading spaces */
        char *p = szItem;
        while (*p == ' ') { iLevel_local++; p++; }

        /* Strip indicator chars */
        if (*p == '+' || *p == '*') p++;

        lstrcpyA(szSub, p);
        iLevel_local /= LIST_INDENT;

        /* Walk up the list to build the full key path */
        /* VB logic: for each level from current down to 1, find the parent
           by scanning upward until we find an item whose leading spaces
           are fewer than iX * LIST_INDENT */
        {
            int iX;
            for (iX = iLevel_local; iX >= 1; iX--) {
                int nSpaces = iX * LIST_INDENT;
                int iY = s_iListIdx;
                char szUp[4096];
                char szSpaceBuf[256];

                /* Build comparison prefix of nSpaces spaces */
                if (nSpaces > (int)sizeof(szSpaceBuf) - 1)
                    nSpaces = sizeof(szSpaceBuf) - 1;
                memset(szSpaceBuf, ' ', nSpaces);
                szSpaceBuf[nSpaces] = '\0';

                /* Walk up while item starts with nSpaces spaces */
                do {
                    iY--;
                    if (iY < 0) break;
                    SendMessageA(g_hList1, LB_GETTEXT, (WPARAM)iY, (LPARAM)szUp);
                } while (iY >= 0 && memcmp(szUp, szSpaceBuf, nSpaces) == 0);

                /* szUp now holds the first item above us with fewer spaces */
                if (iY >= 0) {
                    SendMessageA(g_hList1, LB_GETTEXT, (WPARAM)iY, (LPARAM)szUp);
                    if (szUp[0] != ' ') {
                        /* Hit a main key */
                        s_lMainKey = GetMainKey(szUp);
                    } else {
                        char *q = szUp;
                        while (*q == ' ') q++;
                        if (*q == '+') q++;
                        /* Prepend to szSub */
                        {
                            char tmp[4096];
                            wsprintfA(tmp, "%s\\%s", q, szSub);
                            lstrcpynA(szSub, tmp, sizeof(szSub));
                        }
                    }
                }
            }
        }

        lstrcpynA(s_sSubKey, szSub, sizeof(s_sSubKey));
        s_iLevel = iLevel_local;
    }

    /* Enumerate values */
    RegEnumValuesToList(s_lMainKey, s_sSubKey);
}

/* ================================================================
 *  Main form - List1 double-click handler
 *  Mirrors VB List1_DblClick
 * ================================================================ */
static void MainOnList1DblClick(HWND hwnd)
{
    int nSpaces;
    int iX;
    BOOL bAlreadyExpanded = FALSE;
    char szNext[4096];
    char szSpaces[256];

    nSpaces = (s_iLevel + 1) * LIST_INDENT;
    if (nSpaces > (int)sizeof(szSpaces) - 1) nSpaces = sizeof(szSpaces) - 1;
    memset(szSpaces, ' ', nSpaces);
    szSpaces[nSpaces] = '\0';

    iX = s_iListIdx + 1;

    /* Check if already expanded - collapse if so */
    while (iX < (int)SendMessageA(g_hList1, LB_GETCOUNT, 0, 0)) {
        SendMessageA(g_hList1, LB_GETTEXT, (WPARAM)iX, (LPARAM)szNext);
        if (memcmp(szNext, szSpaces, nSpaces) == 0 && szNext[0] == ' ') {
            SendMessageA(g_hList1, LB_DELETESTRING, (WPARAM)iX, 0);
            bAlreadyExpanded = TRUE;
        } else {
            break;
        }
    }

    if (bAlreadyExpanded) return;

    /* Expand one level */
    RegEnumKeysToList(s_lMainKey, s_sSubKey, s_iLevel, s_iListIdx + 1);

    /* Update horizontal scrollbar */
    {
        HDC hDC = GetDC(hwnd);
        SelectObject(hDC, g_hFont);
        UpdateListHScroll(g_hList1, hDC);
        ReleaseDC(hwnd, hDC);
    }
}

/* ================================================================
 *  Main form - List2 double-click handler
 * ================================================================ */
static void MainOnList2DblClick(HWND hwnd)
{
    int idx = (int)SendMessageA(g_hList2, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) return;
    EditRegValue(s_lMainKey, s_sSubKey, idx);
}

/* ================================================================
 *  Create controls in main window
 * ================================================================ */
static void MainOnCreate(HWND hwnd)
{
    DWORD dwVer;
    LOGFONTA lf;
    HDC hDC;

    g_hMainWnd = hwnd;

    /* Detect Windows version */
    dwVer = GetVersion();
    if (dwVer < 0x80000000)
        g_iWinVers = WINVER_NT;
    else
        g_iWinVers = WINVER_9X;

    /* Create fonts */
    hDC = GetDC(hwnd);
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lstrcpyA(lf.lfFaceName, "Courier New");
    g_hFont = CreateFontIndirectA(&lf);

    lf.lfWeight = FW_BOLD;
    lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    lstrcpyA(lf.lfFaceName, "MS Sans Serif");
    g_hBoldFont = CreateFontIndirectA(&lf);
    ReleaseDC(hwnd, hDC);

    /* Labels */
    {
        HWND h;
        h = CreateWindowExA(0, "STATIC", "Key Name",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            4, 4, 120, 16, hwnd, (HMENU)IDC_LABEL1, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        h = CreateWindowExA(0, "STATIC", "Value Data",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            260, 4, 120, 16, hwnd, (HMENU)IDC_LABEL2, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        h = CreateWindowExA(0, "STATIC", "Double-Click a Key to expand it",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            4, 300, 240, 16, hwnd, (HMENU)IDC_LABEL3, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        h = CreateWindowExA(0, "STATIC", "Double-Click a Value to edit it",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            260, 300, 240, 16, hwnd, (HMENU)IDC_LABEL4, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        h = CreateWindowExA(0, "STATIC", "+ means additional keys",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            16, 320, 220, 16, hwnd, (HMENU)IDC_LABEL5_0, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);

        h = CreateWindowExA(0, "STATIC", "* means key cannot be opened",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            16, 338, 220, 16, hwnd, (HMENU)IDC_LABEL5_1, g_hInst, NULL);
        SendMessageA(h, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
    }

    /* Listboxes */
    g_hList1 = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_TABSTOP,
        4, 22, 240, 270, hwnd, (HMENU)IDC_LIST1, g_hInst, NULL);
    SendMessageA(g_hList1, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    g_hList2 = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_TABSTOP,
        250, 22, 250, 270, hwnd, (HMENU)IDC_LIST2, g_hInst, NULL);
    SendMessageA(g_hList2, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    /* Exit button */
    {
        HWND hBtn = CreateWindowExA(0, "BUTTON", "Exit",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            400, 340, 80, 28, hwnd, (HMENU)IDC_EXIT_BTN, g_hInst, NULL);
        SendMessageA(hBtn, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
    }

    /* Populate root keys */
    SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_CLASSES_ROOT");
    SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_CURRENT_USER");
    SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_LOCAL_MACHINE");
    SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_USERS");

    if (g_iWinVers == WINVER_9X) {
        SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_CURRENT_CONFIG");
        SendMessageA(g_hList1, LB_ADDSTRING, 0, (LPARAM)"HKEY_DYN_DATA");
    }

    /* Setup hourglass cursor */
    g_hWaitCursor = LoadCursorA(NULL, IDC_WAIT);
}

/* ================================================================
 *  Resize handler - mirrors VB Form_Resize
 * ================================================================ */
static void MainOnSize(HWND hwnd, int cx, int cy)
{
    int listH, list1W, list2L, list2W;
    int labelY, label5Y0, label5Y1, btnX, btnY;

    if (cx < 100 || cy < 100) return;

    listH  = cy - 90;
    list1W = (cx / 2) - 10;
    list2L = list1W + 12;
    list2W = cx - list2L - 4;

    MoveWindow(g_hList1, 4, 22, list1W, listH, TRUE);
    MoveWindow(g_hList2, list2L, 22, list2W, listH, TRUE);

    /* Labels */
    labelY  = 22 + listH + 6;
    label5Y0 = labelY + 18;
    label5Y1 = label5Y0 + 18;

    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL1), NULL, 4 + 4, 4, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL2), NULL, list2L + 4, 4, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL3), NULL, 4, labelY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL4), NULL, list2L + 4, labelY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL5_0), NULL, 16, label5Y0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd, IDC_LABEL5_1), NULL, 16, label5Y1, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    btnX = cx - 90;
    btnY = labelY + 20;
    SetWindowPos(GetDlgItem(hwnd, IDC_EXIT_BTN), NULL, btnX, btnY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* ================================================================
 *  Main window procedure
 * ================================================================ */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        MainOnCreate(hwnd);
        return 0;

    case WM_SIZE:
        MainOnSize(hwnd, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_EXIT_BTN:
            DestroyWindow(hwnd);
            return 0;
        case IDC_LIST1:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                MainOnList1Click(hwnd);
            } else if (HIWORD(wParam) == LBN_DBLCLK) {
                /* Click handler already fired on LBN_SELCHANGE,
                   now handle double-click expansion */
                MainOnList1DblClick(hwnd);
            }
            return 0;
        case IDC_LIST2:
            if (HIWORD(wParam) == LBN_DBLCLK) {
                MainOnList2DblClick(hwnd);
            }
            return 0;
        }
        break;

    case WM_DESTROY:
        if (g_hFont) DeleteObject(g_hFont);
        if (g_hBoldFont) DeleteObject(g_hBoldFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ================================================================
 *  Edit dialog template (built in memory for NT4 compat -
 *  avoids needing a .dlg file or RC dialog resource)
 * ================================================================ */

/* Helper to DWORD-align a pointer */
static LPWORD dlgAlignDWORD(LPWORD p)
{
    ULONG_PTR ul = (ULONG_PTR)p;
    ul = (ul + 3) & ~3;
    return (LPWORD)ul;
}

static LPDLGTEMPLATE BuildEditDialog(void)
{
    /*
     * Build the edit dialog template in memory.
     * This is the NT4-safe way to create dialogs without .rc compiled resources.
     *
     * Dialog layout (in dialog units):
     *   Width: 260, Height: 170
     *   - Label "Value Name:" at (10, 4, 100, 10)
     *   - Edit (read-only) for value name at (10, 16, 240, 14)  IDC_EDIT_VALNAME
     *   - Label "Value Data:" at (10, 36, 100, 10)
     *   - Edit for value data at (10, 48, 240, 14)              IDC_EDIT_VALDATA
     *   - GroupBox "Base" at (10, 70, 100, 48)                  IDC_FRAME_BASE (hidden)
     *   - Radio "Hexadecimal" at (18, 82, 80, 12)              IDC_RADIO_HEX (hidden)
     *   - Radio "Decimal" at (18, 96, 80, 12)                  IDC_RADIO_DEC (hidden)
     *   - Button "OK" at (150, 90, 50, 16)                     IDC_EDIT_OK
     *   - Button "Cancel" at (204, 90, 50, 16)                 IDC_EDIT_CANCEL
     */

    /* Allocate enough memory for the template */
    HGLOBAL hGlobal;
    LPDLGTEMPLATE lpdt;
    LPDLGITEMTEMPLATE lpdit;
    LPWORD lpw;
    int nItems = 9; /* 2 labels + 2 edits + 1 group + 2 radios + 2 buttons */

    hGlobal = GlobalAlloc(GMEM_ZEROINIT, 4096);
    if (!hGlobal) return NULL;

    lpdt = (LPDLGTEMPLATE)GlobalLock(hGlobal);

    lpdt->style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_SETFONT;
    lpdt->dwExtendedStyle = 0;
    lpdt->cdit = (WORD)nItems;
    lpdt->x = 0; lpdt->y = 0;
    lpdt->cx = 260; lpdt->cy = 130;

    /* Menu - none */
    lpw = (LPWORD)(lpdt + 1);
    *lpw++ = 0; /* no menu */
    *lpw++ = 0; /* default class */

    /* Title */
    {
        const char *title = "Edit Value";
        int i;
        for (i = 0; title[i]; i++)
            *lpw++ = (WORD)title[i];
        *lpw++ = 0;
    }

    /* Font (DS_SETFONT) - 8pt MS Sans Serif */
    *lpw++ = 8; /* point size */
    {
        const char *fn = "MS Sans Serif";
        int i;
        for (i = 0; fn[i]; i++)
            *lpw++ = (WORD)fn[i];
        *lpw++ = 0;
    }

    /* --- Controls --- */

    /* 1. Label "Value Name:" */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 4; lpdit->cx = 100; lpdit->cy = 10;
    lpdit->id = (WORD)-1;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0082; /* STATIC */
    { const char *t = "Value Name:"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0; /* no creation data */

    /* 2. Edit - Value Name (read-only) */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 16; lpdit->cx = 240; lpdit->cy = 14;
    lpdit->id = IDC_EDIT_VALNAME;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0081; /* EDIT */
    *lpw++ = 0; /* no text */
    *lpw++ = 0; /* no creation data */

    /* 3. Label "Value Data:" */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 36; lpdit->cx = 100; lpdit->cy = 10;
    lpdit->id = (WORD)-1;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0082; /* STATIC */
    { const char *t = "Value Data:"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0; /* no creation data */

    /* 4. Edit - Value Data */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 48; lpdit->cx = 240; lpdit->cy = 14;
    lpdit->id = IDC_EDIT_VALDATA;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0081; /* EDIT */
    *lpw++ = 0; /* no text */
    *lpw++ = 0; /* no creation data */

    /* 5. GroupBox "Base" (hidden by default) */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | BS_GROUPBOX; /* not WS_VISIBLE */
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 10; lpdit->y = 70; lpdit->cx = 100; lpdit->cy = 48;
    lpdit->id = IDC_FRAME_BASE;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; /* BUTTON (groupbox) */
    { const char *t = "Base"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0;

    /* 6. Radio "Hexadecimal" (hidden) */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON; /* not WS_VISIBLE */
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 18; lpdit->y = 82; lpdit->cx = 80; lpdit->cy = 12;
    lpdit->id = IDC_RADIO_HEX;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; /* BUTTON */
    { const char *t = "Hexadecimal"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0;

    /* 7. Radio "Decimal" (hidden) */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON; /* not WS_VISIBLE */
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 18; lpdit->y = 96; lpdit->cx = 80; lpdit->cy = 12;
    lpdit->id = IDC_RADIO_DEC;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; /* BUTTON */
    { const char *t = "Decimal"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0;

    /* 8. Button "OK" */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 150; lpdit->y = 106; lpdit->cx = 50; lpdit->cy = 16;
    lpdit->id = IDC_EDIT_OK;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; /* BUTTON */
    { const char *t = "OK"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0;

    /* 9. Button "Cancel" */
    lpw = dlgAlignDWORD(lpw);
    lpdit = (LPDLGITEMTEMPLATE)lpw;
    lpdit->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
    lpdit->dwExtendedStyle = 0;
    lpdit->x = 204; lpdit->y = 106; lpdit->cx = 50; lpdit->cy = 16;
    lpdit->id = IDC_EDIT_CANCEL;
    lpw = (LPWORD)(lpdit + 1);
    *lpw++ = 0xFFFF; *lpw++ = 0x0080; /* BUTTON */
    { const char *t = "Cancel"; int i; for (i = 0; t[i]; i++) *lpw++ = (WORD)t[i]; *lpw++ = 0; }
    *lpw++ = 0;

    GlobalUnlock(hGlobal);
    return (LPDLGTEMPLATE)hGlobal;
}

/* Store the dialog template globally so EditRegValue can use it */
static LPDLGTEMPLATE g_pEditDlgTemplate = NULL;

/* Redirect DialogBoxParam to use in-memory template */
static void InitEditDialog(void)
{
    g_pEditDlgTemplate = BuildEditDialog();
}

static INT_PTR ShowEditDialog(HWND hParent, EDITPARAMS *pEP)
{
    if (!g_pEditDlgTemplate) return -1;
    return DialogBoxIndirectParamA(g_hInst, g_pEditDlgTemplate,
        hParent, EditDlgProc, (LPARAM)pEP);
}

/* ================================================================
 *  WinMain
 * ================================================================ */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXA wc;
    HWND hwnd;
    MSG msg;

    (void)hPrevInstance;
    (void)lpCmdLine;

    g_hInst = hInstance;

    /* Build in-memory edit dialog template */
    InitEditDialog();

    /* Register main window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = MainWndProc;
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_REGDEMO));
    wc.hCursor        = LoadCursorA(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName  = "RegDemoMainClass";
    wc.hIconSm        = wc.hIcon;

    if (!RegisterClassExA(&wc)) return 1;

    hwnd = CreateWindowExA(0, "RegDemoMainClass", "Win32 RegDemo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 420,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 1;

    CenterWindow(hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    /* Clean up dialog template */
    if (g_pEditDlgTemplate) GlobalFree(g_pEditDlgTemplate);

    return (int)msg.wParam;
}
