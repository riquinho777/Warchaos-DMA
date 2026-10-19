@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>&1
if errorlevel 1 (
  echo Abra este script no Developer Command Prompt x64 do Visual Studio.
  exit /b 1
)
if not defined MEMPROCFS_INCLUDE (
  echo Defina MEMPROCFS_INCLUDE com a pasta que contem vmmdll.h e leechcore.h.
  exit /b 2
)
if not exist "%MEMPROCFS_INCLUDE%\vmmdll.h" exit /b 2
cl /nologo /std:c++17 /EHsc /O2 /MT /W4 /I "%MEMPROCFS_INCLUDE%" /Fo"esp-prototipo.obj" /Fe"..\app\esp-fuser-v39.exe" "esp-prototipo.cpp"
if errorlevel 1 exit /b 3
call Compilar-Loader.cmd
exit /b %errorlevel%
