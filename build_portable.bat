@echo off
chcp 65001 > nul

rem Build a portable Windows release.
rem -static-libgcc and -static-libstdc++ bundle MinGW runtime libraries
rem into the exe, so other computers do not need libstdc++-6.dll.

if not exist release_portable mkdir release_portable

g++ -std=c++11 -Wall -Wextra -static -static-libgcc -static-libstdc++ ^
    -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    main.cpp ui.cpp app.cpp ^
    app_core.cpp app_persistence.cpp app_display.cpp ^
    app_menus.cpp app_admin.cpp app_waiter.cpp ^
    -o release_portable\restaurant.exe

if errorlevel 1 (
    echo.
    echo Portable build failed.
    pause
    exit /b 1
)

copy /Y users.txt release_portable\users.txt > nul
copy /Y dishes.txt release_portable\dishes.txt > nul
copy /Y tables.txt release_portable\tables.txt > nul
copy /Y orders.txt release_portable\orders.txt > nul
copy /Y order_items.txt release_portable\order_items.txt > nul

echo.
echo Portable build completed: release_portable\restaurant.exe
echo Send the entire release_portable folder to another Windows computer.
pause
