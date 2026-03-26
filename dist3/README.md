# RegDemo Win32 C Port (NT4 Baseline)

This folder contains a Win32 C rewrite of the original VB RegDemo project.

## Files

- regdemo_win32_nt4.c
- build.bat

## What Is Ported

- Main window with two list boxes and matching labels/hints.
- Root key population behavior matching the original VB logic:
  - Always: HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_USERS
  - Win9x only: HKEY_CURRENT_CONFIG, HKEY_DYN_DATA
- Double-click left list to expand/collapse subkeys.
- Select key to enumerate values on right list.
- Value display formatting for REG_SZ, REG_DWORD, REG_MULTI_SZ, REG_BINARY, and others.
- Modal editor for REG_SZ and REG_DWORD values.
- DWORD edit supports Hexadecimal/Decimal base switching.

## NT4 Compatibility Notes

- Uses classic Win32 API and ANSI registry APIs (RegOpenKeyExA, RegEnumValueA, etc.).
- Avoids modern framework/runtime dependencies.
- Uses only user32/gdi32/advapi32 at link time.

## Build On Windows

1. Open a Visual Studio Developer Command Prompt or MinGW shell.
2. Run build.bat in this directory.
3. Launch regdemo_win32_nt4.exe.

## Caution

Editing the registry can break system behavior. Only modify values when you know the expected data.
