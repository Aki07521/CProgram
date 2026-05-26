# 中小饭店点餐系统学习版说明

这个版本把原来集中在 `app.cpp` 里的业务代码拆成多个文件，阅读时可以按“数据结构 -> 核心规则 -> 文件读写 -> 页面业务”的顺序学习。

## 文件分工

- `main.cpp`：程序入口，只负责初始化控制台并进入主菜单。
- `ui.hpp` / `ui.cpp`：控制台界面工具，例如清屏、画框、菜单、输入、确认框。
- `app.hpp`：app 模块对外公开的页面函数声明，供 `main.cpp` 调用。
- `app_internal.hpp`：app 模块内部说明书，集中放链表结构体、状态常量、全局链表和内部函数声明。
- `app.cpp`：保留为模块导读，不再放大量业务代码。
- `app_core.cpp`：链表节点创建、追加、查找、释放，以及订单金额、库存、餐桌状态同步规则。
- `app_persistence.cpp`：把链表数据保存到 `users.txt`、`dishes.txt`、`tables.txt`、`orders.txt`、`order_items.txt`。
- `app_display.cpp`：菜品表、餐桌表、用户表、订单表、小票等只读展示页面。
- `app_menus.cpp`：系统主菜单、登录流程、管理员菜单、服务员菜单。
- `app_admin.cpp`：管理员功能，包括用户管理、菜品管理、餐桌管理、订单查询、营收统计、系统设置。
- `app_waiter.cpp`：服务员功能，包括开台点餐、修改订单、退菜、结账、个人统计。

## 推荐阅读顺序

1. 先看 `app_internal.hpp`，理解 `UserNode`、`DishNode`、`TableNode`、`OrderNode` 和 `OrderItemNode` 五种链表节点。
2. 再看 `app_core.cpp`，重点看 `appendXxx`、`findXxx`、`removeOrReduceItem`、`recalcOrderTotal`。
3. 然后看 `app_persistence.cpp`，理解一行文本如何通过 `split` 还原成链表节点。
4. 最后看 `app_admin.cpp` 和 `app_waiter.cpp`，观察页面函数如何调用底层链表和订单规则。

## 编译方式

双击 `build.bat`，或在当前目录运行：

```bat
build.bat
```

也可以手动编译：

```bat
g++ -std=c++11 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 main.cpp ui.cpp app.cpp app_core.cpp app_persistence.cpp app_display.cpp app_menus.cpp app_admin.cpp app_waiter.cpp -o restaurant.exe
```
