@echo off
chcp 65001 > nul

rem 一键编译学习版多文件项目。
rem 如果新增了 app_*.cpp 文件，需要同步把文件名加到下面的编译命令中。

g++ -std=c++11 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    main.cpp ui.cpp app.cpp ^
    app_core.cpp app_persistence.cpp app_display.cpp ^
    app_menus.cpp app_admin.cpp app_waiter.cpp ^
    -o restaurant.exe

if errorlevel 1 (
    echo.
    echo 编译失败，请根据上面的错误信息检查代码。
    pause
    exit /b 1
)

echo.
echo 编译成功：restaurant.exe
pause
