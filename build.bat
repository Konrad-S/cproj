@echo off
@if not exist "build" mkdir "build"
set sources=source\main.cpp dependencies\glad\src\glad.c
set compiler_options=/MD /Zi /DEBUG:FULL -nologo
set includes=/I"dependencies\glad\include" /I"dependencies/glfw-3.4/include"
set linker_options= /link /NOIMPLIB /INCREMENTAL:NO
set libs=dependencies\glfw-3.4.bin.WIN64\lib-vc2022\glfw3.lib opengl32.lib user32.lib gdi32.lib shell32.lib
cl -DCPROJ_SLOW=1 /LD %compiler_options% source\game.cpp /DBUILD_DLL /Fo:build\ /Fe:build\game.dll %linker_options% 
if %ERRORLEVEL% neq 0 exit /b
cl -DCPROJ_SLOW=1 %sources% %compiler_options% %includes% /Fo:build\ /Fe:build\Cproj.exe %linker_options% %libs%
del vc140.pdb