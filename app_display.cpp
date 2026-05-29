#include "app.hpp"
#include "app_internal.hpp"
#include "ui.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

// 本文件集中放置只读展示页面：列表、小票和兼容第一版的演示页面。
// 这些函数一般不修改链表数据，适合作为学习“遍历链表并格式化输出”的入口。

/*
    函数用途：
    显示指定订单的小票/订单明细。

    接口说明：
    - order：要显示的订单节点。
    - waitKey：是否在显示完成后暂停等待按键。

    业务特点：
    这是通用展示函数，订单查询、结账、修改订单前查看明细都会复用它。

    语法说明：
    std::ostringstream 用来一行一行拼接表格内容，再一次性输出。
*/
void showReceiptForOrder(OrderNode* order, bool waitKey) {
    if (order == NULL) {
        UI::showError("没有找到订单。");
        return;
    }

    // 小票页面统一展示订单基础信息和订单明细，也被订单查询、结账等流程复用。
    UI::drawHeader("结账小票 / 订单明细");
    UI::drawBox(LEFT, 4, WIDTH, 20);

    printAt(16, 6, "订单号：" + std::to_string(order->orderId));
    printAt(38, 6, "餐桌号：" + std::to_string(order->tableId));
    printAt(58, 6, "状态：" + orderStatusName(order->status));
    printAt(16, 7, "服务员：" + order->waiter);
    printAt(38, 7, "创建时间：" + order->createdAt);

    printLine(9,  "+----------+----------------------+----------+----------+----------+");
    printLine(10, "| 菜品编号 | 菜品名称             | 数量     | 单价     | 小计     |");
    printLine(11, "+----------+----------------------+----------+----------+----------+");

    int y = 12;
    for (OrderItemNode* item = order->items; item != NULL && y <= 17; item = item->next, ++y) {
        std::ostringstream row;
        row << "| " << std::setw(8) << std::left << item->dishId
            << " | " << std::setw(20) << std::left << item->dishName
            << " | " << std::setw(8) << std::left << item->quantity
            << " | " << std::setw(8) << std::left << money(item->price)
            << " | " << std::setw(8) << std::left << money(item->subtotal) << " |";
        printLine(y, row.str());
    }
    printLine(18, "+----------+----------------------+----------+----------+----------+");
    printAt(16, 20, "应付金额：" + money(order->totalAmount) + " 元");
    if (!order->paidAt.empty()) {
        printAt(42, 20, "结账时间：" + order->paidAt);
    }

    if (waitKey) {
        UI::pause();
    }
}

/*
    函数用途：
    显示菜品列表。

    接口说明：
    - title：页面标题，调用方可以传入“菜品信息表”等不同标题。

    学习重点：
    这是典型的链表遍历输出：
    for (DishNode* p = g_dishes; p != NULL; p = p->next)
*/
void showDishListPage(const std::string& title) {
    drawWorkPage(title);
    printLine(7, "编号     名称                 分类       价格       库存       已售");
    printLine(8, "------------------------------------------------------------------");

    // 菜品链表逐项输出；页面高度有限，超过页面的记录暂不分页。
    int y = 9;
    int count = 0;
    for (DishNode* p = g_dishes; p != NULL && y <= 21; p = p->next, ++y) {
        std::ostringstream row;
        row << std::setw(8) << std::left << p->id
            << std::setw(21) << std::left << p->name
            << std::setw(11) << std::left << p->category
            << std::setw(11) << std::left << money(p->price)
            << std::setw(11) << std::left << p->stock
            << p->sold;
        printLine(y, row.str());
        ++count;
    }
    if (count == 0) {
        UI::printCenter(13, "暂无菜品信息。");
    }
    UI::pause();
}

/*
    函数用途：
    显示餐桌状态列表。

    接口说明：
    - title：页面标题。

    业务注意：
    餐桌状态和当前金额来自 TableNode。
    activeOrderId 用来提示该餐桌当前关联的是哪一个订单。
*/
void showTableListPage(const std::string& title) {
    drawWorkPage(title);
    printLine(7, "餐桌号   容量       状态       当前消费       关联订单");
    printLine(8, "------------------------------------------------------");

    // 餐桌状态来自 TableNode.status，当前金额来自关联订单汇总。
    int y = 9;
    int count = 0;
    for (TableNode* p = g_tables; p != NULL && y <= 21; p = p->next, ++y) {
        std::ostringstream row;
        row << std::setw(9) << std::left << p->id
            << std::setw(11) << std::left << (std::to_string(p->capacity) + "人")
            << std::setw(11) << std::left << tableStatusName(p->status)
            << std::setw(15) << std::left << money(p->currentAmount)
            << p->activeOrderId;
        printLine(y, row.str());
        ++count;
    }
    if (count == 0) {
        UI::printCenter(13, "暂无餐桌信息。");
    }
    UI::pause();
}

/*
    函数用途：
    显示用户列表。

    学习重点：
    roleName 会把内部保存的 "admin" / "waiter" 转成中文显示。
*/
void showUserListPage() {
    drawWorkPage("用户列表");
    printLine(7, "账号                 姓名                 角色");
    printLine(8, "------------------------------------------------");

    int y = 9;
    int count = 0;
    for (UserNode* p = g_users; p != NULL && y <= 21; p = p->next, ++y) {
        std::ostringstream row;
        row << std::setw(21) << std::left << p->username
            << std::setw(21) << std::left << p->name
            << roleName(p->role);
        printLine(y, row.str());
        ++count;
    }
    if (count == 0) {
        UI::printCenter(13, "暂无用户信息。");
    }
    UI::pause();
}

/*
    函数用途：
    显示订单列表。

    业务注意：
    这里会显示历史订单和已取消订单，但营收统计只统计已结账订单。
*/
void showOrderListPage() {
    drawWorkPage("订单列表");
    printLine(7, "订单号   餐桌   服务员           状态       金额       创建时间");
    printLine(8, "----------------------------------------------------------------");

    // 订单列表包含历史订单，已取消订单也会展示，但不参与营收。
    int y = 9;
    int count = 0;
    for (OrderNode* p = g_orders; p != NULL && y <= 21; p = p->next, ++y) {
        std::ostringstream row;
        row << std::setw(9) << std::left << p->orderId
            << std::setw(7) << std::left << p->tableId
            << std::setw(17) << std::left << p->waiter
            << std::setw(11) << std::left << orderStatusName(p->status)
            << std::setw(11) << std::left << money(p->totalAmount)
            << p->createdAt;
        printLine(y, row.str());
        ++count;
    }
    if (count == 0) {
        UI::printCenter(13, "暂无订单信息。");
    }
    UI::pause();
}

/*
    函数用途：
    查找订单链表中的最后一笔订单。

    返回值：
    - 有订单：返回最后一个订单节点。
    - 没有订单：返回 NULL。

    学习重点：
    这个函数没有按订单号查找，而是沿着 next 一直走到链表末尾。
*/
OrderNode* latestOrder() {
    OrderNode* last = NULL;
    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        last = p;
    }
    return last;
}

/*
    函数用途：
    兼容第一版函数名，显示真实菜品列表。
*/
void showDishTableDemo() {
    // 兼容第一版函数名：现在显示的是链表和文件中的真实菜品数据。
    ensureDataLoaded();
    showDishListPage("菜品信息表");
}

/*
    函数用途：
    兼容第一版函数名，显示真实餐桌状态列表。
*/
void showTableStatusDemo() {
    // 兼容第一版函数名：现在显示的是真实餐桌状态。
    ensureDataLoaded();
    showTableListPage("餐桌状态表");
}

/*
    函数用途：
    兼容第一版函数名，显示最新订单小票。
*/
void showOrderReceiptDemo() {
    // 兼容第一版函数名：默认展示最新一笔订单的小票。
    ensureDataLoaded();
    OrderNode* order = latestOrder();
    if (order == NULL) {
        UI::showError("暂无订单信息。");
        return;
    }
    showReceiptForOrder(order, true);
}

/*
    函数用途：
    第一版遗留的占位提示函数。

    接口说明：
    - moduleName：模块名称。
*/
void showTodoPage(const std::string& moduleName) {
    // 第一版保留的占位函数。当前版本功能已接入，只用于兜底提示。
    UI::showMessage("模块提示", moduleName + "模块已接入业务菜单，请从对应菜单进入。");
}
