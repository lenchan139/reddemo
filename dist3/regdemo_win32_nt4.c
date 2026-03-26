#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef HKEY_DYN_DATA
#define HKEY_DYN_DATA ((HKEY)(ULONG_PTR)((LONG)0x80000006L))
#endif

#define APP_TITLE "Win32 RegDemo"
#define MAIN_CLASS_NAME "RegDemoMainClass"
#define EDIT_CLASS_NAME "RegDemoEditClass"

#define IDC_LIST_KEYS 1001
#define IDC_LIST_VALUES 1002
#define IDC_BTN_EXIT 1003
#define IDC_LBL_KEY 1004
#define IDC_LBL_VALUE 1005
#define IDC_LBL_HINT_KEYS 1006
#define IDC_LBL_HINT_VALUES 1007
#define IDC_LBL_PLUS 1008
#define IDC_LBL_DENIED 1009

#define IDC_EDT_NAME 2001
#define IDC_EDT_VALUE 2002
#define IDC_BTN_OK 2003
#define IDC_BTN_CANCEL 2004
#define IDC_RAD_HEX 2005
#define IDC_RAD_DEC 2006
#define IDC_GRP_BASE 2007
#define IDC_LBL_NAME 2008
#define IDC_LBL_DATA 2009

#define LIST_INDENT 2
#define MAX_DISPLAY_DATA 20000

typedef struct TreeItem {
    HKEY rootKey;
    char *subKey;
    char *leafName;
    int level;
    int hasChildren;
    int accessDenied;
    int isRoot;
} TreeItem;

typedef struct ValueItem {
    char *name;
    char *displayName;
    char *displayValue;
    char *editValue;
    DWORD type;
    int editable;
} ValueItem;

typedef struct EditState {
    HWND hwnd;
    HWND parent;
    HFONT hFont;
    int done;
    int accepted;
    DWORD valueType;
    char *valueName;
    char *initialValue;
    int baseHex;
    char *resultText;
    DWORD resultDword;
} EditState;

typedef struct AppState {
    HWND hwnd;
    HWND listKeys;
    HWND listValues;
    HWND btnExit;
    HWND lblKey;
    HWND lblValue;
    HWND lblHintKeys;
    HWND lblHintValues;
    HWND lblPlus;
    HWND lblDenied;
    HFONT hUiFont;
    HFONT hMonoFont;
    HCURSOR hWaitCursor;
    int list2Tab;
} AppState;

static AppState gApp;

static char *xstrdup(const char *src) {
    size_t n;
    char *dst;
    if (src == NULL) {
        dst = (char *)malloc(1);
        if (dst) dst[0] = '\0';
        return dst;
    }
    n = strlen(src);
    dst = (char *)malloc(n + 1);
    if (!dst) return NULL;
    memcpy(dst, src, n + 1);
    return dst;
}

static void center_window(HWND hwnd, HWND parent) {
    RECT wr;
    RECT pr;
    int w;
    int h;
    int x;
    int y;

    GetWindowRect(hwnd, &wr);
    w = wr.right - wr.left;
    h = wr.bottom - wr.top;

    if (parent != NULL && GetWindowRect(parent, &pr)) {
        x = pr.left + ((pr.right - pr.left) - w) / 2;
        y = pr.top + ((pr.bottom - pr.top) - h) / 2;
    } else {
        x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
        y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    }

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void free_tree_item(TreeItem *item) {
    if (!item) return;
    if (item->subKey) free(item->subKey);
    if (item->leafName) free(item->leafName);
    free(item);
}

static void free_value_item(ValueItem *item) {
    if (!item) return;
    if (item->name) free(item->name);
    if (item->displayName) free(item->displayName);
    if (item->displayValue) free(item->displayValue);
    if (item->editValue) free(item->editValue);
    free(item);
}

static void clear_list_with_data(HWND list, int isTree) {
    int i;
    int n = (int)SendMessageA(list, LB_GETCOUNT, 0, 0);
    for (i = 0; i < n; ++i) {
        void *p = (void *)SendMessageA(list, LB_GETITEMDATA, (WPARAM)i, 0);
        if (p && p != (void *)LB_ERR) {
            if (isTree) free_tree_item((TreeItem *)p);
            else free_value_item((ValueItem *)p);
        }
    }
    SendMessageA(list, LB_RESETCONTENT, 0, 0);
}

static const char *root_name(HKEY key) {
    if (key == HKEY_CLASSES_ROOT) return "HKEY_CLASSES_ROOT";
    if (key == HKEY_CURRENT_USER) return "HKEY_CURRENT_USER";
    if (key == HKEY_LOCAL_MACHINE) return "HKEY_LOCAL_MACHINE";
    if (key == HKEY_USERS) return "HKEY_USERS";
    if (key == HKEY_CURRENT_CONFIG) return "HKEY_CURRENT_CONFIG";
    if (key == HKEY_DYN_DATA) return "HKEY_DYN_DATA";
    return "HKEY_UNKNOWN";
}

static int is_nt_family(void) {
    DWORD v = GetVersion();
    return (v & 0x80000000UL) == 0;
}

static void format_error_message(DWORD code, char *buf, size_t buflen) {
    if (buflen == 0) return;

    switch (code) {
        case ERROR_BADDB:
        case 1015:
            lstrcpynA(buf, "The Registry Database is corrupt!", (int)buflen);
            return;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_BADKEY:
            lstrcpynA(buf, "Bad Key Name!", (int)buflen);
            return;
        case ERROR_CANTOPEN:
            lstrcpynA(buf, "Can't Open Key", (int)buflen);
            return;
        case ERROR_CANTREAD:
            lstrcpynA(buf, "Can't Read Key", (int)buflen);
            return;
        case ERROR_ACCESS_DENIED:
            lstrcpynA(buf, "Access to this key is denied.", (int)buflen);
            return;
        case ERROR_CANTWRITE:
            lstrcpynA(buf, "Can't Write Key", (int)buflen);
            return;
        case ERROR_OUTOFMEMORY:
            lstrcpynA(buf, "Out of memory", (int)buflen);
            return;
        case ERROR_INVALID_PARAMETER:
            lstrcpynA(buf, "Invalid Parameter", (int)buflen);
            return;
        case ERROR_MORE_DATA:
            lstrcpynA(buf, "Error - There is more data than the buffer can handle!", (int)buflen);
            return;
        default:
            break;
    }

    if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        code,
                        0,
                        buf,
                        (DWORD)buflen,
                        NULL)) {
        _snprintf(buf, buflen - 1, "Undefined Key Error Code %lu!", (unsigned long)code);
        buf[buflen - 1] = '\0';
    }
}

static void show_registry_error(HWND owner, DWORD code) {
    char msg[512];
    format_error_message(code, msg, sizeof(msg));
    MessageBoxA(owner, msg, APP_TITLE, MB_OK | MB_ICONERROR);
}

static int text_pixel_width(HWND list, HFONT font, const char *s) {
    HDC hdc;
    HGDIOBJ old;
    SIZE sz;
    int w = 0;

    hdc = GetDC(list);
    old = SelectObject(hdc, font);
    if (GetTextExtentPoint32A(hdc, s, (int)strlen(s), &sz)) {
        w = sz.cx + 8;
    }
    SelectObject(hdc, old);
    ReleaseDC(list, hdc);
    return w;
}

static void update_hscroll(HWND list, HFONT font) {
    int i;
    int n;
    int maxw = 0;
    n = (int)SendMessageA(list, LB_GETCOUNT, 0, 0);
    for (i = 0; i < n; ++i) {
        int len = (int)SendMessageA(list, LB_GETTEXTLEN, (WPARAM)i, 0);
        char *buf;
        int w;
        if (len <= 0) continue;
        buf = (char *)malloc((size_t)len + 1);
        if (!buf) continue;
        SendMessageA(list, LB_GETTEXT, (WPARAM)i, (LPARAM)buf);
        w = text_pixel_width(list, font, buf);
        if (w > maxw) maxw = w;
        free(buf);
    }
    SendMessageA(list, LB_SETHORIZONTALEXTENT, (WPARAM)maxw, 0);
}

static TreeItem *create_tree_item(HKEY root, const char *sub, const char *leaf, int level, int hasChildren, int accessDenied, int isRoot) {
    TreeItem *it = (TreeItem *)calloc(1, sizeof(TreeItem));
    if (!it) return NULL;
    it->rootKey = root;
    it->subKey = xstrdup(sub ? sub : "");
    it->leafName = xstrdup(leaf ? leaf : "");
    it->level = level;
    it->hasChildren = hasChildren;
    it->accessDenied = accessDenied;
    it->isRoot = isRoot;
    if (!it->subKey || !it->leafName) {
        free_tree_item(it);
        return NULL;
    }
    return it;
}

static ValueItem *create_value_item(const char *name,
                                    const char *displayName,
                                    const char *displayValue,
                                    const char *editValue,
                                    DWORD type,
                                    int editable) {
    ValueItem *it = (ValueItem *)calloc(1, sizeof(ValueItem));
    if (!it) return NULL;
    it->name = xstrdup(name ? name : "");
    it->displayName = xstrdup(displayName ? displayName : "");
    it->displayValue = xstrdup(displayValue ? displayValue : "");
    it->editValue = xstrdup(editValue ? editValue : "");
    it->type = type;
    it->editable = editable;
    if (!it->name || !it->displayName || !it->displayValue || !it->editValue) {
        free_value_item(it);
        return NULL;
    }
    return it;
}

static void tree_display_text(const TreeItem *it, char *buf, size_t buflen) {
    int i;
    int pos = 0;
    if (buflen == 0) return;
    buf[0] = '\0';

    if (it->isRoot) {
        lstrcpynA(buf, root_name(it->rootKey), (int)buflen);
        return;
    }

    for (i = 0; i < it->level * LIST_INDENT && pos + 1 < (int)buflen; ++i) {
        buf[pos++] = ' ';
    }

    if (it->accessDenied && pos + 1 < (int)buflen) {
        buf[pos++] = '*';
    } else if (it->hasChildren && pos + 1 < (int)buflen) {
        buf[pos++] = '+';
    }

    buf[pos] = '\0';
    strncat(buf, it->leafName, buflen - strlen(buf) - 1);
}

static void add_tree_item(HWND list, TreeItem *it, int index) {
    char row[1024];
    LRESULT rowIndex;
    tree_display_text(it, row, sizeof(row));

    if (index < 0) rowIndex = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)row);
    else rowIndex = SendMessageA(list, LB_INSERTSTRING, (WPARAM)index, (LPARAM)row);

    if (rowIndex == LB_ERR || rowIndex == LB_ERRSPACE) {
        free_tree_item(it);
        return;
    }

    SendMessageA(list, LB_SETITEMDATA, (WPARAM)rowIndex, (LPARAM)it);
}

static void add_value_item(HWND list, ValueItem *it, int tabWidth) {
    LRESULT rowIndex;
    int spaces;
    size_t total;
    char *row;

    if (tabWidth < 1) tabWidth = 1;
    spaces = tabWidth - (int)strlen(it->displayName) + 1;
    if (spaces < 1) spaces = 1;

    total = strlen(it->displayName) + (size_t)spaces + strlen(it->displayValue) + 1;
    row = (char *)malloc(total);
    if (!row) {
        free_value_item(it);
        return;
    }

    row[0] = '\0';
    strcat(row, it->displayName);
    while (spaces-- > 0) strcat(row, " ");
    strcat(row, it->displayValue);

    rowIndex = SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)row);
    free(row);

    if (rowIndex == LB_ERR || rowIndex == LB_ERRSPACE) {
        free_value_item(it);
        return;
    }

    SendMessageA(list, LB_SETITEMDATA, (WPARAM)rowIndex, (LPARAM)it);
}

static char *join_subkey(const char *parent, const char *child) {
    size_t a = parent ? strlen(parent) : 0;
    size_t b = child ? strlen(child) : 0;
    char *out = (char *)malloc(a + b + 2);
    if (!out) return NULL;
    out[0] = '\0';
    if (a > 0) {
        strcat(out, parent);
        strcat(out, "\\");
    }
    if (b > 0) strcat(out, child);
    return out;
}

static void subkey_flags(HKEY root, const char *sub, int *hasChildren, int *accessDenied) {
    HKEY h;
    LONG rc;
    DWORD subCount = 0;

    *hasChildren = 0;
    *accessDenied = 0;

    rc = RegOpenKeyExA(root, (sub && sub[0]) ? sub : NULL, 0, KEY_READ, &h);
    if (rc == ERROR_ACCESS_DENIED) {
        *accessDenied = 1;
        return;
    }
    if (rc != ERROR_SUCCESS) return;

    rc = RegQueryInfoKeyA(h, NULL, NULL, NULL, &subCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (rc == ERROR_SUCCESS && subCount > 0) *hasChildren = 1;

    RegCloseKey(h);
}

static void populate_roots(AppState *app) {
    add_tree_item(app->listKeys, create_tree_item(HKEY_CLASSES_ROOT, "", root_name(HKEY_CLASSES_ROOT), 0, 1, 0, 1), -1);
    add_tree_item(app->listKeys, create_tree_item(HKEY_CURRENT_USER, "", root_name(HKEY_CURRENT_USER), 0, 1, 0, 1), -1);
    add_tree_item(app->listKeys, create_tree_item(HKEY_LOCAL_MACHINE, "", root_name(HKEY_LOCAL_MACHINE), 0, 1, 0, 1), -1);
    add_tree_item(app->listKeys, create_tree_item(HKEY_USERS, "", root_name(HKEY_USERS), 0, 1, 0, 1), -1);

    if (!is_nt_family()) {
        add_tree_item(app->listKeys, create_tree_item(HKEY_CURRENT_CONFIG, "", root_name(HKEY_CURRENT_CONFIG), 0, 1, 0, 1), -1);
        add_tree_item(app->listKeys, create_tree_item(HKEY_DYN_DATA, "", root_name(HKEY_DYN_DATA), 0, 1, 0, 1), -1);
    }

    update_hscroll(app->listKeys, app->hMonoFont);
}

static const char *reg_type_name(DWORD type) {
    switch (type) {
        case REG_NONE: return "REG_NONE";
        case REG_SZ: return "REG_SZ";
        case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
        case REG_BINARY: return "REG_BINARY";
        case REG_DWORD: return "REG_DWORD";
        case REG_DWORD_BIG_ENDIAN: return "REG_DWORD_BIG_ENDIAN";
        case REG_LINK: return "REG_LINK";
        case REG_MULTI_SZ: return "REG_MULTI_SZ";
        case REG_RESOURCE_LIST: return "REG_RESOURCE_LIST";
        case REG_FULL_RESOURCE_DESCRIPTOR: return "REG_FULL_RESOURCE_DESCRIPTOR";
        case REG_RESOURCE_REQUIREMENTS_LIST: return "REG_RESOURCE_REQUIREMENTS_LIST";
        default: return "REG_UNKNOWN";
    }
}

static char *format_binary(const BYTE *data, DWORD n) {
    DWORD i;
    char *s;
    char *p;
    size_t len;

    if (n == 0) return xstrdup("(value not set)");

    len = (size_t)n * 3 + 1;
    s = (char *)malloc(len);
    if (!s) return NULL;
    s[0] = '\0';
    p = s;

    for (i = 0; i < n; ++i) {
        _snprintf(p, len - (size_t)(p - s), "%02X ", data[i]);
        p += strlen(p);
    }

    return s;
}

static void enumerate_values(AppState *app, const TreeItem *sel) {
    HKEY h;
    LONG rc;
    DWORD idx = 0;
    DWORD maxName = 0;
    DWORD maxData = 0;

    clear_list_with_data(app->listValues, 0);
    SendMessageA(app->listValues, LB_SETHORIZONTALEXTENT, 0, 0);

    rc = RegOpenKeyExA(sel->rootKey, (sel->subKey && sel->subKey[0]) ? sel->subKey : NULL, 0, KEY_READ, &h);
    if (rc != ERROR_SUCCESS) {
        show_registry_error(app->hwnd, (DWORD)rc);
        return;
    }

    rc = RegQueryInfoKeyA(h, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &maxName, &maxData, NULL, NULL);
    if (rc != ERROR_SUCCESS) {
        RegCloseKey(h);
        show_registry_error(app->hwnd, (DWORD)rc);
        return;
    }

    app->list2Tab = (int)maxName;
    if (app->list2Tab < 9) app->list2Tab = 9;

    for (;;) {
        char *nameBuf;
        BYTE *dataBuf;
        DWORD nameLen = maxName + 1;
        DWORD dataLen = maxData + 1;
        DWORD type = 0;
        char *displayName;
        char *displayValue = NULL;
        char *editValue = NULL;
        int editable = 0;
        ValueItem *item;

        nameBuf = (char *)malloc((size_t)maxName + 2);
        dataBuf = (BYTE *)malloc((size_t)maxData + 2);
        if (!nameBuf || !dataBuf) {
            if (nameBuf) free(nameBuf);
            if (dataBuf) free(dataBuf);
            MessageBoxA(app->hwnd, "Out of memory while reading registry values.", APP_TITLE, MB_OK | MB_ICONERROR);
            break;
        }

        nameBuf[0] = '\0';
        rc = RegEnumValueA(h, idx, nameBuf, &nameLen, NULL, &type, dataBuf, &dataLen);
        if (rc == ERROR_MORE_DATA) {
            maxData += 256;
            maxName += 64;
            free(nameBuf);
            free(dataBuf);
            continue;
        }
        if (rc == ERROR_NO_MORE_ITEMS) {
            free(nameBuf);
            free(dataBuf);
            break;
        }
        if (rc != ERROR_SUCCESS) {
            free(nameBuf);
            free(dataBuf);
            show_registry_error(app->hwnd, (DWORD)rc);
            break;
        }

        if (nameLen == 0) displayName = xstrdup("(Default)");
        else displayName = xstrdup(nameBuf);

        if (!displayName) {
            free(nameBuf);
            free(dataBuf);
            MessageBoxA(app->hwnd, "Out of memory while preparing value display.", APP_TITLE, MB_OK | MB_ICONERROR);
            break;
        }

        if (dataLen > MAX_DISPLAY_DATA) {
            displayValue = xstrdup("(Value too large to display)");
            editValue = xstrdup("");
        } else {
            if (type == REG_SZ) {
                size_t textLen = dataLen;
                char *raw;
                if (textLen > 0 && dataBuf[textLen - 1] == '\0') textLen--;
                raw = (char *)malloc(textLen + 1);
                if (raw) {
                    memcpy(raw, dataBuf, textLen);
                    raw[textLen] = '\0';
                    editValue = xstrdup(raw);

                    displayValue = (char *)malloc(textLen + 3);
                    if (displayValue) {
                        displayValue[0] = '"';
                        memcpy(displayValue + 1, raw, textLen);
                        displayValue[textLen + 1] = '"';
                        displayValue[textLen + 2] = '\0';
                    }
                    free(raw);
                }
                editable = 1;
            } else if (type == REG_DWORD) {
                char buf[128];
                char ebuf[32];
                DWORD d = 0;
                if (dataLen >= 4) {
                    d = (DWORD)dataBuf[0] | ((DWORD)dataBuf[1] << 8) | ((DWORD)dataBuf[2] << 16) | ((DWORD)dataBuf[3] << 24);
                    _snprintf(buf, sizeof(buf) - 1, "&H%lX (%lu)", (unsigned long)d, (unsigned long)d);
                    buf[sizeof(buf) - 1] = '\0';
                    _snprintf(ebuf, sizeof(ebuf) - 1, "%lX", (unsigned long)d);
                    ebuf[sizeof(ebuf) - 1] = '\0';
                    displayValue = xstrdup(buf);
                    editValue = xstrdup(ebuf);
                    editable = 1;
                } else {
                    displayValue = xstrdup("(Invalid DWORD data)");
                    editValue = xstrdup("");
                }
            } else if (type == REG_MULTI_SZ) {
                DWORD i;
                char *tmp = (char *)malloc(dataLen + 1);
                if (tmp) {
                    for (i = 0; i < dataLen; ++i) tmp[i] = (dataBuf[i] == '\0') ? ' ' : (char)dataBuf[i];
                    tmp[dataLen] = '\0';
                    displayValue = tmp;
                }
                editValue = xstrdup("");
            } else if (type == REG_BINARY) {
                displayValue = format_binary(dataBuf, dataLen);
                editValue = xstrdup("");
            } else if (type == REG_FULL_RESOURCE_DESCRIPTOR) {
                displayValue = xstrdup("REG_FULL_RESOURCE_DESCRIPTOR");
                editValue = xstrdup("");
            } else {
                displayValue = xstrdup(reg_type_name(type));
                editValue = xstrdup("");
            }
        }

        if (!displayValue || displayValue[0] == '\0') {
            if (displayValue) free(displayValue);
            displayValue = xstrdup("(value not set)");
        }
        if (!editValue) editValue = xstrdup("");

        item = create_value_item(nameBuf, displayName, displayValue, editValue, type, editable);
        if (item) add_value_item(app->listValues, item, app->list2Tab);

        free(displayName);
        free(displayValue);
        free(editValue);
        free(nameBuf);
        free(dataBuf);

        idx++;
    }

    RegCloseKey(h);
    update_hscroll(app->listValues, app->hMonoFont);
}

static void enumerate_subkeys(AppState *app, const TreeItem *sel, int insertAt) {
    HKEY h;
    LONG rc;
    DWORD maxSub = 0;
    DWORD maxClass = 0;
    DWORD subCount = 0;
    DWORD idx;
    HCURSOR oldc;

    oldc = SetCursor(app->hWaitCursor);

    rc = RegOpenKeyExA(sel->rootKey, (sel->subKey && sel->subKey[0]) ? sel->subKey : NULL, 0, KEY_READ, &h);
    if (rc != ERROR_SUCCESS) {
        SetCursor(oldc);
        show_registry_error(app->hwnd, (DWORD)rc);
        return;
    }

    rc = RegQueryInfoKeyA(h, NULL, NULL, NULL, &subCount, &maxSub, &maxClass, NULL, NULL, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS) {
        RegCloseKey(h);
        SetCursor(oldc);
        show_registry_error(app->hwnd, (DWORD)rc);
        return;
    }

    for (idx = 0; idx < subCount; ++idx) {
        DWORD len = maxSub + 1;
        FILETIME ft;
        char *name = (char *)malloc((size_t)maxSub + 2);
        TreeItem *child;
        char *full;
        int hasChildren = 0;
        int accessDenied = 0;

        if (!name) break;
        name[0] = '\0';

        rc = RegEnumKeyExA(h, idx, name, &len, NULL, NULL, NULL, &ft);
        if (rc == ERROR_MORE_DATA) {
            free(name);
            continue;
        }
        if (rc != ERROR_SUCCESS) {
            free(name);
            show_registry_error(app->hwnd, (DWORD)rc);
            break;
        }

        full = join_subkey(sel->subKey, name);
        if (!full) {
            free(name);
            break;
        }

        subkey_flags(sel->rootKey, full, &hasChildren, &accessDenied);
        child = create_tree_item(sel->rootKey, full, name, sel->level + 1, hasChildren, accessDenied, 0);
        if (child) {
            add_tree_item(app->listKeys, child, insertAt);
            insertAt++;
        }

        free(full);
        free(name);
    }

    RegCloseKey(h);
    update_hscroll(app->listKeys, app->hMonoFont);
    SetCursor(oldc);
}

static void collapse_subtree(AppState *app, int parentIndex, int parentLevel) {
    int count = (int)SendMessageA(app->listKeys, LB_GETCOUNT, 0, 0);
    int idx = parentIndex + 1;

    while (idx < count) {
        TreeItem *it = (TreeItem *)SendMessageA(app->listKeys, LB_GETITEMDATA, (WPARAM)idx, 0);
        if (!it || it == (TreeItem *)LB_ERR || it->level <= parentLevel) break;
        free_tree_item(it);
        SendMessageA(app->listKeys, LB_DELETESTRING, (WPARAM)idx, 0);
        count = (int)SendMessageA(app->listKeys, LB_GETCOUNT, 0, 0);
    }

    update_hscroll(app->listKeys, app->hMonoFont);
}

static DWORD parse_u32(const char *s, int hex, int *ok) {
    unsigned long long v;
    char *end;
    while (*s == ' ' || *s == '\t') s++;
    if (hex) {
        if ((s[0] == '&') && (s[1] == 'H' || s[1] == 'h')) s += 2;
        if ((s[0] == '0') && (s[1] == 'x' || s[1] == 'X')) s += 2;
        v = strtoull(s, &end, 16);
    } else {
        v = strtoull(s, &end, 10);
    }
    while (*end == ' ' || *end == '\t') end++;
    if (end == s || *end != '\0' || v > 0xFFFFFFFFULL) {
        *ok = 0;
        return 0;
    }
    *ok = 1;
    return (DWORD)v;
}

static void set_edit_numeric_text(HWND hEdit, DWORD v, int hex) {
    char buf[64];
    if (hex) _snprintf(buf, sizeof(buf) - 1, "%lX", (unsigned long)v);
    else _snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)v);
    buf[sizeof(buf) - 1] = '\0';
    SetWindowTextA(hEdit, buf);
    SendMessageA(hEdit, EM_SETSEL, 0, -1);
}

static void switch_base(EditState *st, int newHex) {
    HWND hEdit;
    char buf[256];
    int ok;
    DWORD v;

    if (st->baseHex == newHex) return;
    hEdit = GetDlgItem(st->hwnd, IDC_EDT_VALUE);
    GetWindowTextA(hEdit, buf, sizeof(buf));
    v = parse_u32(buf, st->baseHex, &ok);
    if (ok) set_edit_numeric_text(hEdit, v, newHex);
    st->baseHex = newHex;
}

static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    EditState *st = (EditState *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            st = (EditState *)((CREATESTRUCT *)lParam)->lpCreateParams;
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)st);
            st->hwnd = hwnd;
            return TRUE;

        case WM_CREATE: {
            HWND h;
            int isDword = (st->valueType == REG_DWORD);

            h = CreateWindowExA(0, "STATIC", "Value Name:", WS_CHILD | WS_VISIBLE, 20, 14, 130, 18, hwnd, (HMENU)IDC_LBL_NAME, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);

            h = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", (st->valueName && st->valueName[0]) ? st->valueName : "(Default)", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 20, 34, 500, 22, hwnd, (HMENU)IDC_EDT_NAME, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);

            h = CreateWindowExA(0, "STATIC", "Value Data:", WS_CHILD | WS_VISIBLE, 20, 70, 130, 18, hwnd, (HMENU)IDC_LBL_DATA, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);

            h = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", st->initialValue ? st->initialValue : "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 90, 500, 22, hwnd, (HMENU)IDC_EDT_VALUE, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);
            SendMessageA(h, EM_SETSEL, 0, -1);

            h = CreateWindowExA(0, "BUTTON", "Base", WS_CHILD | BS_GROUPBOX | WS_VISIBLE, 20, 124, 220, 76, hwnd, (HMENU)IDC_GRP_BASE, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);
            ShowWindow(h, isDword ? SW_SHOW : SW_HIDE);

            h = CreateWindowExA(0, "BUTTON", "Hexadecimal", WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE, 38, 146, 170, 20, hwnd, (HMENU)IDC_RAD_HEX, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);
            SendMessageA(h, BM_SETCHECK, BST_CHECKED, 0);
            ShowWindow(h, isDword ? SW_SHOW : SW_HIDE);

            h = CreateWindowExA(0, "BUTTON", "Decimal", WS_CHILD | BS_AUTORADIOBUTTON | WS_VISIBLE, 38, 168, 170, 20, hwnd, (HMENU)IDC_RAD_DEC, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);
            ShowWindow(h, isDword ? SW_SHOW : SW_HIDE);

            h = CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 320, isDword ? 162 : 132, 95, 26, hwnd, (HMENU)IDC_BTN_OK, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);

            h = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 426, isDword ? 162 : 132, 95, 26, hwnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);
            SendMessageA(h, WM_SETFONT, (WPARAM)st->hFont, TRUE);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BTN_OK: {
                    char buf[512];
                    GetWindowTextA(GetDlgItem(hwnd, IDC_EDT_VALUE), buf, sizeof(buf));
                    if (st->valueType == REG_DWORD) {
                        int ok;
                        st->resultDword = parse_u32(buf, st->baseHex, &ok);
                        if (!ok) {
                            MessageBoxA(hwnd, "Enter a valid DWORD value.", APP_TITLE, MB_OK | MB_ICONERROR);
                            SetFocus(GetDlgItem(hwnd, IDC_EDT_VALUE));
                            return 0;
                        }
                    } else {
                        if (st->resultText) free(st->resultText);
                        st->resultText = xstrdup(buf);
                        if (!st->resultText) {
                            MessageBoxA(hwnd, "Out of memory.", APP_TITLE, MB_OK | MB_ICONERROR);
                            return 0;
                        }
                    }
                    st->accepted = 1;
                    DestroyWindow(hwnd);
                    return 0;
                }
                case IDC_BTN_CANCEL:
                    DestroyWindow(hwnd);
                    return 0;
                case IDC_RAD_HEX:
                    if (HIWORD(wParam) == BN_CLICKED) switch_base(st, 1);
                    return 0;
                case IDC_RAD_DEC:
                    if (HIWORD(wParam) == BN_CLICKED) switch_base(st, 0);
                    return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            st->done = 1;
            EnableWindow(st->parent, TRUE);
            SetActiveWindow(st->parent);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int show_edit_dialog(HWND parent, HFONT hFont, ValueItem *item, char **outText, DWORD *outDword) {
    EditState st;
    MSG msg;
    RECT pr;
    HWND dlg;
    int h = (item->type == REG_DWORD) ? 240 : 200;

    memset(&st, 0, sizeof(st));
    st.parent = parent;
    st.hFont = hFont;
    st.valueType = item->type;
    st.valueName = item->name;
    st.initialValue = item->editValue;
    st.baseHex = 1;

    GetWindowRect(parent, &pr);

    dlg = CreateWindowExA(WS_EX_DLGMODALFRAME,
                          EDIT_CLASS_NAME,
                          (item->type == REG_DWORD) ? "Edit DWORD Value" : "Edit String Value",
                          WS_POPUP | WS_CAPTION | WS_SYSMENU,
                          pr.left + 40,
                          pr.top + 40,
                          560,
                          h,
                          parent,
                          NULL,
                          NULL,
                          &st);
    if (!dlg) return 0;

    EnableWindow(parent, FALSE);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    center_window(dlg, parent);

    while (!st.done && GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (st.accepted) {
        if (item->type == REG_DWORD) {
            *outDword = st.resultDword;
            *outText = NULL;
        } else {
            *outText = st.resultText;
            st.resultText = NULL;
        }
    }

    if (st.resultText) free(st.resultText);
    return st.accepted;
}

static int save_registry_value(HWND owner, const TreeItem *sel, const ValueItem *item, const char *text, DWORD dwordVal) {
    HKEY h;
    LONG rc;

    rc = RegOpenKeyExA(sel->rootKey, (sel->subKey && sel->subKey[0]) ? sel->subKey : NULL, 0, KEY_SET_VALUE, &h);
    if (rc != ERROR_SUCCESS) {
        show_registry_error(owner, (DWORD)rc);
        return 0;
    }

    if (item->type == REG_DWORD) {
        rc = RegSetValueExA(h, item->name[0] ? item->name : NULL, 0, REG_DWORD, (const BYTE *)&dwordVal, 4);
    } else {
        DWORD cb = (DWORD)strlen(text);
        rc = RegSetValueExA(h, item->name[0] ? item->name : NULL, 0, REG_SZ, (const BYTE *)text, cb);
    }

    RegCloseKey(h);

    if (rc != ERROR_SUCCESS) {
        show_registry_error(owner, (DWORD)rc);
        return 0;
    }

    return 1;
}

static void refresh_selected_key_values(AppState *app) {
    int idx = (int)SendMessageA(app->listKeys, LB_GETCURSEL, 0, 0);
    TreeItem *sel;
    if (idx == LB_ERR) {
        clear_list_with_data(app->listValues, 0);
        return;
    }
    sel = (TreeItem *)SendMessageA(app->listKeys, LB_GETITEMDATA, (WPARAM)idx, 0);
    if (!sel || sel == (TreeItem *)LB_ERR) return;
    enumerate_values(app, sel);
}

static void toggle_expand_selected(AppState *app) {
    int idx;
    int count;
    TreeItem *sel;

    idx = (int)SendMessageA(app->listKeys, LB_GETCURSEL, 0, 0);
    if (idx == LB_ERR) return;

    sel = (TreeItem *)SendMessageA(app->listKeys, LB_GETITEMDATA, (WPARAM)idx, 0);
    if (!sel || sel == (TreeItem *)LB_ERR) return;
    if (sel->accessDenied || !sel->hasChildren) return;

    count = (int)SendMessageA(app->listKeys, LB_GETCOUNT, 0, 0);
    if (idx + 1 < count) {
        TreeItem *next = (TreeItem *)SendMessageA(app->listKeys, LB_GETITEMDATA, (WPARAM)(idx + 1), 0);
        if (next && next != (TreeItem *)LB_ERR && next->level > sel->level) {
            collapse_subtree(app, idx, sel->level);
            return;
        }
    }

    enumerate_subkeys(app, sel, idx + 1);
}

static void edit_selected_value(AppState *app) {
    int idxKey;
    int idxVal;
    TreeItem *sel;
    ValueItem *v;
    char *newText = NULL;
    DWORD newDword = 0;

    idxKey = (int)SendMessageA(app->listKeys, LB_GETCURSEL, 0, 0);
    idxVal = (int)SendMessageA(app->listValues, LB_GETCURSEL, 0, 0);
    if (idxKey == LB_ERR || idxVal == LB_ERR) return;

    sel = (TreeItem *)SendMessageA(app->listKeys, LB_GETITEMDATA, (WPARAM)idxKey, 0);
    v = (ValueItem *)SendMessageA(app->listValues, LB_GETITEMDATA, (WPARAM)idxVal, 0);
    if (!sel || sel == (TreeItem *)LB_ERR || !v || v == (ValueItem *)LB_ERR) return;

    if (!v->editable) {
        char msg[256];
        _snprintf(msg,
                  sizeof(msg) - 1,
                  "This demo only supports editing REG_SZ and REG_DWORD values. This value is of type %s.",
                  reg_type_name(v->type));
        msg[sizeof(msg) - 1] = '\0';
        MessageBoxA(app->hwnd, msg, APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (!show_edit_dialog(app->hwnd, app->hUiFont, v, &newText, &newDword)) return;

    if (save_registry_value(app->hwnd, sel, v, newText ? newText : "", newDword)) {
        enumerate_values(app, sel);
        if (idxVal < (int)SendMessageA(app->listValues, LB_GETCOUNT, 0, 0)) {
            SendMessageA(app->listValues, LB_SETCURSEL, (WPARAM)idxVal, 0);
        }
    }

    if (newText) free(newText);
}

static void layout_main(AppState *app, int w, int h) {
    int margin = 8;
    int top = 26;
    int bottom = 88;
    int listH = h - top - bottom;
    int leftW = (w / 2) - 14;
    int rightW = w - leftW - (margin * 3);

    if (listH < 110) listH = 110;
    if (leftW < 160) leftW = 160;
    if (rightW < 180) rightW = 180;

    MoveWindow(app->lblKey, margin, 6, 140, 18, TRUE);
    MoveWindow(app->lblValue, margin + leftW + margin, 6, 140, 18, TRUE);
    MoveWindow(app->listKeys, margin, top, leftW, listH, TRUE);
    MoveWindow(app->listValues, margin + leftW + margin, top, rightW, listH, TRUE);

    MoveWindow(app->lblHintKeys, margin, top + listH + 10, leftW, 18, TRUE);
    MoveWindow(app->lblHintValues, margin + leftW + margin, top + listH + 10, rightW, 18, TRUE);
    MoveWindow(app->lblPlus, margin, top + listH + 32, leftW, 18, TRUE);
    MoveWindow(app->lblDenied, margin, top + listH + 52, leftW, 18, TRUE);
    MoveWindow(app->btnExit, w - 104, top + listH + 28, 90, 28, TRUE);
}

static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    AppState *app = (AppState *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
        case WM_NCCREATE:
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
            return TRUE;

        case WM_CREATE: {
            HFONT hGui = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            app = (AppState *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
            app->hwnd = hwnd;
            app->hUiFont = hGui;
            app->hMonoFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");
            if (!app->hMonoFont) app->hMonoFont = hGui;
            app->hWaitCursor = LoadCursor(NULL, IDC_WAIT);

            app->lblKey = CreateWindowExA(0, "STATIC", "Key Name", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_KEY, NULL, NULL);
            app->lblValue = CreateWindowExA(0, "STATIC", "Value Data", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_VALUE, NULL, NULL);

            app->listKeys = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hwnd, (HMENU)IDC_LIST_KEYS, NULL, NULL);
            app->listValues = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hwnd, (HMENU)IDC_LIST_VALUES, NULL, NULL);

            app->lblHintKeys = CreateWindowExA(0, "STATIC", "Double-click a Key to expand it", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_HINT_KEYS, NULL, NULL);
            app->lblHintValues = CreateWindowExA(0, "STATIC", "Double-click a Value to edit it", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_HINT_VALUES, NULL, NULL);
            app->lblPlus = CreateWindowExA(0, "STATIC", "+ means additional keys", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_PLUS, NULL, NULL);
            app->lblDenied = CreateWindowExA(0, "STATIC", "* means key cannot be opened", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LBL_DENIED, NULL, NULL);
            app->btnExit = CreateWindowExA(0, "BUTTON", "Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)IDC_BTN_EXIT, NULL, NULL);

            SendMessageA(app->lblKey, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->lblValue, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->lblHintKeys, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->lblHintValues, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->lblPlus, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->lblDenied, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->btnExit, WM_SETFONT, (WPARAM)app->hUiFont, TRUE);
            SendMessageA(app->listKeys, WM_SETFONT, (WPARAM)app->hMonoFont, TRUE);
            SendMessageA(app->listValues, WM_SETFONT, (WPARAM)app->hMonoFont, TRUE);

            populate_roots(app);
            return 0;
        }

        case WM_SIZE:
            if (app) layout_main(app, LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BTN_EXIT:
                    DestroyWindow(hwnd);
                    return 0;

                case IDC_LIST_KEYS:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        refresh_selected_key_values(app);
                    } else if (HIWORD(wParam) == LBN_DBLCLK) {
                        refresh_selected_key_values(app);
                        toggle_expand_selected(app);
                    }
                    return 0;

                case IDC_LIST_VALUES:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        edit_selected_value(app);
                    }
                    return 0;
            }
            break;

        case WM_DESTROY:
            if (app) {
                clear_list_with_data(app->listKeys, 1);
                clear_list_with_data(app->listValues, 0);
                if (app->hMonoFont && app->hMonoFont != app->hUiFont) DeleteObject(app->hMonoFont);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    WNDCLASSEXA wc;
    WNDCLASSEXA ed;
    HWND hwnd;
    MSG msg;

    (void)hPrev;
    (void)lpCmd;

    ZeroMemory(&gApp, sizeof(gApp));
    ZeroMemory(&wc, sizeof(wc));
    ZeroMemory(&ed, sizeof(ed));

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = MAIN_CLASS_NAME;

    ed.cbSize = sizeof(ed);
    ed.lpfnWndProc = EditProc;
    ed.hInstance = hInst;
    ed.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    ed.hCursor = LoadCursor(NULL, IDC_ARROW);
    ed.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    ed.lpszClassName = EDIT_CLASS_NAME;

    RegisterClassExA(&wc);
    RegisterClassExA(&ed);

    hwnd = CreateWindowExA(0,
                           MAIN_CLASS_NAME,
                           APP_TITLE,
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           820,
                           560,
                           NULL,
                           NULL,
                           hInst,
                           &gApp);
    if (!hwnd) return 1;

    center_window(hwnd, NULL);
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
