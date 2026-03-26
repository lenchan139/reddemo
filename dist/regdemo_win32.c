#define UNICODE
#define _UNICODE

#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <strsafe.h>

#ifndef HKEY_DYN_DATA
#define HKEY_DYN_DATA ((HKEY)(ULONG_PTR)((LONG)0x80000006))
#endif

#define APP_CLASS_NAME L"RegDemoWin32Main"
#define EDIT_CLASS_NAME L"RegDemoWin32Edit"

#define IDC_TREE_LIST 1001
#define IDC_VALUE_LIST 1002
#define IDC_EXIT_BUTTON 1003
#define IDC_LABEL_KEY 1004
#define IDC_LABEL_VALUE 1005
#define IDC_LABEL_KEY_HINT 1006
#define IDC_LABEL_VALUE_HINT 1007
#define IDC_LABEL_PLUS 1008
#define IDC_LABEL_DENIED 1009

#define IDC_EDIT_NAME 2001
#define IDC_EDIT_VALUE 2002
#define IDC_EDIT_OK 2003
#define IDC_EDIT_CANCEL 2004
#define IDC_EDIT_BASE_HEX 2005
#define IDC_EDIT_BASE_DEC 2006
#define IDC_EDIT_BASE_GROUP 2007
#define IDC_EDIT_NAME_LABEL 2008
#define IDC_EDIT_VALUE_LABEL 2009

#define LIST_INDENT_SPACES 2
#define MAX_DISPLAY_DATA_BYTES 20000

typedef struct TreeItem {
    HKEY rootKey;
    wchar_t *subKey;
    wchar_t *leafName;
    int level;
    BOOL hasChildren;
    BOOL accessDenied;
    BOOL isRoot;
} TreeItem;

typedef struct ValueItem {
    wchar_t *valueName;
    wchar_t *displayName;
    wchar_t *displayValue;
    wchar_t *editValue;
    DWORD valueType;
    BOOL editable;
} ValueItem;

typedef struct AppState {
    HWND hwnd;
    HWND treeList;
    HWND valueList;
    HWND exitButton;
    HWND labelKey;
    HWND labelValue;
    HWND labelKeyHint;
    HWND labelValueHint;
    HWND labelPlus;
    HWND labelDenied;
    HFONT uiFont;
    HFONT monoFont;
    HCURSOR waitCursor;
} AppState;

typedef struct EditDialogState {
    HWND hwnd;
    HWND parent;
    HFONT uiFont;
    HKEY rootKey;
    const wchar_t *subKey;
    const wchar_t *valueName;
    DWORD valueType;
    const wchar_t *initialValue;
    BOOL accepted;
    BOOL done;
    int selectedBase;
    wchar_t *resultText;
    DWORD resultDword;
} EditDialogState;

enum {
    DWORD_BASE_HEX = 0,
    DWORD_BASE_DEC = 1
};

static wchar_t *dup_string(const wchar_t *text) {
    size_t length;
    wchar_t *copy;

    if (text == NULL) {
        copy = (wchar_t *)calloc(1, sizeof(wchar_t));
        return copy;
    }

    length = wcslen(text);
    copy = (wchar_t *)calloc(length + 1, sizeof(wchar_t));
    if (copy != NULL) {
        memcpy(copy, text, (length + 1) * sizeof(wchar_t));
    }
    return copy;
}

static void free_tree_item(TreeItem *item) {
    if (item == NULL) {
        return;
    }
    free(item->subKey);
    free(item->leafName);
    free(item);
}

static void free_value_item(ValueItem *item) {
    if (item == NULL) {
        return;
    }
    free(item->valueName);
    free(item->displayName);
    free(item->displayValue);
    free(item->editValue);
    free(item);
}

static void free_tree_item_ptr(void *item) {
    free_tree_item((TreeItem *)item);
}

static void free_value_item_ptr(void *item) {
    free_value_item((ValueItem *)item);
}

static const wchar_t *root_key_name(HKEY key) {
    if (key == HKEY_CLASSES_ROOT) {
        return L"HKEY_CLASSES_ROOT";
    }
    if (key == HKEY_CURRENT_USER) {
        return L"HKEY_CURRENT_USER";
    }
    if (key == HKEY_LOCAL_MACHINE) {
        return L"HKEY_LOCAL_MACHINE";
    }
    if (key == HKEY_USERS) {
        return L"HKEY_USERS";
    }
    if (key == HKEY_CURRENT_CONFIG) {
        return L"HKEY_CURRENT_CONFIG";
    }
    if (key == HKEY_DYN_DATA) {
        return L"HKEY_DYN_DATA";
    }
    return L"HKEY_UNKNOWN";
}

static TreeItem *create_tree_item(HKEY rootKey, const wchar_t *subKey, const wchar_t *leafName, int level, BOOL hasChildren, BOOL accessDenied, BOOL isRoot) {
    TreeItem *item = (TreeItem *)calloc(1, sizeof(TreeItem));
    if (item == NULL) {
        return NULL;
    }

    item->rootKey = rootKey;
    item->subKey = dup_string(subKey != NULL ? subKey : L"");
    item->leafName = dup_string(leafName != NULL ? leafName : L"");
    item->level = level;
    item->hasChildren = hasChildren;
    item->accessDenied = accessDenied;
    item->isRoot = isRoot;

    if (item->subKey == NULL || item->leafName == NULL) {
        free_tree_item(item);
        return NULL;
    }

    return item;
}

static ValueItem *create_value_item(const wchar_t *valueName, const wchar_t *displayName, const wchar_t *displayValue, const wchar_t *editValue, DWORD valueType, BOOL editable) {
    ValueItem *item = (ValueItem *)calloc(1, sizeof(ValueItem));
    if (item == NULL) {
        return NULL;
    }

    item->valueName = dup_string(valueName != NULL ? valueName : L"");
    item->displayName = dup_string(displayName != NULL ? displayName : L"");
    item->displayValue = dup_string(displayValue != NULL ? displayValue : L"");
    item->editValue = dup_string(editValue != NULL ? editValue : L"");
    item->valueType = valueType;
    item->editable = editable;

    if (item->valueName == NULL || item->displayName == NULL || item->displayValue == NULL || item->editValue == NULL) {
        free_value_item(item);
        return NULL;
    }

    return item;
}

static void set_control_font(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, (WPARAM)font, TRUE);
}

static void center_window(HWND hwnd, HWND parent) {
    RECT windowRect;
    RECT parentRect;
    int width;
    int height;
    int x;
    int y;

    GetWindowRect(hwnd, &windowRect);
    width = windowRect.right - windowRect.left;
    height = windowRect.bottom - windowRect.top;

    if (parent != NULL && GetWindowRect(parent, &parentRect)) {
        x = parentRect.left + ((parentRect.right - parentRect.left) - width) / 2;
        y = parentRect.top + ((parentRect.bottom - parentRect.top) - height) / 2;
    } else {
        x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    }

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
}

static void clear_listbox_with_data(HWND listBox, void (*freeItem)(void *)) {
    int count;
    int index;

    count = (int)SendMessageW(listBox, LB_GETCOUNT, 0, 0);
    for (index = 0; index < count; ++index) {
        void *item = (void *)SendMessageW(listBox, LB_GETITEMDATA, (WPARAM)index, 0);
        if (item != NULL && item != (void *)LB_ERR && freeItem != NULL) {
            freeItem(item);
        }
    }
    SendMessageW(listBox, LB_RESETCONTENT, 0, 0);
}

static int listbox_text_width(HWND listBox, HFONT font, const wchar_t *text) {
    HDC dc;
    HGDIOBJ oldFont;
    SIZE size;
    int width = 0;

    dc = GetDC(listBox);
    oldFont = SelectObject(dc, font);
    if (GetTextExtentPoint32W(dc, text, (int)wcslen(text), &size)) {
        width = size.cx + 8;
    }
    SelectObject(dc, oldFont);
    ReleaseDC(listBox, dc);
    return width;
}

static void update_horizontal_extent(HWND listBox, HFONT font) {
    int count;
    int index;
    int widest = 0;

    count = (int)SendMessageW(listBox, LB_GETCOUNT, 0, 0);
    for (index = 0; index < count; ++index) {
        int textLength = (int)SendMessageW(listBox, LB_GETTEXTLEN, (WPARAM)index, 0);
        wchar_t *buffer;
        int width;

        if (textLength <= 0) {
            continue;
        }

        buffer = (wchar_t *)calloc((size_t)textLength + 1, sizeof(wchar_t));
        if (buffer == NULL) {
            continue;
        }

        SendMessageW(listBox, LB_GETTEXT, (WPARAM)index, (LPARAM)buffer);
        width = listbox_text_width(listBox, font, buffer);
        if (width > widest) {
            widest = width;
        }
        free(buffer);
    }

    SendMessageW(listBox, LB_SETHORIZONTALEXTENT, (WPARAM)widest, 0);
}

static void show_registry_error(HWND owner, DWORD errorCode) {
    wchar_t message[512];
    DWORD formatResult;

    switch (errorCode) {
        case ERROR_BADDB:
            StringCchCopyW(message, ARRAYSIZE(message), L"The registry database is corrupt.");
            break;
        case ERROR_FILE_NOT_FOUND:
        case ERROR_BADKEY:
            StringCchCopyW(message, ARRAYSIZE(message), L"Bad key name.");
            break;
        case ERROR_CANTOPEN:
            StringCchCopyW(message, ARRAYSIZE(message), L"Can't open key.");
            break;
        case ERROR_CANTREAD:
            StringCchCopyW(message, ARRAYSIZE(message), L"Can't read key.");
            break;
        case ERROR_ACCESS_DENIED:
            StringCchCopyW(message, ARRAYSIZE(message), L"Access to this key is denied.");
            break;
        case ERROR_CANTWRITE:
            StringCchCopyW(message, ARRAYSIZE(message), L"Can't write key.");
            break;
        case ERROR_OUTOFMEMORY:
            StringCchCopyW(message, ARRAYSIZE(message), L"Out of memory.");
            break;
        case ERROR_INVALID_PARAMETER:
            StringCchCopyW(message, ARRAYSIZE(message), L"Invalid parameter.");
            break;
        case ERROR_MORE_DATA:
            StringCchCopyW(message, ARRAYSIZE(message), L"There is more data than the buffer can handle.");
            break;
        default:
            formatResult = FormatMessageW(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                errorCode,
                0,
                message,
                (DWORD)ARRAYSIZE(message),
                NULL);
            if (formatResult == 0) {
                StringCchPrintfW(message, ARRAYSIZE(message), L"Undefined registry error code %lu.", errorCode);
            }
            break;
    }

    MessageBoxW(owner, message, L"RegDemo", MB_ICONERROR | MB_OK);
}

static wchar_t *join_subkey_path(const wchar_t *parent, const wchar_t *leaf) {
    wchar_t *combined;
    size_t parentLength = parent != NULL ? wcslen(parent) : 0;
    size_t leafLength = leaf != NULL ? wcslen(leaf) : 0;
    size_t totalLength = parentLength + leafLength + 2;

    combined = (wchar_t *)calloc(totalLength, sizeof(wchar_t));
    if (combined == NULL) {
        return NULL;
    }

    if (parentLength > 0) {
        StringCchCopyW(combined, totalLength, parent);
        StringCchCatW(combined, totalLength, L"\\");
        if (leaf != NULL) {
            StringCchCatW(combined, totalLength, leaf);
        }
    } else if (leaf != NULL) {
        StringCchCopyW(combined, totalLength, leaf);
    }

    return combined;
}

static void format_tree_label(const TreeItem *item, wchar_t *buffer, size_t bufferCount) {
    int indent;
    size_t position = 0;
    int index;

    buffer[0] = L'\0';

    if (item->isRoot) {
        StringCchCopyW(buffer, bufferCount, root_key_name(item->rootKey));
        return;
    }

    indent = item->level * LIST_INDENT_SPACES;
    for (index = 0; index < indent && position + 1 < bufferCount; ++index) {
        buffer[position++] = L' ';
    }

    if (item->accessDenied && position + 1 < bufferCount) {
        buffer[position++] = L'*';
    } else if (item->hasChildren && position + 1 < bufferCount) {
        buffer[position++] = L'+';
    }

    if (position < bufferCount) {
        buffer[position] = L'\0';
    }

    StringCchCatW(buffer, bufferCount, item->leafName);
}

static void insert_tree_item(HWND listBox, TreeItem *item, int insertIndex, HFONT font) {
    wchar_t label[1024];
    LRESULT finalIndex;

    format_tree_label(item, label, ARRAYSIZE(label));

    if (insertIndex < 0) {
        finalIndex = SendMessageW(listBox, LB_ADDSTRING, 0, (LPARAM)label);
    } else {
        finalIndex = SendMessageW(listBox, LB_INSERTSTRING, (WPARAM)insertIndex, (LPARAM)label);
    }

    if (finalIndex != LB_ERR && finalIndex != LB_ERRSPACE) {
        SendMessageW(listBox, LB_SETITEMDATA, (WPARAM)finalIndex, (LPARAM)item);
        update_horizontal_extent(listBox, font);
    } else {
        free_tree_item(item);
    }
}

static void add_value_item(HWND listBox, ValueItem *item, HFONT font) {
    wchar_t row[4096];
    size_t nameLength = wcslen(item->displayName);
    size_t pad;
    LRESULT index;

    pad = nameLength < 1 ? 1 : 1;
    StringCchPrintfW(row, ARRAYSIZE(row), L"%ls%*ls%ls", item->displayName, (int)pad, L"", item->displayValue);
    index = SendMessageW(listBox, LB_ADDSTRING, 0, (LPARAM)row);
    if (index != LB_ERR && index != LB_ERRSPACE) {
        SendMessageW(listBox, LB_SETITEMDATA, (WPARAM)index, (LPARAM)item);
        update_horizontal_extent(listBox, font);
    } else {
        free_value_item(item);
    }
}

static const wchar_t *registry_type_name(DWORD valueType) {
    switch (valueType) {
        case REG_NONE:
            return L"REG_NONE";
        case REG_SZ:
            return L"REG_SZ";
        case REG_EXPAND_SZ:
            return L"REG_EXPAND_SZ";
        case REG_BINARY:
            return L"REG_BINARY";
        case REG_DWORD:
            return L"REG_DWORD";
        case REG_DWORD_BIG_ENDIAN:
            return L"REG_DWORD_BIG_ENDIAN";
        case REG_LINK:
            return L"REG_LINK";
        case REG_MULTI_SZ:
            return L"REG_MULTI_SZ";
        case REG_RESOURCE_LIST:
            return L"REG_RESOURCE_LIST";
        case REG_FULL_RESOURCE_DESCRIPTOR:
            return L"REG_FULL_RESOURCE_DESCRIPTOR";
        case REG_RESOURCE_REQUIREMENTS_LIST:
            return L"REG_RESOURCE_REQUIREMENTS_LIST";
        case REG_QWORD:
            return L"REG_QWORD";
        default:
            return L"REG_UNKNOWN";
    }
}

static wchar_t *format_binary_data(const BYTE *data, DWORD dataSize) {
    wchar_t *buffer;
    size_t charsNeeded;
    DWORD index;
    wchar_t *writeCursor;

    if (dataSize == 0) {
        return dup_string(L"(value not set)");
    }

    charsNeeded = (size_t)dataSize * 3 + 1;
    buffer = (wchar_t *)calloc(charsNeeded, sizeof(wchar_t));
    if (buffer == NULL) {
        return NULL;
    }

    writeCursor = buffer;
    for (index = 0; index < dataSize; ++index) {
        StringCchPrintfW(writeCursor, charsNeeded - (size_t)(writeCursor - buffer), L"%02X ", data[index]);
        writeCursor += wcslen(writeCursor);
    }

    return buffer;
}

static wchar_t *format_multi_sz(const wchar_t *data, DWORD dataSizeBytes) {
    size_t count = dataSizeBytes / sizeof(wchar_t);
    wchar_t *buffer;
    size_t index;

    if (count == 0) {
        return dup_string(L"(value not set)");
    }

    buffer = (wchar_t *)calloc(count + 1, sizeof(wchar_t));
    if (buffer == NULL) {
        return NULL;
    }

    for (index = 0; index < count; ++index) {
        buffer[index] = data[index] == L'\0' ? L' ' : data[index];
    }
    buffer[count] = L'\0';
    return buffer;
}

static wchar_t *format_sz_value(const wchar_t *data, DWORD dataSizeBytes) {
    size_t charCount = dataSizeBytes / sizeof(wchar_t);
    size_t end = charCount;
    wchar_t *buffer;

    while (end > 0 && data[end - 1] == L'\0') {
        --end;
    }

    buffer = (wchar_t *)calloc(end + 3, sizeof(wchar_t));
    if (buffer == NULL) {
        return NULL;
    }

    buffer[0] = L'"';
    if (end > 0) {
        memcpy(buffer + 1, data, end * sizeof(wchar_t));
    }
    buffer[end + 1] = L'"';
    buffer[end + 2] = L'\0';
    return buffer;
}

static wchar_t *extract_sz_edit_value(const wchar_t *data, DWORD dataSizeBytes) {
    size_t charCount = dataSizeBytes / sizeof(wchar_t);
    size_t end = charCount;
    wchar_t *buffer;

    while (end > 0 && data[end - 1] == L'\0') {
        --end;
    }

    buffer = (wchar_t *)calloc(end + 1, sizeof(wchar_t));
    if (buffer == NULL) {
        return NULL;
    }

    if (end > 0) {
        memcpy(buffer, data, end * sizeof(wchar_t));
    }
    buffer[end] = L'\0';
    return buffer;
}

static wchar_t *format_dword_value(DWORD value) {
    wchar_t *buffer = (wchar_t *)calloc(64, sizeof(wchar_t));
    if (buffer != NULL) {
        StringCchPrintfW(buffer, 64, L"&H%08X (%lu)", value, (unsigned long)value);
    }
    return buffer;
}

static wchar_t *format_dword_edit_value(DWORD value) {
    wchar_t *buffer = (wchar_t *)calloc(16, sizeof(wchar_t));
    if (buffer != NULL) {
        StringCchPrintfW(buffer, 16, L"%08X", value);
    }
    return buffer;
}

static BOOL query_subkey_flags(HKEY rootKey, const wchar_t *subKey, BOOL *hasChildren, BOOL *accessDenied) {
    HKEY openedKey;
    LONG result;
    DWORD subKeyCount = 0;

    *hasChildren = FALSE;
    *accessDenied = FALSE;

    result = RegOpenKeyExW(rootKey, (subKey != NULL && subKey[0] != L'\0') ? subKey : NULL, 0, KEY_READ, &openedKey);
    if (result == ERROR_ACCESS_DENIED) {
        *accessDenied = TRUE;
        return TRUE;
    }
    if (result != ERROR_SUCCESS) {
        return FALSE;
    }

    result = RegQueryInfoKeyW(openedKey, NULL, NULL, NULL, &subKeyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result == ERROR_SUCCESS) {
        *hasChildren = subKeyCount > 0;
    }

    RegCloseKey(openedKey);
    return result == ERROR_SUCCESS;
}

static void populate_root_keys(AppState *app) {
    insert_tree_item(app->treeList, create_tree_item(HKEY_CLASSES_ROOT, L"", root_key_name(HKEY_CLASSES_ROOT), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
    insert_tree_item(app->treeList, create_tree_item(HKEY_CURRENT_USER, L"", root_key_name(HKEY_CURRENT_USER), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
    insert_tree_item(app->treeList, create_tree_item(HKEY_LOCAL_MACHINE, L"", root_key_name(HKEY_LOCAL_MACHINE), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
    insert_tree_item(app->treeList, create_tree_item(HKEY_USERS, L"", root_key_name(HKEY_USERS), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
    insert_tree_item(app->treeList, create_tree_item(HKEY_CURRENT_CONFIG, L"", root_key_name(HKEY_CURRENT_CONFIG), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
    insert_tree_item(app->treeList, create_tree_item(HKEY_DYN_DATA, L"", root_key_name(HKEY_DYN_DATA), 0, TRUE, FALSE, TRUE), -1, app->monoFont);
}

static void enumerate_values(AppState *app, const TreeItem *treeItem) {
    HKEY openedKey;
    LONG result;
    DWORD index = 0;
    DWORD maxValueName = 0;
    DWORD maxValueData = 0;

    clear_listbox_with_data(app->valueList, free_value_item_ptr);

    result = RegOpenKeyExW(treeItem->rootKey, (treeItem->subKey != NULL && treeItem->subKey[0] != L'\0') ? treeItem->subKey : NULL, 0, KEY_READ, &openedKey);
    if (result != ERROR_SUCCESS) {
        show_registry_error(app->hwnd, (DWORD)result);
        return;
    }

    result = RegQueryInfoKeyW(openedKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &maxValueName, &maxValueData, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(openedKey);
        show_registry_error(app->hwnd, (DWORD)result);
        return;
    }

    while (1) {
        DWORD nameCapacity = maxValueName + 2;
        DWORD dataCapacity = maxValueData + 2;
        wchar_t *nameBuffer = (wchar_t *)calloc(nameCapacity, sizeof(wchar_t));
        BYTE *dataBuffer = (BYTE *)calloc(dataCapacity > 0 ? dataCapacity : 2, sizeof(BYTE));
        DWORD nameLength = maxValueName + 1;
        DWORD dataLength = maxValueData + 1;
        DWORD valueType = 0;

        if (nameBuffer == NULL || dataBuffer == NULL) {
            free(nameBuffer);
            free(dataBuffer);
            MessageBoxW(app->hwnd, L"Out of memory while reading registry values.", L"RegDemo", MB_OK | MB_ICONERROR);
            break;
        }

        result = RegEnumValueW(openedKey, index, nameBuffer, &nameLength, NULL, &valueType, dataBuffer, &dataLength);
        if (result == ERROR_MORE_DATA) {
            maxValueData += 256;
            maxValueName += 64;
            free(nameBuffer);
            free(dataBuffer);
            continue;
        }
        if (result == ERROR_NO_MORE_ITEMS) {
            free(nameBuffer);
            free(dataBuffer);
            break;
        }
        if (result != ERROR_SUCCESS) {
            free(nameBuffer);
            free(dataBuffer);
            show_registry_error(app->hwnd, (DWORD)result);
            break;
        }

        nameBuffer[nameLength] = L'\0';

        {
            const wchar_t *displayName = nameLength == 0 ? L"(Default)" : nameBuffer;
            wchar_t *displayValue = NULL;
            wchar_t *editValue = dup_string(L"");
            BOOL editable = FALSE;
            ValueItem *item;

            if (dataLength > MAX_DISPLAY_DATA_BYTES) {
                displayValue = dup_string(L"(Value too large to display)");
            } else {
                switch (valueType) {
                    case REG_SZ:
                        displayValue = format_sz_value((const wchar_t *)dataBuffer, dataLength);
                        free(editValue);
                        editValue = extract_sz_edit_value((const wchar_t *)dataBuffer, dataLength);
                        editable = TRUE;
                        break;
                    case REG_DWORD:
                        if (dataLength >= sizeof(DWORD)) {
                            DWORD dwordValue = *(DWORD *)dataBuffer;
                            displayValue = format_dword_value(dwordValue);
                            free(editValue);
                            editValue = format_dword_edit_value(dwordValue);
                            editable = TRUE;
                        } else {
                            displayValue = dup_string(L"(Invalid DWORD data)");
                        }
                        break;
                    case REG_MULTI_SZ:
                        displayValue = format_multi_sz((const wchar_t *)dataBuffer, dataLength);
                        break;
                    case REG_BINARY:
                        displayValue = format_binary_data(dataBuffer, dataLength);
                        break;
                    case REG_FULL_RESOURCE_DESCRIPTOR:
                        displayValue = dup_string(L"REG_FULL_RESOURCE_DESCRIPTOR");
                        break;
                    default:
                    {
                        wchar_t typeBuffer[64];
                        StringCchPrintfW(typeBuffer, ARRAYSIZE(typeBuffer), L"%ls", registry_type_name(valueType));
                        displayValue = dup_string(typeBuffer);
                        break;
                    }
                }
            }

            if (displayValue == NULL) {
                displayValue = dup_string(L"(value not set)");
            }
            if (editValue == NULL) {
                editValue = dup_string(L"");
            }
            if (wcslen(displayValue) == 0) {
                free(displayValue);
                displayValue = dup_string(L"(value not set)");
            }

            item = create_value_item(nameBuffer, displayName, displayValue, editValue, valueType, editable);
            if (item != NULL) {
                add_value_item(app->valueList, item, app->monoFont);
            }

            free(displayValue);
            free(editValue);
        }

        free(nameBuffer);
        free(dataBuffer);
        ++index;
    }

    RegCloseKey(openedKey);
}

static void enumerate_subkeys(AppState *app, const TreeItem *parentItem, int insertIndex) {
    HKEY openedKey;
    LONG result;
    DWORD maxSubKeyName = 0;
    DWORD subKeyCount = 0;
    DWORD index = 0;
    HCURSOR oldCursor;

    oldCursor = SetCursor(app->waitCursor);
    result = RegOpenKeyExW(parentItem->rootKey, (parentItem->subKey != NULL && parentItem->subKey[0] != L'\0') ? parentItem->subKey : NULL, 0, KEY_READ, &openedKey);
    if (result != ERROR_SUCCESS) {
        SetCursor(oldCursor);
        show_registry_error(app->hwnd, (DWORD)result);
        return;
    }

    result = RegQueryInfoKeyW(openedKey, NULL, NULL, NULL, &subKeyCount, &maxSubKeyName, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(openedKey);
        SetCursor(oldCursor);
        show_registry_error(app->hwnd, (DWORD)result);
        return;
    }

    for (index = 0; index < subKeyCount; ++index) {
        DWORD nameLength = maxSubKeyName + 1;
        wchar_t *nameBuffer = (wchar_t *)calloc(nameLength + 1, sizeof(wchar_t));

        if (nameBuffer == NULL) {
            MessageBoxW(app->hwnd, L"Out of memory while reading registry subkeys.", L"RegDemo", MB_OK | MB_ICONERROR);
            break;
        }

        result = RegEnumKeyExW(openedKey, index, nameBuffer, &nameLength, NULL, NULL, NULL, NULL);
        if (result == ERROR_MORE_DATA) {
            free(nameBuffer);
            continue;
        }
        if (result != ERROR_SUCCESS) {
            free(nameBuffer);
            show_registry_error(app->hwnd, (DWORD)result);
            break;
        }

        nameBuffer[nameLength] = L'\0';

        {
            wchar_t *fullPath = join_subkey_path(parentItem->subKey, nameBuffer);
            BOOL hasChildren = FALSE;
            BOOL accessDenied = FALSE;
            TreeItem *childItem;

            if (fullPath == NULL) {
                free(nameBuffer);
                MessageBoxW(app->hwnd, L"Out of memory while building a registry path.", L"RegDemo", MB_OK | MB_ICONERROR);
                break;
            }

            query_subkey_flags(parentItem->rootKey, fullPath, &hasChildren, &accessDenied);
            childItem = create_tree_item(parentItem->rootKey, fullPath, nameBuffer, parentItem->level + 1, hasChildren, accessDenied, FALSE);
            if (childItem != NULL) {
                insert_tree_item(app->treeList, childItem, insertIndex++, app->monoFont);
            }

            free(fullPath);
        }

        free(nameBuffer);
    }

    RegCloseKey(openedKey);
    SetCursor(oldCursor);
}

static void collapse_tree_children(AppState *app, int parentIndex, int parentLevel) {
    int count = (int)SendMessageW(app->treeList, LB_GETCOUNT, 0, 0);
    int index = parentIndex + 1;

    while (index < count) {
        TreeItem *item = (TreeItem *)SendMessageW(app->treeList, LB_GETITEMDATA, (WPARAM)index, 0);
        if (item == NULL || item == (TreeItem *)LB_ERR || item->level <= parentLevel) {
            break;
        }

        free_tree_item(item);
        SendMessageW(app->treeList, LB_DELETESTRING, (WPARAM)index, 0);
        count = (int)SendMessageW(app->treeList, LB_GETCOUNT, 0, 0);
    }

    update_horizontal_extent(app->treeList, app->monoFont);
}

static BOOL parse_uint32(const wchar_t *text, int base, DWORD *valueOut) {
    wchar_t *endPtr;
    unsigned long long parsed;
    const wchar_t *cursor = text;

    while (*cursor == L' ' || *cursor == L'\t') {
        ++cursor;
    }

    if (base == DWORD_BASE_HEX) {
        if ((cursor[0] == L'&' && (cursor[1] == L'h' || cursor[1] == L'H')) || (cursor[0] == L'0' && (cursor[1] == L'x' || cursor[1] == L'X'))) {
            cursor += 2;
        }
        parsed = wcstoull(cursor, &endPtr, 16);
    } else {
        parsed = wcstoull(cursor, &endPtr, 10);
    }

    while (*endPtr == L' ' || *endPtr == L'\t') {
        ++endPtr;
    }

    if (cursor == endPtr || *endPtr != L'\0' || parsed > 0xFFFFFFFFULL) {
        return FALSE;
    }

    *valueOut = (DWORD)parsed;
    return TRUE;
}

static void set_edit_text_u32(HWND editControl, DWORD value, int base) {
    wchar_t buffer[64];

    if (base == DWORD_BASE_HEX) {
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%08X", value);
    } else {
        StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"%lu", (unsigned long)value);
    }
    SetWindowTextW(editControl, buffer);
    SendMessageW(editControl, EM_SETSEL, 0, -1);
}

static void edit_dialog_switch_base(EditDialogState *state, int newBase) {
    wchar_t buffer[256];
    DWORD parsedValue;
    HWND valueEdit = GetDlgItem(state->hwnd, IDC_EDIT_VALUE);

    if (state->selectedBase == newBase) {
        return;
    }

    GetWindowTextW(valueEdit, buffer, ARRAYSIZE(buffer));
    if (parse_uint32(buffer, state->selectedBase, &parsedValue)) {
        set_edit_text_u32(valueEdit, parsedValue, newBase);
    }
    state->selectedBase = newBase;
}

static LRESULT CALLBACK EditWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    EditDialogState *state = (EditDialogState *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case WM_NCCREATE:
        {
            CREATESTRUCTW *create = (CREATESTRUCTW *)lParam;
            state = (EditDialogState *)create->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)state);
            state->hwnd = hwnd;
            return TRUE;
        }

        case WM_CREATE:
        {
            HWND control;
            BOOL isDword = state->valueType == REG_DWORD;

            control = CreateWindowExW(0, L"STATIC", L"Value Name:", WS_CHILD | WS_VISIBLE, 20, 16, 120, 20, hwnd, (HMENU)IDC_EDIT_NAME_LABEL, NULL, NULL);
            set_control_font(control, state->uiFont);

            control = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->valueName != NULL && state->valueName[0] != L'\0' ? state->valueName : L"(Default)", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 20, 38, 500, 24, hwnd, (HMENU)IDC_EDIT_NAME, NULL, NULL);
            set_control_font(control, state->uiFont);

            control = CreateWindowExW(0, L"STATIC", L"Value Data:", WS_CHILD | WS_VISIBLE, 20, 78, 120, 20, hwnd, (HMENU)IDC_EDIT_VALUE_LABEL, NULL, NULL);
            set_control_font(control, state->uiFont);

            control = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->initialValue != NULL ? state->initialValue : L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 100, 500, 24, hwnd, (HMENU)IDC_EDIT_VALUE, NULL, NULL);
            set_control_font(control, state->uiFont);

            control = CreateWindowExW(0, L"BUTTON", L"Base", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | (isDword ? 0 : WS_DISABLED), 20, 136, 220, 74, hwnd, (HMENU)IDC_EDIT_BASE_GROUP, NULL, NULL);
            set_control_font(control, state->uiFont);
            ShowWindow(control, isDword ? SW_SHOW : SW_HIDE);

            control = CreateWindowExW(0, L"BUTTON", L"Hexadecimal", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 38, 158, 160, 20, hwnd, (HMENU)IDC_EDIT_BASE_HEX, NULL, NULL);
            set_control_font(control, state->uiFont);
            SendMessageW(control, BM_SETCHECK, BST_CHECKED, 0);
            ShowWindow(control, isDword ? SW_SHOW : SW_HIDE);

            control = CreateWindowExW(0, L"BUTTON", L"Decimal", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 38, 182, 160, 20, hwnd, (HMENU)IDC_EDIT_BASE_DEC, NULL, NULL);
            set_control_font(control, state->uiFont);
            ShowWindow(control, isDword ? SW_SHOW : SW_HIDE);

            control = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 320, 176, 90, 28, hwnd, (HMENU)IDC_EDIT_OK, NULL, NULL);
            set_control_font(control, state->uiFont);

            control = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 430, 176, 90, 28, hwnd, (HMENU)IDC_EDIT_CANCEL, NULL, NULL);
            set_control_font(control, state->uiFont);

            SendMessageW(GetDlgItem(hwnd, IDC_EDIT_VALUE), EM_SETSEL, 0, -1);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_EDIT_OK:
                {
                    wchar_t buffer[512];

                    GetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_VALUE), buffer, ARRAYSIZE(buffer));
                    if (state->valueType == REG_DWORD) {
                        DWORD parsedValue;
                        if (!parse_uint32(buffer, state->selectedBase, &parsedValue)) {
                            MessageBoxW(hwnd, L"Enter a valid DWORD value.", L"RegDemo", MB_OK | MB_ICONERROR);
                            SetFocus(GetDlgItem(hwnd, IDC_EDIT_VALUE));
                            return 0;
                        }
                        state->resultDword = parsedValue;
                    } else {
                        free(state->resultText);
                        state->resultText = dup_string(buffer);
                        if (state->resultText == NULL) {
                            MessageBoxW(hwnd, L"Out of memory while storing the edited value.", L"RegDemo", MB_OK | MB_ICONERROR);
                            return 0;
                        }
                    }

                    state->accepted = TRUE;
                    DestroyWindow(hwnd);
                    return 0;
                }

                case IDC_EDIT_CANCEL:
                    DestroyWindow(hwnd);
                    return 0;

                case IDC_EDIT_BASE_HEX:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        edit_dialog_switch_base(state, DWORD_BASE_HEX);
                    }
                    return 0;

                case IDC_EDIT_BASE_DEC:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        edit_dialog_switch_base(state, DWORD_BASE_DEC);
                    }
                    return 0;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            state->done = TRUE;
            EnableWindow(state->parent, TRUE);
            SetActiveWindow(state->parent);
            return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

static BOOL show_edit_dialog(HWND parent, HFONT uiFont, HKEY rootKey, const wchar_t *subKey, ValueItem *item, wchar_t **resultText, DWORD *resultDword) {
    EditDialogState state;
    MSG message;
    HWND dialog;
    RECT parentRect;
    int width = 560;
    int height = item->valueType == REG_DWORD ? 255 : 185;

    ZeroMemory(&state, sizeof(state));
    state.parent = parent;
    state.uiFont = uiFont;
    state.rootKey = rootKey;
    state.subKey = subKey;
    state.valueName = item->valueName;
    state.valueType = item->valueType;
    state.initialValue = item->editValue;
    state.selectedBase = DWORD_BASE_HEX;

    GetWindowRect(parent, &parentRect);
    dialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        EDIT_CLASS_NAME,
        item->valueType == REG_DWORD ? L"Edit DWORD Value" : L"Edit String Value",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        parentRect.left + 60,
        parentRect.top + 60,
        width,
        height,
        parent,
        NULL,
        NULL,
        &state);

    if (dialog == NULL) {
        return FALSE;
    }

    EnableWindow(parent, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    center_window(dialog, parent);

    while (!state.done && GetMessageW(&message, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    if (state.accepted) {
        if (item->valueType == REG_DWORD) {
            *resultDword = state.resultDword;
            *resultText = NULL;
        } else {
            *resultText = state.resultText;
            state.resultText = NULL;
        }
    }

    free(state.resultText);
    return state.accepted;
}

static BOOL save_registry_value(HWND owner, HKEY rootKey, const wchar_t *subKey, ValueItem *item, const wchar_t *stringValue, DWORD dwordValue) {
    HKEY openedKey;
    LONG result;

    result = RegOpenKeyExW(rootKey, (subKey != NULL && subKey[0] != L'\0') ? subKey : NULL, 0, KEY_SET_VALUE, &openedKey);
    if (result != ERROR_SUCCESS) {
        show_registry_error(owner, (DWORD)result);
        return FALSE;
    }

    if (item->valueType == REG_DWORD) {
        result = RegSetValueExW(openedKey, item->valueName[0] != L'\0' ? item->valueName : NULL, 0, REG_DWORD, (const BYTE *)&dwordValue, sizeof(dwordValue));
    } else {
        DWORD sizeBytes = (DWORD)((wcslen(stringValue) + 1) * sizeof(wchar_t));
        result = RegSetValueExW(openedKey, item->valueName[0] != L'\0' ? item->valueName : NULL, 0, REG_SZ, (const BYTE *)stringValue, sizeBytes);
    }

    RegCloseKey(openedKey);

    if (result != ERROR_SUCCESS) {
        show_registry_error(owner, (DWORD)result);
        return FALSE;
    }

    return TRUE;
}

static void refresh_for_selected_tree_item(AppState *app) {
    int selectedIndex = (int)SendMessageW(app->treeList, LB_GETCURSEL, 0, 0);
    if (selectedIndex == LB_ERR) {
        clear_listbox_with_data(app->valueList, free_value_item_ptr);
        return;
    }

    {
        TreeItem *item = (TreeItem *)SendMessageW(app->treeList, LB_GETITEMDATA, (WPARAM)selectedIndex, 0);
        if (item != NULL && item != (TreeItem *)LB_ERR) {
            enumerate_values(app, item);
        }
    }
}

static void edit_selected_value(AppState *app) {
    int treeIndex = (int)SendMessageW(app->treeList, LB_GETCURSEL, 0, 0);
    int valueIndex = (int)SendMessageW(app->valueList, LB_GETCURSEL, 0, 0);
    TreeItem *treeItem;
    ValueItem *valueItem;
    wchar_t *resultText = NULL;
    DWORD resultDword = 0;

    if (treeIndex == LB_ERR || valueIndex == LB_ERR) {
        return;
    }

    treeItem = (TreeItem *)SendMessageW(app->treeList, LB_GETITEMDATA, (WPARAM)treeIndex, 0);
    valueItem = (ValueItem *)SendMessageW(app->valueList, LB_GETITEMDATA, (WPARAM)valueIndex, 0);
    if (treeItem == NULL || treeItem == (TreeItem *)LB_ERR || valueItem == NULL || valueItem == (ValueItem *)LB_ERR) {
        return;
    }

    if (!valueItem->editable) {
        wchar_t message[256];
        StringCchPrintfW(message, ARRAYSIZE(message), L"This port only supports editing REG_SZ and REG_DWORD values. The selected value is %ls.", registry_type_name(valueItem->valueType));
        MessageBoxW(app->hwnd, message, L"RegDemo", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (show_edit_dialog(app->hwnd, app->uiFont, treeItem->rootKey, treeItem->subKey, valueItem, &resultText, &resultDword)) {
        if (save_registry_value(app->hwnd, treeItem->rootKey, treeItem->subKey, valueItem, resultText != NULL ? resultText : L"", resultDword)) {
            enumerate_values(app, treeItem);
            if (valueIndex < (int)SendMessageW(app->valueList, LB_GETCOUNT, 0, 0)) {
                SendMessageW(app->valueList, LB_SETCURSEL, (WPARAM)valueIndex, 0);
            }
        }
    }

    free(resultText);
}

static void toggle_tree_expansion(AppState *app) {
    int selectedIndex = (int)SendMessageW(app->treeList, LB_GETCURSEL, 0, 0);
    TreeItem *item;
    int count;

    if (selectedIndex == LB_ERR) {
        return;
    }

    item = (TreeItem *)SendMessageW(app->treeList, LB_GETITEMDATA, (WPARAM)selectedIndex, 0);
    if (item == NULL || item == (TreeItem *)LB_ERR || item->accessDenied || !item->hasChildren) {
        return;
    }

    count = (int)SendMessageW(app->treeList, LB_GETCOUNT, 0, 0);
    if (selectedIndex + 1 < count) {
        TreeItem *nextItem = (TreeItem *)SendMessageW(app->treeList, LB_GETITEMDATA, (WPARAM)(selectedIndex + 1), 0);
        if (nextItem != NULL && nextItem != (TreeItem *)LB_ERR && nextItem->level > item->level) {
            collapse_tree_children(app, selectedIndex, item->level);
            return;
        }
    }

    enumerate_subkeys(app, item, selectedIndex + 1);
}

static void layout_main_window(AppState *app, int width, int height) {
    int margin = 10;
    int headerTop = 8;
    int listTop = 28;
    int footerHeight = 88;
    int listHeight = height - listTop - footerHeight;
    int leftWidth = (width / 2) - 16;
    int rightWidth = width - leftWidth - (margin * 3);

    if (listHeight < 120) {
        listHeight = 120;
    }
    if (leftWidth < 160) {
        leftWidth = 160;
    }
    if (rightWidth < 180) {
        rightWidth = 180;
    }

    MoveWindow(app->labelKey, margin, headerTop, 120, 20, TRUE);
    MoveWindow(app->labelValue, margin + leftWidth + margin, headerTop, 140, 20, TRUE);
    MoveWindow(app->treeList, margin, listTop, leftWidth, listHeight, TRUE);
    MoveWindow(app->valueList, margin + leftWidth + margin, listTop, rightWidth, listHeight, TRUE);
    MoveWindow(app->labelKeyHint, margin, listTop + listHeight + 10, leftWidth, 18, TRUE);
    MoveWindow(app->labelValueHint, margin + leftWidth + margin, listTop + listHeight + 10, rightWidth, 18, TRUE);
    MoveWindow(app->labelPlus, margin, listTop + listHeight + 34, leftWidth, 18, TRUE);
    MoveWindow(app->labelDenied, margin, listTop + listHeight + 54, leftWidth, 18, TRUE);
    MoveWindow(app->exitButton, width - 110, listTop + listHeight + 30, 90, 30, TRUE);
}

static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppState *app = (AppState *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (message) {
        case WM_NCCREATE:
        {
            CREATESTRUCTW *create = (CREATESTRUCTW *)lParam;
            app = (AppState *)create->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app);
            app->hwnd = hwnd;
            return TRUE;
        }

        case WM_CREATE:
        {
            NONCLIENTMETRICSW metrics;
            HDC screenDc;
            int dpi;

            metrics.cbSize = sizeof(metrics);
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0);
            app->uiFont = CreateFontIndirectW(&metrics.lfMessageFont);

            screenDc = GetDC(hwnd);
            dpi = GetDeviceCaps(screenDc, LOGPIXELSY);
            ReleaseDC(hwnd, screenDc);
            app->monoFont = CreateFontW(-MulDiv(9, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            if (app->monoFont == NULL) {
                app->monoFont = CreateFontW(-MulDiv(9, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
            }

            app->waitCursor = LoadCursorW(NULL, IDC_WAIT);

            app->labelKey = CreateWindowExW(0, L"STATIC", L"Key Name", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_KEY, NULL, NULL);
            app->labelValue = CreateWindowExW(0, L"STATIC", L"Value Data", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_VALUE, NULL, NULL);
            app->treeList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hwnd, (HMENU)IDC_TREE_LIST, NULL, NULL);
            app->valueList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hwnd, (HMENU)IDC_VALUE_LIST, NULL, NULL);
            app->labelKeyHint = CreateWindowExW(0, L"STATIC", L"Double-click a key to expand it", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_KEY_HINT, NULL, NULL);
            app->labelValueHint = CreateWindowExW(0, L"STATIC", L"Double-click a value to edit it", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_VALUE_HINT, NULL, NULL);
            app->labelPlus = CreateWindowExW(0, L"STATIC", L"+ means additional keys", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_PLUS, NULL, NULL);
            app->labelDenied = CreateWindowExW(0, L"STATIC", L"* means key cannot be opened", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)IDC_LABEL_DENIED, NULL, NULL);
            app->exitButton = CreateWindowExW(0, L"BUTTON", L"Exit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hwnd, (HMENU)IDC_EXIT_BUTTON, NULL, NULL);

            set_control_font(app->labelKey, app->uiFont);
            set_control_font(app->labelValue, app->uiFont);
            set_control_font(app->labelKeyHint, app->uiFont);
            set_control_font(app->labelValueHint, app->uiFont);
            set_control_font(app->labelPlus, app->uiFont);
            set_control_font(app->labelDenied, app->uiFont);
            set_control_font(app->exitButton, app->uiFont);
            set_control_font(app->treeList, app->monoFont);
            set_control_font(app->valueList, app->monoFont);

            populate_root_keys(app);
            return 0;
        }

        case WM_SIZE:
            if (app != NULL) {
                layout_main_window(app, LOWORD(lParam), HIWORD(lParam));
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_EXIT_BUTTON:
                    DestroyWindow(hwnd);
                    return 0;

                case IDC_TREE_LIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        refresh_for_selected_tree_item(app);
                    } else if (HIWORD(wParam) == LBN_DBLCLK) {
                        refresh_for_selected_tree_item(app);
                        toggle_tree_expansion(app);
                    }
                    return 0;

                case IDC_VALUE_LIST:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        edit_selected_value(app);
                    }
                    return 0;
            }
            break;

        case WM_DESTROY:
            if (app != NULL) {
                clear_listbox_with_data(app->treeList, free_tree_item_ptr);
                clear_listbox_with_data(app->valueList, free_value_item_ptr);
                DeleteObject(app->uiFont);
                DeleteObject(app->monoFont);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int showCommand) {
    WNDCLASSEXW mainClass;
    WNDCLASSEXW editClass;
    AppState app;
    HWND hwnd;
    MSG message;

    (void)previousInstance;
    (void)commandLine;

    ZeroMemory(&app, sizeof(app));
    ZeroMemory(&mainClass, sizeof(mainClass));
    ZeroMemory(&editClass, sizeof(editClass));

    mainClass.cbSize = sizeof(mainClass);
    mainClass.lpfnWndProc = MainWindowProc;
    mainClass.hInstance = instance;
    mainClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    mainClass.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    mainClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    mainClass.lpszClassName = APP_CLASS_NAME;

    editClass.cbSize = sizeof(editClass);
    editClass.lpfnWndProc = EditWindowProc;
    editClass.hInstance = instance;
    editClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    editClass.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    editClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    editClass.lpszClassName = EDIT_CLASS_NAME;

    RegisterClassExW(&mainClass);
    RegisterClassExW(&editClass);

    hwnd = CreateWindowExW(
        0,
        APP_CLASS_NAME,
        L"Win32 RegDemo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        820,
        560,
        NULL,
        NULL,
        instance,
        &app);

    if (hwnd == NULL) {
        return 1;
    }

    center_window(hwnd, NULL);
    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}