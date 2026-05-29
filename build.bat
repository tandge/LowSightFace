@echo off & setlocal enabledelayedexpansion
set "ROOT=%~dp0"
set "PRO=!ROOT!BntechEyeFriend.pro"

if not defined QTDIR    set "QTDIR=C:\Qt\5.15.2\msvc2019_64"
if not defined QT_WASM  set "QT_WASM=C:\Qt\5.15.2\wasm_32"
if not defined EMSDK    set "EMSDK=C:\emsdk"
if not defined VCDIR    set "VCDIR=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build"

echo !QTDIR!|findstr /i "msvc" >nul
if not errorlevel 1 (
  echo [VS] vcvars64.bat ...
  call "!VCDIR!\vcvars64.bat"
  set "MAKE=nmake"
) else (
  set "MAKE=mingw32-make -j%NUMBER_OF_PROCESSORS%"
)

:menu
echo.
echo [1]Desktop-Debug  [2]Desktop-Release  [3]WASM-Debug  [4]WASM-Release  [5]Clean  [0]Exit
set /p c=Choice: 
if "%c%"=="1" (set "DIR=desktop_debug"&  set "TYPE=debug"&   set "BASE=!ROOT!build"&      goto desktop)
if "%c%"=="2" (set "DIR=desktop_release"& set "TYPE=release"& set "BASE=!ROOT!build"&      goto desktop)
if "%c%"=="3" (set "DIR=wasm_debug"&     set "TYPE=debug"&   set "BASE=!ROOT!build_wasm"&  goto wasm)
if "%c%"=="4" (set "DIR=wasm_release"&   set "TYPE=release"& set "BASE=!ROOT!build_wasm"&  goto wasm)
if "%c%"=="5" (rmdir /s /q "!ROOT!build" "!ROOT!build_wasm" 2>nul& echo Cleaned& goto menu)
if "%c%"=="0" exit /b
goto menu

:desktop
set "OUT=!BASE!\!DIR!"
echo !OUT!
if not exist "!OUT!" mkdir "!OUT!"
cd /d "!OUT!" || (echo Cannot cd& pause& goto menu)
"!QTDIR!\bin\qmake" "!PRO!" CONFIG+=!TYPE!
if errorlevel 1 (echo [ERROR] qmake& pause& goto menu)
!MAKE!
if errorlevel 1 (echo [ERROR] Build& pause& goto menu)
"!QTDIR!\bin\windeployqt" --release --no-translations --no-compiler-runtime release\BntechEyeFriend.exe
if errorlevel 1 echo [WARN] windeployqt failed, copy Qt DLLs manually
echo [OK] !OUT! - run: release\BntechEyeFriend.exe
goto menu

:wasm
set "OUT=!BASE!\!DIR!"
if not exist "!EMSDK!\emsdk_env.bat" (echo EMSDK not found& pause& goto menu)
call "!EMSDK!\emsdk_env.bat"
echo !OUT!
if not exist "!OUT!" mkdir "!OUT!"
cd /d "!OUT!" || (echo Cannot cd& pause& goto menu)
"!QT_WASM!\bin\qmake" "!PRO!" CONFIG+=!TYPE!
if errorlevel 1 (echo [ERROR] qmake& pause& goto menu)
mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (echo [ERROR] Build& pause& goto menu)
echo [OK] !OUT!  ^(python -m http.server 8000^)
goto menu
