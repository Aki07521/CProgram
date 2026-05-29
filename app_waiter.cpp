#include "app.hpp"
#include "app_internal.hpp"
#include "ui.hpp"

#include <vector>

// 本文件保存服务员业务：开台点餐、修改订单、退菜、结账和个人统计。
// 这些函数会频繁调用 app_core.cpp 中的订单规则函数，重点观察库存、订单金额和餐桌状态如何同步变化。

/*
    函数用途：
    服务员“开台点餐”的完整流程入口。

    接口说明：
    - 无参数。
    - 无返回值，所有结果通过链表数据、txt 文件和界面提示体现。

    业务流程：
    1. 选择一个空闲餐桌。
    2. 创建一个新的订单节点，但先不立刻加入订单链表。
    3. 循环选择菜品和数量，调用 addDishToOrder 扣库存、加明细、重算金额。
    4. 如果没有点任何菜，删除临时订单，开台取消。
    5. 如果点了菜，把订单加入 g_orders，并把餐桌改为使用中。

    学习重点：
    - makeOrder 只负责创建节点。
    - appendOrder 才是真正把订单放进全局订单链表。
    - table->activeOrderId 是餐桌和订单之间的连接点。
*/
void showOpenTableOrder() {
    ensureDataLoaded();

    // 开台点餐流程：
    // 1. 选择空闲餐桌；
    // 2. 创建新订单节点；
    // 3. 循环选择菜品并扣减库存；
    // 4. 订单加入订单链表，餐桌状态改为使用中。
    showTableStatusDemo();
    drawWorkPage("开台点餐");
    printAt(18, 8, "");
    int tableId = inputExistingTableId("餐桌号 输入0返回", true);
    if (tableId == 0) {
        return;
    }

    TableNode* table = findTable(tableId);
    if (table->status != TABLE_IDLE) {
        UI::showError("该餐桌不是空闲状态，不能开台。");
        return;
    }

    OrderNode* order = makeOrder(g_nextOrderId++, tableId, g_currentUsername, ORDER_ACTIVE,
                                 0.0, nowText(), "");

    while (true) {
        // 0 作为结束点餐标记，其余编号必须是已存在菜品。
        showDishTableDemo();
        drawWorkPage("添加点餐菜品");
        printAt(18, 7, "当前订单金额：" + money(order->totalAmount) + " 元");
        printAt(18, 9, "");
        int dishId = inputExistingDishId("菜品编号 输入0结束", true);
        if (dishId == 0) {
            break;
        }

        DishNode* dish = findDish(dishId);
        if (dish->stock <= 0) {
            UI::showError("该菜品库存不足。");
            continue;
        }
        printAt(18, 11, "");
        int quantity = UI::inputInt("数量", 1, dish->stock);
        addDishToOrder(order, dish, quantity);
        UI::showSuccess("菜品已加入订单。");
    }

    if (!orderHasItems(order)) {
        // 如果用户开台后没有点任何菜，则不生成空订单。
        clearOrderItems(order->items);
        delete order;
        UI::showMessage("提示", "没有选择菜品，开台已取消。");
        return;
    }

    appendOrder(order);
    // 餐桌记录保存当前订单号，后续修改、退菜、结账都通过它找到订单。
    table->status = TABLE_USING;
    table->currentAmount = order->totalAmount;
    table->activeOrderId = order->orderId;
    saveAll();
    UI::showSuccess("开台点餐完成，订单号：" + std::to_string(order->orderId));
}

/*
    函数用途：
    修改某张餐桌当前未结账订单。

    接口说明：
    - 无参数。
    - 无返回值。

    可执行操作：
    - 查看订单明细。
    - 追加菜品。
    - 修改菜品数量。
    - 删除订单菜品。
    - 设置为待结账。

    业务注意：
    已结账订单不能修改，因为它已经成为历史消费记录。

    语法/链表点：
    修改数量时，订单明细节点 item 来自 findOrderItem。
    如果数量变成 0，需要调用 updateEmptyOrder 判断订单是否要自动取消。
*/
void showModifyOrder() {
    ensureDataLoaded();

    // 修改订单只允许操作未结账订单，已结账订单作为历史记录不再修改。
    showTableStatusDemo();
    drawWorkPage("修改订单");
    printAt(18, 8, "");
    int tableId = inputExistingTableId("餐桌号 输入0返回", true);
    if (tableId == 0) {
        return;
    }

    OrderNode* order = findOrderByTable(tableId);
    if (order == NULL || order->status == ORDER_PAID || order->status == ORDER_CANCELLED) {
        UI::showError("该餐桌没有可修改的未结账订单。");
        return;
    }

    std::vector<std::string> items;
    items.push_back("查看订单明细");
    items.push_back("追加菜品");
    items.push_back("修改菜品数量");
    items.push_back("删除订单菜品");
    items.push_back("设为待结账");
    items.push_back("返回上级");

    while (true) {
        int choice = UI::menuSelect("修改订单：" + std::to_string(order->orderId), items);
        if (choice == 0) {
            showReceiptForOrder(order, true);
        } else if (choice == 1) {
            // 追加菜品复用 addDishToOrder，自动扣库存并重算总金额。
            showDishTableDemo();
            drawWorkPage("追加菜品");
            printAt(18, 9, "");
            int dishId = inputExistingDishId("菜品编号", false);
            DishNode* dish = findDish(dishId);
            if (dish->stock <= 0) {
                UI::showError("该菜品库存不足。");
                continue;
            }
            printAt(18, 11, "");
            int quantity = UI::inputInt("数量", 1, dish->stock);
            addDishToOrder(order, dish, quantity);
            recalcOrderTotal(order);
            saveAll();
            UI::showSuccess("菜品已追加。");
        } else if (choice == 2) {
            // 修改数量时，新数量可以为 0，表示从订单中删除该菜品。
            showReceiptForOrder(order, true);
            drawWorkPage("修改菜品数量");
            printAt(18, 8, "");
            int dishId = inputExistingDishId("菜品编号", false);
            OrderItemNode* item = findOrderItem(order, dishId);
            if (item == NULL) {
                UI::showError("订单中没有该菜品。");
                continue;
            }
            DishNode* dish = findDish(dishId);
            int maxQuantity = item->quantity + (dish == NULL ? 0 : dish->stock);
            printAt(18, 10, "");
            int newQuantity = UI::inputInt("新数量 输入0表示删除", 0, maxQuantity);
            if (newQuantity > item->quantity && dish != NULL) {
                // 数量增加时，需要补扣库存和增加销量。
                int diff = newQuantity - item->quantity;
                dish->stock -= diff;
                dish->sold += diff;
                item->quantity = newQuantity;
            } else if (newQuantity < item->quantity) {
                // 数量减少时，统一调用 removeOrReduceItem 回滚库存。
                removeOrReduceItem(order, dishId, item->quantity - newQuantity);
            }
            if (newQuantity == 0) {
                // 如果这是订单最后一道菜，订单会自动取消并释放餐桌。
                updateEmptyOrder(order);
            } else {
                recalcOrderTotal(order);
            }
            saveAll();
            UI::showSuccess("订单数量已更新。");
            if (order->status == ORDER_CANCELLED) {
                return;
            }
        } else if (choice == 3) {
            showReceiptForOrder(order, true);
            drawWorkPage("删除订单菜品");
            printAt(18, 8, "");
            int dishId = inputExistingDishId("菜品编号", false);
            OrderItemNode* item = findOrderItem(order, dishId);
            if (item == NULL) {
                UI::showError("订单中没有该菜品。");
                continue;
            }
            if (UI::confirm("确定删除该订单菜品吗？")) {
                removeOrReduceItem(order, dishId, item->quantity);
                updateEmptyOrder(order);
                saveAll();
                UI::showSuccess("订单菜品已删除。");
                if (order->status == ORDER_CANCELLED) {
                    return;
                }
            }
        } else if (choice == 4) {
            // 待结账状态表示不再继续点菜，下一步通常进入顾客结账。
            TableNode* table = findTable(order->tableId);
            order->status = ORDER_WAIT_PAY;
            if (table != NULL) {
                table->status = TABLE_WAIT_PAY;
                table->currentAmount = order->totalAmount;
            }
            saveAll();
            UI::showSuccess("订单已设为待结账。");
        } else if (choice == 5 || choice == -1) {
            return;
        }
    }
}

/*
    函数用途：
    服务员退菜处理。

    接口说明：
    - 无参数。
    - 无返回值。

    业务流程：
    1. 根据餐桌号找到当前未结账订单。
    2. 显示订单小票，让服务员确认订单内容。
    3. 输入要退的菜品编号和数量。
    4. 调用 removeOrReduceItem 减少订单明细，并恢复库存。
    5. 调用 updateEmptyOrder 检查订单是否已经没有菜品。

    学习重点：
    退菜不是单独建一条“退菜记录”，而是修改订单明细链表。
*/
void showReturnDish() {
    ensureDataLoaded();

    // 退菜本质上是减少订单明细数量，同时恢复对应菜品库存。
    showTableStatusDemo();
    drawWorkPage("退菜处理");
    printAt(18, 8, "");
    int tableId = inputExistingTableId("餐桌号 输入0返回", true);
    if (tableId == 0) {
        return;
    }

    OrderNode* order = findOrderByTable(tableId);
    if (order == NULL || order->status == ORDER_PAID || order->status == ORDER_CANCELLED) {
        UI::showError("该餐桌没有可退菜的未结账订单。");
        return;
    }

    showReceiptForOrder(order, true);
    drawWorkPage("退菜处理");
    printAt(18, 8, "");
    int dishId = inputExistingDishId("退菜编号", false);
    OrderItemNode* item = findOrderItem(order, dishId);
    if (item == NULL) {
        UI::showError("订单中没有该菜品。");
        return;
    }
    printAt(18, 10, "");
    int quantity = UI::inputInt("退菜数量", 1, item->quantity);
    if (UI::confirm("确定执行退菜吗？")) {
        removeOrReduceItem(order, dishId, quantity);
        updateEmptyOrder(order);
        saveAll();
        UI::showSuccess("退菜完成。");
    }
}

/*
    函数用途：
    顾客结账。

    接口说明：
    - 无参数。
    - 无返回值。

    业务流程：
    1. 通过餐桌号找到当前订单。
    2. 展示小票。
    3. 服务员确认收款。
    4. 订单状态改为 ORDER_PAID。
    5. 记录 paidAt 结账时间。
    6. 餐桌恢复为空闲。

    注意：
    已结账订单不会恢复库存，因为它代表真实销售。
*/
void showCheckoutPage() {
    ensureDataLoaded();

    // 结账流程会把订单状态改为已结账，并释放餐桌。
    // 已结账订单保留金额，用于订单查询和营收统计。
    showTableStatusDemo();
    drawWorkPage("顾客结账");
    printAt(18, 8, "");
    int tableId = inputExistingTableId("餐桌号 输入0返回", true);
    if (tableId == 0) {
        return;
    }

    OrderNode* order = findOrderByTable(tableId);
    if (order == NULL || order->status == ORDER_PAID || order->status == ORDER_CANCELLED) {
        UI::showError("该餐桌没有可结账订单。");
        return;
    }

    showReceiptForOrder(order, true);
    if (!UI::confirm("确认收款 " + money(order->totalAmount) + " 元吗？")) {
        return;
    }

    order->status = ORDER_PAID;
    order->paidAt = nowText();

    TableNode* table = findTable(order->tableId);
    if (table != NULL) {
        // 收款完成后餐桌立即恢复为空闲，可以继续开台。
        table->status = TABLE_IDLE;
        table->currentAmount = 0.0;
        table->activeOrderId = 0;
    }
    saveAll();
    UI::showSuccess("结账完成，餐桌已释放。");
}

/*
    函数用途：
    统计当前登录服务员的个人业绩。

    接口说明：
    - 无参数。
    - 无返回值。

    统计逻辑：
    遍历 g_orders 订单链表，只统计 order->waiter 等于当前登录账号的订单。

    学习重点：
    这是一个典型的“遍历链表 + 条件筛选 + 累加统计”的例子。
*/
void showPersonalStats() {
    ensureDataLoaded();

    // 个人统计只统计当前登录服务员创建的订单。
    int paidCount = 0;
    int activeCount = 0;
    double amount = 0.0;

    for (OrderNode* order = g_orders; order != NULL; order = order->next) {
        if (order->waiter != g_currentUsername) {
            continue;
        }
        if (order->status == ORDER_PAID) {
            ++paidCount;
            amount += order->totalAmount;
        } else if (order->status == ORDER_ACTIVE || order->status == ORDER_WAIT_PAY) {
            ++activeCount;
        }
    }

    drawWorkPage("个人服务统计");
    printAt(18, 8, "服务员账号：" + g_currentUsername);
    printAt(18, 10, "服务员姓名：" + currentDisplayName());
    printAt(18, 12, "已结账订单数：" + std::to_string(paidCount));
    printAt(18, 14, "未结账订单数：" + std::to_string(activeCount));
    printAt(18, 16, "个人累计收款：" + money(amount) + " 元");
    UI::pause();
}
