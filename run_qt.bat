@echo off
chcp 65001 > nul

set QT_DIR=E:\Qt\6.11.1\mingw_64
set PATH=%QT_DIR%\bin;E:\Qt\Tools\mingw1310_64\bin;%PATH%

restaurant_qt.exe
