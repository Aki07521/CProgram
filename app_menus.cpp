#include "app.hpp"
#include "app_internal.hpp"
#include "ui.hpp"

#include <vector>

// 本文件负责系统入口、登录流程以及管理员/服务员两级主菜单。
// 菜单函数只做页面调度，真正的业务修改分别放在 app_admin.cpp 和 app_waiter.cpp。

/*
    函数用途：
    系统主菜单入口。

    接口说明：
    - 无参数。
    - 无返回值；当用户确认退出系统时函数结束。

    业务流程：
    1. ensureDataLoaded 确保账号、菜品、餐桌、订单等数据已经加载。
    2. 显示管理员登录、服务员登录、退出系统三个选项。
    3. 根据选择跳转到对应登录页面。

    学习重点：
    这是整个业务系统的入口函数，main.cpp 最终会调用它。
*/
void showMainMenu() {
    // 主菜单启动时先加载数据，保证登录账号和业务数据可用。
    ensureDataLoaded();

    std::vector<std::string> items;
    items.push_back("管理员登录");
    items.push_back("服务员登录");
    items.push_back("退出系统");

    while (true) {
        int choice = UI::menuSelect("中小饭店点餐管理系统", items);
        if (choice == 0) {
            showLoginDemo("admin");
        } else if (choice == 1) {
            showLoginDemo("waiter");
        } else if (choice == 2 || choice == -1) {
            if (UI::confirm("确定要退出系统吗？")) {
                // 正常退出前保存一次所有链表数据。
                saveAll();
                UI::showSuccess("数据已保存，感谢使用本系统。");
                return;
            }
        }
    }
}

/*
    函数用途：
    登录页面，根据角色校验账号密码。

    接口说明：
    - role："admin" 表示管理员登录；"waiter" 表示服务员登录。
    - 无返回值。

    业务规则：
    登录成功后，会把当前账号和角色保存到 g_currentUsername / g_currentRole。
    退出对应功能菜单后，再清空当前登录信息。

    学习重点：
    一个登录函数复用两种角色入口，靠 role 参数决定进入哪个菜单。
*/
void showLoginDemo(const std::string& role) {
    ensureDataLoaded();

    while (true) {
        UI::drawHeader(roleName(role) + "登录");
        UI::drawBox(LEFT, 6, WIDTH, 13);

        printAt(18, 8, "默认账号可用：");
        if (role == "admin") {
            printAt(18, 9, "管理员 admin / 123456");
        } else {
            printAt(18, 9, "服务员 waiter / 123456");
        }

        printAt(18, 12, "");
        std::string username = UI::inputString("用户名");
        printAt(18, 14, "");
        std::string password = UI::inputString("密  码");

        if (checkAccount(role, username, password)) {
            // 保存当前登录用户，后续开台点餐、个人统计会用到服务员账号。
            g_currentUsername = username;
            g_currentRole = role;
            UI::showSuccess("登录成功，正在进入" + roleName(role) + "功能菜单。");
            if (role == "admin") {
                showAdminMenu();
            } else {
                showWaiterMenu();
            }
            g_currentUsername.clear();
            g_currentRole.clear();
            return;
        }

        UI::showError("用户名、密码或角色不匹配。");
        if (!UI::confirm("是否重新输入登录信息？")) {
            return;
        }
    }
}

/*
    函数用途：
    管理员功能菜单。

    接口说明：
    - 无参数。
    - 无返回值。

    菜单特点：
    这里只负责分发功能，不直接写用户、菜品、餐桌的具体业务逻辑。
    具体功能在 app_admin.cpp 中实现。
*/
void showAdminMenu() {
    // 管理员负责基础资料维护和统计查询。
    std::vector<std::string> items;
    items.push_back("用户管理");
    items.push_back("菜品管理");
    items.push_back("餐桌管理");
    items.push_back("订单查询");
    items.push_back("营收统计");
    items.push_back("系统设置");
    items.push_back("退出登录");

    while (true) {
        int choice = UI::menuSelect("管理员功能菜单", items);
        if (choice == 0) {
            showUserManagement();
        } else if (choice == 1) {
            showDishManagement();
        } else if (choice == 2) {
            showTableManagement();
        } else if (choice == 3) {
            showOrderQuery();
        } else if (choice == 4) {
            showRevenueStats();
        } else if (choice == 5) {
            showSystemSettings();
        } else if (choice == 6 || choice == -1) {
            saveAll();
            return;
        }
    }
}

/*
    函数用途：
    服务员功能菜单。

    接口说明：
    - 无参数。
    - 无返回值。

    菜单特点：
    这里只负责跳转前台业务页面。
    具体开台、点餐、退菜、结账逻辑在 app_waiter.cpp 中实现。
*/
void showWaiterMenu() {
    // 服务员负责前台业务：开台、点餐、退菜、结账。
    std::vector<std::string> items;
    items.push_back("浏览菜单");
    items.push_back("开台点餐");
    items.push_back("修改订单");
    items.push_back("退菜处理");
    items.push_back("餐桌状态查询");
    items.push_back("顾客结账");
    items.push_back("个人服务统计");
    items.push_back("退出登录");

    while (true) {
        int choice = UI::menuSelect("服务员功能菜单", items);
        if (choice == 0) {
            showDishTableDemo();
        } else if (choice == 1) {
            showOpenTableOrder();
        } else if (choice == 2) {
            showModifyOrder();
        } else if (choice == 3) {
            showReturnDish();
        } else if (choice == 4) {
            showTableStatusDemo();
        } else if (choice == 5) {
            showCheckoutPage();
        } else if (choice == 6) {
            showPersonalStats();
        } else if (choice == 7 || choice == -1) {
            saveAll();
            return;
        }
    }
}
