#ifndef APP_HPP
#define APP_HPP

#include <string>

/*
    app.hpp 是 app 模块对外公开的头文件。

    main.cpp 只需要知道“从哪里进入系统”，不需要知道链表节点、文件读写、
    库存扣减等内部细节。因此这里不声明 UserNode、DishNode 等结构体，
    只声明可以被 main.cpp 或其他外部模块调用的页面函数。

    内部学习入口在 app_internal.hpp：
    - 想看数据结构：先看 app_internal.hpp。
    - 想看链表增删改查：看 app_core.cpp。
    - 想看文件读写：看 app_persistence.cpp。
    - 想看管理员/服务员业务：看 app_admin.cpp、app_waiter.cpp。
*/

// app 模块负责业务流程和页面跳转，UI 绘制细节放在 ui.cpp 中。
// 管理员与服务员登录后会进入不同菜单，因此声明分角色页面函数。

// 系统入口页面和登录页面。
void showMainMenu();
void showLoginDemo(const std::string& role);
void showAdminMenu();
void showWaiterMenu();

// 管理员可进入的业务模块。
void showUserManagement();
void showDishManagement();
void showTableManagement();
void showOrderQuery();
void showRevenueStats();
void showSystemSettings();

// 服务员可进入的业务模块。
void showOpenTableOrder();
void showModifyOrder();
void showReturnDish();
void showCheckoutPage();
void showPersonalStats();

// 通用展示页面，既可供管理员查看，也可供服务员业务流程复用。
void showDishTableDemo();
void showTableStatusDemo();
void showOrderReceiptDemo();
void showTodoPage(const std::string& moduleName);

#endif
