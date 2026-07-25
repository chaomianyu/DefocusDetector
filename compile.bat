@echo off
chcp 65001 >nul

echo ================================
echo  RAW to Gray - MSVC Build Script
echo ================================
echo.

set OPENCV_PATH=D:\OpenCV\install2
set OPENCV_INCLUDE=%OPENCV_PATH%\include
set OPENCV_LIB=%OPENCV_PATH%\x64\vc18\lib
set OPENCV_BIN=%OPENCV_PATH%\x64\vc18\bin

set LIBRAW_PATH=D:\OpenCV\libraw\LibRaw-0.21.5
set LIBRAW_INCLUDE=%LIBRAW_PATH%
set LIBRAW_INCLUDE_LIBRAW=%LIBRAW_PATH%\libraw
set LIBRAW_LIB=%LIBRAW_PATH%\lib
set LIBRAW_BIN=%LIBRAW_PATH%\bin

set SOURCE=raw2gray
set TARGET=raw2gray

echo Checking OpenCV...
if not exist "%OPENCV_LIB%\opencv_core500.lib" (
    echo ERROR: OpenCV lib not found in %OPENCV_LIB%
    pause
    exit /b 1
)
echo OK: OpenCV found

echo Checking LibRaw...
if not exist "%LIBRAW_LIB%\libraw.lib" (
    echo ERROR: libraw.lib not found
    echo Path: %LIBRAW_LIB%
    pause
    exit /b 1
)
echo OK: LibRaw found

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
echo Compiling %SOURCE%.cpp with MSVC...
cl.exe /EHsc /nologo /utf-8 /std:c++17 "%SOURCE%.cpp" /Fe"%TARGET%.exe" ^
    /I "%OPENCV_INCLUDE%" ^
    /I "%LIBRAW_INCLUDE%" ^
    /I "%LIBRAW_INCLUDE_LIBRAW%" ^
    /link /LIBPATH:"%OPENCV_LIB%" opencv_core500.lib opencv_imgcodecs500.lib opencv_imgproc500.lib ^
          /LIBPATH:"%LIBRAW_LIB%" libraw.lib ^
          ws2_32.lib user32.lib

if %errorlevel% equ 0 (
    echo.
    echo SUCCESS: Compiled %TARGET%.exe

    echo.
    echo Copying DLLs...

    for %%D in (opencv_core500 opencv_imgcodecs500 opencv_imgproc500 opencv_geometry500 opencv_flann500) do (
        if exist "%OPENCV_BIN%\%%D.dll" (
            copy "%OPENCV_BIN%\%%D.dll" "%cd%\%%D.dll" /Y >nul
            echo   OK: %%D.dll
        )
    )

    if exist "%LIBRAW_BIN%\libraw.dll" (
        copy "%LIBRAW_BIN%\libraw.dll" "%cd%\libraw.dll" /Y >nul
        echo   OK: libraw.dll
    )

    echo.
    echo Running program...
    echo ----------------
    %TARGET%.exe
    echo ----------------
) else (
    echo.
    echo ERROR: Compilation failed!
    pause
)

echo.
echo ================================
pause
