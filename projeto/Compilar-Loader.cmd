@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>&1
if errorlevel 1 (
  echo Abra este script no Developer Command Prompt x64 do Visual Studio.
  exit /b 1
)
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MT /W4 /I "vendor\imgui" /I "vendor\imgui\backends" /Fe"..\Loader.exe" loader-imgui.cpp vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp vendor\imgui\backends\imgui_impl_win32.cpp vendor\imgui\backends\imgui_impl_dx11.cpp /link /SUBSYSTEM:WINDOWS
exit /b %errorlevel%
