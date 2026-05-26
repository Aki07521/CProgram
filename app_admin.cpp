#include "app.hpp"
#include "app_internal.hpp"
#include "ui.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

// 本文件保存管理员业务：用户、菜品、餐桌维护，以及订单查询、营收统计和系统设置。
// 读代码时可以把它理解为“后台管理端”的控制层。

void showUserManagement() {
    ensureDataLoaded();

    // 用户管理实现系统账号的增删改查。
    // 角色字段决定登录后进入管理员菜单还是服务员菜单。
    std::vector<std::string> items;
    items.push_back("浏览用户");
    items.push_back("新增用户");
    items.push_back("修改用户");
    items.push_back("删除用户");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("用户管理", items);
        if (choice == 0) {
            showUserListPage();
        } else if (choice == 1) {
            // 新增用户时先检查账号唯一性，避免登录时出现重复账号。
            drawWorkPage("新增用户");
            printAt(18, 8, "");
            std::string username = readRequiredString("账号");
            if (findUser(username) != NULL) {
                UI::showError("该账号已存在。");
                continue;
            }
            printAt(18, 10, "");
            std::string password = readRequiredString("密码");
            printAt(18, 12, "");
            std::string name = readRequiredString("姓名");
            printAt(18, 14, "");
            int roleChoice = UI::inputInt("角色 1管理员 2服务员", 1, 2);
            appendUser(makeUser(username, password, roleChoice == 1 ? "admin" : "waiter", name));
            saveAll();
            UI::showSuccess("用户添加成功。");
        } else if (choice == 2) {
            // 修改用户时采用二级菜单，只修改用户选择的字段。
            drawWorkPage("修改用户");
            printAt(18, 8, "");
            std::string username = readRequiredString("要修改的账号");
            UserNode* user = findUser(username);
            if (user == NULL) {
                UI::showError("没有找到该用户。");
                continue;
            }

            std::vector<std::string> sub;
            sub.push_back("修改姓名");
            sub.push_back("修改密码");
            sub.push_back("修改角色");
            sub.push_back("返回");
            int subChoice = UI::menuSelect("修改用户：" + username, sub);
            if (subChoice == 0) {
                drawWorkPage("修改姓名");
                printAt(18, 9, "");
                user->name = readRequiredString("新姓名");
            } else if (subChoice == 1) {
                drawWorkPage("修改密码");
                printAt(18, 9, "");
                user->password = readRequiredString("新密码");
            } else if (subChoice == 2) {
                if (user->role == "admin" && countAdmins() <= 1) {
                    // 至少保留一个管理员，否则后续无法进入管理员菜单维护系统。
                    UI::showError("系统至少需要保留一个管理员。");
                    continue;
                }
                drawWorkPage("修改角色");
                printAt(18, 9, "");
                int roleChoice = UI::inputInt("新角色 1管理员 2服务员", 1, 2);
                user->role = roleChoice == 1 ? "admin" : "waiter";
            } else {
                continue;
            }
            saveAll();
            UI::showSuccess("用户信息已更新。");
        } else if (choice == 3) {
            // 删除链表节点需要找到目标节点和它的前驱节点。
            drawWorkPage("删除用户");
            printAt(18, 9, "");
            std::string username = readRequiredString("要删除的账号");
            if (username == g_currentUsername) {
                UI::showError("不能删除当前登录账号。");
                continue;
            }

            UserNode* prev = NULL;
            UserNode* user = g_users;
            while (user != NULL && user->username != username) {
                prev = user;
                user = user->next;
            }
            if (user == NULL) {
                UI::showError("没有找到该用户。");
                continue;
            }
            if (user->role == "admin" && countAdmins() <= 1) {
                UI::showError("系统至少需要保留一个管理员。");
                continue;
            }
            if (UI::confirm("确定删除用户 " + username + " 吗？")) {
                if (prev == NULL) {
                    g_users = user->next;
                } else {
                    prev->next = user->next;
                }
                delete user;
                saveAll();
                UI::showSuccess("用户已删除。");
            }
        } else if (choice == 4 || choice == -1) {
            return;
        }
    }
}

void showDishManagement() {
    ensureDataLoaded();

    // 菜品管理维护菜单基础资料，服务员点餐时会读取这里的数据。
    std::vector<std::string> items;
    items.push_back("浏览菜品");
    items.push_back("查询菜品");
    items.push_back("新增菜品");
    items.push_back("修改菜品");
    items.push_back("删除菜品");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("菜品管理", items);
        if (choice == 0) {
            showDishTableDemo();
        } else if (choice == 1) {
            drawWorkPage("查询菜品");
            printAt(18, 8, "");
            int id = inputExistingDishId("菜品编号", false);
            DishNode* dish = findDish(id);
            drawWorkPage("菜品详情");
            printAt(18, 8, "编号：" + std::to_string(dish->id));
            printAt(18, 10, "名称：" + dish->name);
            printAt(18, 12, "分类：" + dish->category);
            printAt(18, 14, "价格：" + money(dish->price));
            printAt(18, 16, "库存：" + std::to_string(dish->stock));
            printAt(18, 18, "累计点菜数量：" + std::to_string(dish->sold));
            UI::pause();
        } else if (choice == 2) {
            // 新增菜品时销量从 0 开始，库存由管理员录入。
            drawWorkPage("新增菜品");
            printAt(18, 7, "");
            int id = UI::inputInt("编号", 1, 999999);
            if (findDish(id) != NULL) {
                UI::showError("该菜品编号已存在。");
                continue;
            }
            printAt(18, 9, "");
            std::string name = readRequiredString("名称");
            printAt(18, 11, "");
            std::string category = readRequiredString("分类");
            printAt(18, 13, "");
            double price = inputDouble("价格", 0.01, 99999.0);
            printAt(18, 15, "");
            int stock = UI::inputInt("库存", 0, 999999);
            appendDish(makeDish(id, name, category, price, stock, 0));
            saveAll();
            UI::showSuccess("菜品添加成功。");
        } else if (choice == 3) {
            // 修改菜品价格只影响之后的新订单，历史订单明细中保存的是下单时价格。
            drawWorkPage("修改菜品");
            printAt(18, 8, "");
            int id = inputExistingDishId("菜品编号", false);
            DishNode* dish = findDish(id);
            std::vector<std::string> sub;
            sub.push_back("修改名称");
            sub.push_back("修改分类");
            sub.push_back("修改价格");
            sub.push_back("修改库存");
            sub.push_back("返回");
            int subChoice = UI::menuSelect("修改菜品：" + dish->name, sub);
            if (subChoice == 0) {
                drawWorkPage("修改菜品名称");
                printAt(18, 9, "");
                dish->name = readRequiredString("新名称");
            } else if (subChoice == 1) {
                drawWorkPage("修改菜品分类");
                printAt(18, 9, "");
                dish->category = readRequiredString("新分类");
            } else if (subChoice == 2) {
                drawWorkPage("修改菜品价格");
                printAt(18, 9, "");
                dish->price = inputDouble("新价格", 0.01, 99999.0);
            } else if (subChoice == 3) {
                drawWorkPage("修改菜品库存");
                printAt(18, 9, "");
                dish->stock = UI::inputInt("新库存", 0, 999999);
            } else {
                continue;
            }
            saveAll();
            UI::showSuccess("菜品信息已更新。");
        } else if (choice == 4) {
            drawWorkPage("删除菜品");
            printAt(18, 9, "");
            int id = inputExistingDishId("菜品编号", false);
            DishNode* dish = findDish(id);
            if (dishInUnpaidOrder(id)) {
                // 未结账订单仍可能继续打印小票或修改数量，因此禁止删除正在使用的菜品。
                UI::showError("该菜品存在于未结账订单中，暂不能删除。");
                continue;
            }
            if (!UI::confirm("确定删除菜品 " + dish->name + " 吗？")) {
                continue;
            }

            DishNode* prev = NULL;
            DishNode* p = g_dishes;
            while (p != NULL && p->id != id) {
                prev = p;
                p = p->next;
            }
            if (p != NULL) {
                if (prev == NULL) {
                    g_dishes = p->next;
                } else {
                    prev->next = p->next;
                }
                delete p;
                saveAll();
                UI::showSuccess("菜品已删除。");
            }
        } else if (choice == 5 || choice == -1) {
            return;
        }
    }
}

void showTableManagement() {
    ensureDataLoaded();

    // 餐桌管理维护堂食桌台资料，开台点餐只能选择空闲餐桌。
    std::vector<std::string> items;
    items.push_back("浏览餐桌");
    items.push_back("新增餐桌");
    items.push_back("修改餐桌容量");
    items.push_back("删除餐桌");
    items.push_back("强制释放餐桌");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("餐桌管理", items);
        if (choice == 0) {
            showTableStatusDemo();
        } else if (choice == 1) {
            drawWorkPage("新增餐桌");
            printAt(18, 8, "");
            int id = UI::inputInt("餐桌号", 1, 9999);
            if (findTable(id) != NULL) {
                UI::showError("该餐桌号已存在。");
                continue;
            }
            printAt(18, 10, "");
            int capacity = UI::inputInt("容量", 1, 50);
            appendTable(makeTable(id, capacity, TABLE_IDLE, 0.0, 0));
            saveAll();
            UI::showSuccess("餐桌添加成功。");
        } else if (choice == 2) {
            drawWorkPage("修改餐桌容量");
            printAt(18, 8, "");
            int id = inputExistingTableId("餐桌号", false);
            TableNode* table = findTable(id);
            printAt(18, 10, "");
            table->capacity = UI::inputInt("新容量", 1, 50);
            saveAll();
            UI::showSuccess("餐桌容量已更新。");
        } else if (choice == 3) {
            drawWorkPage("删除餐桌");
            printAt(18, 9, "");
            int id = inputExistingTableId("餐桌号", false);
            TableNode* table = findTable(id);
            if (table->status != TABLE_IDLE) {
                // 正在使用或待结账的餐桌有关联订单，不能直接删除。
                UI::showError("餐桌正在使用或待结账，不能删除。");
                continue;
            }
            if (!UI::confirm("确定删除该餐桌吗？")) {
                continue;
            }

            TableNode* prev = NULL;
            TableNode* p = g_tables;
            while (p != NULL && p->id != id) {
                prev = p;
                p = p->next;
            }
            if (p != NULL) {
                if (prev == NULL) {
                    g_tables = p->next;
                } else {
                    prev->next = p->next;
                }
                delete p;
                saveAll();
                UI::showSuccess("餐桌已删除。");
            }
        } else if (choice == 4) {
            // 强制释放用于处理误开台或测试数据，释放时会取消未结账订单并回滚库存。
            drawWorkPage("强制释放餐桌");
            printAt(18, 9, "");
            int id = inputExistingTableId("餐桌号", false);
            TableNode* table = findTable(id);
            if (table->status == TABLE_IDLE) {
                UI::showMessage("提示", "该餐桌已经处于空闲状态。");
                continue;
            }
            if (UI::confirm("释放餐桌会取消未结账订单，是否继续？")) {
                OrderNode* order = findOrder(table->activeOrderId);
                cancelOrder(order);
                table->status = TABLE_IDLE;
                table->currentAmount = 0.0;
                table->activeOrderId = 0;
                saveAll();
                UI::showSuccess("餐桌已释放。");
            }
        } else if (choice == 5 || choice == -1) {
            return;
        }
    }
}

void showOrderQuery() {
    ensureDataLoaded();

    // 订单查询只查看数据，不修改订单状态。
    std::vector<std::string> items;
    items.push_back("浏览订单");
    items.push_back("按编号查看订单明细");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("订单查询", items);
        if (choice == 0) {
            showOrderListPage();
        } else if (choice == 1) {
            drawWorkPage("查看订单明细");
            printAt(18, 9, "");
            int id = UI::inputInt("订单号", 1, 999999);
            OrderNode* order = findOrder(id);
            if (order == NULL) {
                UI::showError("没有找到该订单。");
            } else {
                showReceiptForOrder(order, true);
            }
        } else if (choice == 2 || choice == -1) {
            return;
        }
    }
}

void showRevenueStats() {
    ensureDataLoaded();

    // 营收只统计已结账订单，进行中、待结账和已取消订单不计入收入。
    int paidCount = 0;
    int unpaidCount = 0;
    double revenue = 0.0;

    for (OrderNode* order = g_orders; order != NULL; order = order->next) {
        if (order->status == ORDER_PAID) {
            ++paidCount;
            revenue += order->totalAmount;
        } else if (order->status == ORDER_ACTIVE || order->status == ORDER_WAIT_PAY) {
            ++unpaidCount;
        }
    }

    drawWorkPage("营收统计");
    printAt(16, 7, "已结账订单数：" + std::to_string(paidCount));
    printAt(16, 8, "未结账订单数：" + std::to_string(unpaidCount));
    printAt(16, 9, "累计营业额：" + money(revenue) + " 元");
    if (paidCount > 0) {
        printAt(16, 10, "平均每单消费：" + money(revenue / paidCount) + " 元");
    } else {
        printAt(16, 10, "平均每单消费：0.00 元");
    }

    printLine(12, "菜品销售统计（仅统计已结账订单）：");
    int y = 13;
    for (DishNode* dish = g_dishes; dish != NULL && y <= 21; dish = dish->next) {
        // 统计时从已结账订单明细中重新累加，保证结果与历史小票一致。
        int quantity = 0;
        double amount = 0.0;
        for (OrderNode* order = g_orders; order != NULL; order = order->next) {
            if (order->status != ORDER_PAID) {
                continue;
            }
            for (OrderItemNode* item = order->items; item != NULL; item = item->next) {
                if (item->dishId == dish->id) {
                    quantity += item->quantity;
                    amount += item->subtotal;
                }
            }
        }
        if (quantity > 0) {
            std::ostringstream row;
            row << dish->name << "  数量：" << quantity << "  金额：" << money(amount) << " 元";
            printLine(y++, row.str());
        }
    }
    if (y == 13) {
        printLine(y, "暂无已结账菜品销售数据。");
    }
    UI::pause();
}

void showSystemSettings() {
    ensureDataLoaded();

    // 系统设置提供密码修改、手动保存和恢复演示数据。
    std::vector<std::string> items;
    items.push_back("修改当前账号密码");
    items.push_back("立即保存全部数据");
    items.push_back("重置为演示数据");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("系统设置", items);
        if (choice == 0) {
            UserNode* user = findUser(g_currentUsername);
            if (user == NULL) {
                UI::showError("当前账号不存在。");
                continue;
            }
            drawWorkPage("修改密码");
            printAt(18, 8, "");
            std::string oldPassword = readRequiredString("原密码");
            if (oldPassword != user->password) {
                UI::showError("原密码错误。");
                continue;
            }
            printAt(18, 10, "");
            std::string newPassword = readRequiredString("新密码");
            user->password = newPassword;
            saveAll();
            UI::showSuccess("密码已修改。");
        } else if (choice == 1) {
            saveAll();
            UI::showSuccess("全部数据已保存。");
        } else if (choice == 2) {
            if (UI::confirm("确定清空当前数据并恢复演示数据吗？")) {
                // 重置数据会清空订单和当前业务资料，适合演示前恢复初始状态。
                clearAllData();
                seedUsers();
                seedDishes();
                seedTables();
                saveAll();
                UI::showSuccess("演示数据已重置。");
            }
        } else if (choice == 3 || choice == -1) {
            return;
        }
    }
}
