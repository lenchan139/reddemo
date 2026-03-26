# Win32 C Port (VB4 Conversion)

This folder contains a C conversion of the VB4 RegDemo project using the Win32 API.

## Files

- `regdemo_win32.c`: standalone Win32 application source.
- `build.bat`: Windows build script for MSVC or MinGW.

## Behavior Parity

- Main form with two list boxes (`Key Name` and `Value Data`) and matching labels/hints.
- Key expansion/collapse by double-click in the left list.
- Registry value enumeration for the selected key in the right list.
- Modal edit window for `REG_SZ` and `REG_DWORD` (with Hex/Decimal base switching for DWORD).
- Root key population behavior matching the VB4 logic (`HKEY_CURRENT_CONFIG` and `HKEY_DYN_DATA` only on non-NT path).

## Build on Windows

1. Open a Visual Studio Developer Command Prompt, or a shell with MinGW `gcc` in `PATH`.
2. Run `build.bat` in this folder.

## Notes

- This workspace is on macOS, so no Windows executable was built here.
- `HKEY_DYN_DATA` is obsolete on modern Windows; it is retained only for historical parity with the original sample.