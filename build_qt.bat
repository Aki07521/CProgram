@echo off
chcp 65001 > nul

set QT_DIR=E:\Qt\6.11.1\mingw_64
set PATH=%QT_DIR%\bin;E:\Qt\Tools\mingw1310_64\bin;%PATH%
set BUILD_DIR=%TEMP%\restaurant_qt_build

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -S . -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%QT_DIR%
if errorlevel 1 (
    echo.
    echo Qt configure failed.
    pause
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo.
    echo Qt build failed.
    exit /b 1
)

echo.
echo Qt build succeeded: restaurant_qt.exe
echo Run with: run_qt.bat
