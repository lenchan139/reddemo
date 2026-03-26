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
