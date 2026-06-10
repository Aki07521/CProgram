# 中小饭店点餐系统 Qt 版

本项目当前版本已经调整为前后端分离结构：Qt 只负责图形界面，业务数据、点餐规则和文件持久化集中在后端模块中。旧的 EasyX/控制台版本已不再作为主线维护。

## 当前结构

- `qt_main.cpp`: Qt 前端入口和窗口界面。
- `restaurant_backend.hpp` / `restaurant_backend.cpp`: 独立业务后端，负责用户、菜品、餐桌、订单和数据保存。
- `CMakeLists.txt`: Qt/CMake 构建配置。
- `build_qt.bat`: 一键构建脚本，默认使用 `E:\Qt\6.11.1\mingw_64`。
- `users.txt`、`dishes.txt`、`tables.txt`、`orders.txt`、`order_items.txt`: 程序运行数据。

## 构建方式

确认 Qt 安装路径为 `E:\Qt\6.11.1\mingw_64` 后，在项目根目录运行：

```bat
build_qt.bat
```

构建完成后运行：

```bat
restaurant_qt.exe
```

`build_qt/`、Qt DLL、Qt 插件目录和生成的 exe 都属于生成物，不纳入 git 管理。
