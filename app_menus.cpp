#include "app.hpp"
#include "app_internal.hpp"
#include "ui.hpp"

#include <vector>

// 本文件负责系统入口、登录流程以及管理员/服务员两级主菜单。
// 菜单函数只做页面调度，真正的业务修改分别放在 app_admin.cpp 和 app_waiter.cpp。

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
