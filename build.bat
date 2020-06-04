@echo off

set OUT_DIR=Debug
set OUT_EXE=sdl2_opengl

set SDL2_DIR=libs\SDL2-2.0.12
set OPENGL_DIR=libs\glad

set INCLUDES=/I%SDL2_DIR%\include /I%OPENGL_DIR%\include
set SOURCES=src\main.c %OPENGL_DIR%\src\glad.c
set LIBS=/libpath:%SDL2_DIR%\lib\x86 shell32.lib SDL2.lib SDL2main.lib

if not exist %OUT_DIR% mkdir %OUT_DIR%

cl /nologo /Zi /MD %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console
