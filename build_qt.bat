@echo off
chcp 65001 > nul

set QT_DIR=E:\Qt\6.11.1\mingw_64
set PATH=%QT_DIR%\bin;E:\Qt\Tools\mingw1310_64\bin;%PATH%

if not exist build_qt mkdir build_qt

cmake -S . -B build_qt -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%QT_DIR%
if errorlevel 1 (
    echo.
    echo Qt configure failed.
    pause
    exit /b 1
)

cmake --build build_qt --config Release
if errorlevel 1 (
    echo.
    echo Qt build failed.
    exit /b 1
)

windeployqt restaurant_qt.exe

echo.
echo Qt build succeeded: restaurant_qt.exe
