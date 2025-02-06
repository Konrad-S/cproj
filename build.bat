@echo off
@if not exist "build" mkdir "build"
set sources=source\main.cpp dependencies\glad\src\glad.c
set compiler_options=/MD
set includes=/I"dependencies\glad\include" /I"dependencies/glfw-3.4/include"
set linker_options=/Fo:build\ /Fe:build\Cproj.exe
set libs=dependencies\glfw-3.4.bin.WIN64\lib-vc2022\glfw3.lib opengl32.lib user32.lib gdi32.lib shell32.lib
cl %sources% %compiler_options% %includes% %linker_options% /link %libs%