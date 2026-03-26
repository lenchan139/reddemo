# Win32 C Port

This folder contains a Win32 C port of the original VB6 registry demo.

Files:

- `regdemo_win32.c`: standalone Win32 application source.
- `build.bat`: simple Windows build script for MSVC or MinGW.

Behavior implemented:

- Two-list registry browser UI.
- Expand and collapse subkeys by double-clicking.
- Enumerate registry values for the selected key.
- Edit `REG_SZ` and `REG_DWORD` values in a modal editor.
- Display unsupported or oversized value types without editing them.

Build on Windows:

1. Open a Developer Command Prompt for Visual Studio, or a shell with MinGW `gcc` in `PATH`.
2. Run `build.bat` from this directory.

Notes:

- The current workspace is on macOS and does not have a Windows cross-compiler installed, so no `.exe` was produced here.
- `HKEY_DYN_DATA` is kept for parity with the original sample, but that hive is obsolete on modern Windows.