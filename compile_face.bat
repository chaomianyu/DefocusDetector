@echo off
chcp 65001 >nul

echo ================================
echo  Focus Evaluate - SCRFD 2.5G
echo ================================
echo.

set OPENCV_PATH=D:\OpenCV\install2
set OPENCV_INCLUDE=%OPENCV_PATH%\include
set OPENCV_LIB=%OPENCV_PATH%\x64\vc18\lib
set OPENCV_BIN=%OPENCV_PATH%\x64\vc18\bin

echo Checking OpenCV...
if not exist "%OPENCV_LIB%\opencv_core500.lib" (
    echo ERROR: OpenCV lib not found in %OPENCV_LIB%
    pause
    exit /b 1
)
echo OK: OpenCV found

echo.
echo Locating Visual Studio...
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    echo ERROR: vswhere.exe not found. Please install Visual Studio.
    pause
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VS_PATH=%%i
)

if "%VS_PATH%"=="" (
    echo ERROR: Visual Studio with C++ workload not found.
    pause
    exit /b 1
)
echo OK: Visual Studio found

echo.
echo Setting up MSVC environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo ERROR: Failed to setup MSVC environment
    pause
    exit /b 1
)
echo OK: MSVC environment ready

echo.
echo Compiling focus_evaluate...

cl.exe /EHsc /nologo /utf-8 /std:c++17 ^
    main.cpp api\FocusApi.cpp detector\ScrfdOnnxDetector.cpp core\EyeFocusEvaluator.cpp utils\SharpnessMetric.cpp ^
    /Fe"focus_evaluate.exe" ^
    /I "%OPENCV_INCLUDE%" ^
    /I . ^
    /link /LIBPATH:"%OPENCV_LIB%" ^
          opencv_core500.lib opencv_imgcodecs500.lib opencv_imgproc500.lib opencv_dnn500.lib ^
          ws2_32.lib user32.lib

if %errorlevel% equ 0 (
    echo.
    echo ================================
    echo SUCCESS: focus_evaluate.exe
    echo ================================

    echo.
    echo Copying OpenCV DLLs for runtime...
    for %%D in (opencv_core500 opencv_imgcodecs500 opencv_imgproc500 opencv_dnn500 opencv_geometry500 opencv_flann500) do (
        if exist "%OPENCV_BIN%\%%D.dll" (
            copy "%OPENCV_BIN%\%%D.dll" "%cd%\%%D.dll" /Y >nul
            echo   OK: %%D.dll
        )
    )

    echo.
    echo Usage: focus_evaluate.exe --input after --save
) else (
    echo.
    echo ================================
    echo ERROR: Compilation failed!
    echo ================================
)

echo.
pause
