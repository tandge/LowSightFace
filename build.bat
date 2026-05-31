@echo off & setlocal enabledelayedexpansion
set "ROOT=%~dp0"
set "PRO=!ROOT!BntechEyeFriend.pro"

:: 优先使用系统环境变量 QTDIR，未设置时给出警告并使用 fallback
if not defined QTDIR (
    echo [WARN] QTDIR environment variable is NOT set. Using fallback.
    set "QTDIR=C:\Qt\5.15.2\msvc2019_64"
)
if not defined QT_WASM  set "QT_WASM=C:\Qt\5.15.2\wasm_32"
if not defined EMSDK    set "EMSDK=C:\emsdk"
if not defined VCDIR    set "VCDIR=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build"

echo [INFO] QTDIR = !QTDIR!
if not exist "!QTDIR!\bin\qmake.exe" (
    echo [ERROR] qmake.exe not found in "!QTDIR!\bin"
    echo         Please set QTDIR environment variable to your Qt installation path.
    pause
    exit /b 1
)

echo !QTDIR!|findstr /i "msvc" >nul
if not errorlevel 1 (
  echo [VS] vcvars64.bat ...
  call "!VCDIR!\vcvars64.bat"
  if errorlevel 1 (
      echo [ERROR] Failed to initialize VC++ x64 environment. Check VCDIR.
      pause
      goto menu
  )
  set "MAKE=nmake"

  :: 验证 Qt 为 64 位，防止 32/64 位混用导致 0xc000007b
  dumpbin /headers "!QTDIR!\bin\Qt5Core.dll" 2>nul | findstr "x64" >nul
  if errorlevel 1 (
      echo [ERROR] Qt5Core.dll is NOT 64-bit!
      echo         QTDIR might point to a 32-bit Qt installation.
      echo         Please set QTDIR to the correct 64-bit Qt path ^(e.g., xxx_msvc2019_64^).
      pause
      goto menu
  )
  echo [OK] Qt 64-bit verified.
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

:: Copy OpenCV / ONNX Runtime DLLs (face module)
if /i "!TYPE!"=="debug" (
    if exist "C:\opencv\opencv\build\x64\vc16\bin\opencv_world490d.dll" (
        copy /y "C:\opencv\opencv\build\x64\vc16\bin\opencv_world490d.dll" "!OUT!\!TYPE!\"
    )
) else (
    if exist "C:\opencv\opencv\build\x64\vc16\bin\opencv_world490.dll" (
        copy /y "C:\opencv\opencv\build\x64\vc16\bin\opencv_world490.dll" "!OUT!\!TYPE!\"
    )
)
if exist "C:\onnxruntime\lib\onnxruntime.dll" (
    copy /y "C:\onnxruntime\lib\onnxruntime.dll" "!OUT!\!TYPE!\"
)
if exist "C:\onnxruntime\lib\onnxruntime_providers_shared.dll" (
    copy /y "C:\onnxruntime\lib\onnxruntime_providers_shared.dll" "!OUT!\!TYPE!\"
)

:: 若存在旧版 Qt DLL，先删除，防止 windeployqt 跳过复制
if exist "!OUT!\!TYPE!\Qt5Core.dll" del /q "!OUT!\!TYPE!\Qt5Core.dll" 2>nul

set "PATH=!QTDIR!\bin;%PATH%"
"!QTDIR!\bin\windeployqt" --!TYPE! --no-translations --no-compiler-runtime !TYPE!\BntechEyeFriend.exe
if errorlevel 1 echo [WARN] windeployqt failed, copy Qt DLLs manually

:: 校验部署出来的 Qt 是否为 64 位，防止 PATH 中的其他 Qt 干扰
dumpbin /headers "!OUT!\!TYPE!\Qt5Core.dll" 2>nul | findstr "x64" >nul
if errorlevel 1 (
    echo [ERROR] Deployed Qt5Core.dll is NOT 64-bit!
    echo         windeployqt may have picked up a 32-bit Qt from your system PATH.
    echo         Please remove 32-bit Qt paths from your system PATH environment variable.
    pause
    goto menu
)
echo [OK] Deployed Qt is 64-bit verified.

if exist "!ROOT!models" (
    xcopy /s /e /i /y "!ROOT!models" "!OUT!\!TYPE!\models\" >nul
    echo [OK] Copied models to output directory.
) else (
    echo [WARN] models directory not found in project root.
)
if exist "!ROOT!db" (
    xcopy /s /e /i /y "!ROOT!db" "!OUT!\!TYPE!\db\" >nul
    echo [OK] Copied db to output directory.
) else (
    echo [WARN] db directory not found in project root.
)
echo [OK] !OUT! - run: !TYPE!\BntechEyeFriend.exe

if /i "!TYPE!"=="release" (
  echo [PKG] Creating distributable zip ...
  set "DIST=!OUT!\dist"
  set "PKG=!DIST!\BntechEyeFriend"
  set "ZIP=!OUT!\BntechEyeFriend-win64.zip"
  if exist "!PKG!" rmdir /s /q "!PKG!"
  if exist "!ZIP!" del /q "!ZIP!"
  mkdir "!PKG!"
  copy /y "!OUT!\release\BntechEyeFriend.exe" "!PKG!\" >nul
  set "PATH=!QTDIR!\bin;%PATH%"
  "!QTDIR!\bin\windeployqt" --release --no-translations --compiler-runtime "!PKG!\BntechEyeFriend.exe"
  if errorlevel 1 (
    echo [ERROR] windeployqt failed for dist
    pause
    goto menu
  )

  :: 校验打包用的 Qt 是否为 64 位
  dumpbin /headers "!PKG!\Qt5Core.dll" 2>nul | findstr "x64" >nul
  if errorlevel 1 (
      echo [ERROR] Packaged Qt5Core.dll is NOT 64-bit!
      pause
      goto menu
  )

  if exist "!ROOT!models" (
      xcopy /s /e /i /y "!ROOT!models" "!PKG!\models\" >nul
      echo [OK] Copied models to package.
  )
  if exist "!ROOT!db" (
      xcopy /s /e /i /y "!ROOT!db" "!PKG!\db\" >nul
      echo [OK] Copied db to package.
  )
  powershell -NoProfile -Command "Compress-Archive -Path '!PKG!\*' -DestinationPath '!ZIP!' -Force"
  if errorlevel 1 (
    echo [ERROR] Compress-Archive failed
    pause
    goto menu
  )
  echo [OK] Package: !ZIP!
)
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
