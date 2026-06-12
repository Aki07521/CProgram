@echo off
chcp 65001 > nul
setlocal

set "QT_DIR=E:\Qt\6.11.1\mingw_64"
set "MINGW_DIR=E:\Qt\Tools\mingw1310_64"
set "PACKAGE_DIR=release_portable"
set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

call build_qt.bat
if errorlevel 1 (
    echo.
    echo Build failed. Portable package was not created.
    exit /b 1
)

if exist "%PACKAGE_DIR%" rmdir /s /q "%PACKAGE_DIR%"
mkdir "%PACKAGE_DIR%"

copy /Y "restaurant_qt.exe" "%PACKAGE_DIR%\" > nul
copy /Y "users.txt" "%PACKAGE_DIR%\" > nul
copy /Y "dishes.txt" "%PACKAGE_DIR%\" > nul
copy /Y "tables.txt" "%PACKAGE_DIR%\" > nul
copy /Y "orders.txt" "%PACKAGE_DIR%\" > nul
copy /Y "order_items.txt" "%PACKAGE_DIR%\" > nul

windeployqt --release --compiler-runtime --no-translations "%PACKAGE_DIR%\restaurant_qt.exe"
if errorlevel 1 (
    echo.
    echo Qt runtime deployment failed.
    exit /b 1
)

(
    echo Restaurant Ordering System - Portable Package
    echo.
    echo How to use:
    echo 1. Copy the whole release_portable folder, not only restaurant_qt.exe.
    echo 2. On another Windows computer, double-click restaurant_qt.exe in this folder.
    echo 3. Keep users.txt, dishes.txt, tables.txt, orders.txt and order_items.txt next to the exe.
    echo.
    echo Default accounts:
    echo admin / 123456
    echo waiter / 123456
) > "%PACKAGE_DIR%\README_PORTABLE.txt"

echo.
echo Portable package created: %PACKAGE_DIR%
echo Send the whole %PACKAGE_DIR% folder to another Windows computer.

endlocal
