#include "app_internal.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>

// 本文件只负责“链表 <-> 文本文件”的转换。
// 每种数据对应一个 txt 文件，字段用 | 分隔，便于课程设计展示持久化过程。

bool loadUsers() {
    std::ifstream in("users.txt");
    if (!in) {
        return false;
    }

    clearUsers();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        // 用户文件格式：账号|密码|角色|姓名
        std::vector<std::string> parts = split(line, '|');
        if (parts.size() >= 4) {
            appendUser(makeUser(parts[0], parts[1], parts[2], parts[3]));
        }
    }
    return g_users != NULL;
}

bool loadDishes() {
    std::ifstream in("dishes.txt");
    if (!in) {
        return false;
    }

    clearDishes();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        // 菜品文件格式：编号|名称|分类|价格|库存|累计销量
        std::vector<std::string> parts = split(line, '|');
        if (parts.size() >= 6) {
            appendDish(makeDish(std::atoi(parts[0].c_str()), parts[1], parts[2],
            std::atof(parts[3].c_str()), std::atoi(parts[4].c_str()),
            std::atoi(parts[5].c_str())));
        }
    }
    return g_dishes != NULL;
}

bool loadTables() {
    std::ifstream in("tables.txt");
    if (!in) {
        return false;
    }

    clearTables();
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        // 餐桌文件格式：餐桌号|容量|状态|当前金额|关联订单号
        std::vector<std::string> parts = split(line, '|');
        if (parts.size() >= 5) {
            appendTable(makeTable(std::atoi(parts[0].c_str()), std::atoi(parts[1].c_str()),
            std::atoi(parts[2].c_str()), std::atof(parts[3].c_str()),
            std::atoi(parts[4].c_str())));
        }
    }
    return g_tables != NULL;
}

bool loadOrders() {
    std::ifstream in("orders.txt");
    if (!in) {
        return false;
    }

    clearOrders();
    std::string line;
    int maxId = 0;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        // 订单文件格式：订单号|餐桌号|服务员|状态|总金额|创建时间|结账时间
        std::vector<std::string> parts = split(line, '|');
        if (parts.size() >= 7) {
            int id = std::atoi(parts[0].c_str());
            appendOrder(makeOrder(id, std::atoi(parts[1].c_str()), parts[2],
            std::atoi(parts[3].c_str()), std::atof(parts[4].c_str()),
            parts[5], parts[6]));
            if (id > maxId) {
                maxId = id;
            }
        }
    }
    // 订单号根据文件中最大订单号继续递增，避免重启程序后订单号重复。
    g_nextOrderId = maxId + 1;
    return true;
}

bool loadOrderItems() {
    std::ifstream in("order_items.txt");
    if (!in) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        // 订单明细文件格式：订单号|菜品编号|菜品名称|数量|单价|小计
        std::vector<std::string> parts = split(line, '|');
        if (parts.size() >= 6) {
            OrderNode* order = findOrder(std::atoi(parts[0].c_str()));
            if (order != NULL) {
                appendOrderItem(order, makeOrderItem(std::atoi(parts[1].c_str()), parts[2],
                std::atoi(parts[3].c_str()),
                std::atof(parts[4].c_str()),
                std::atof(parts[5].c_str())));
            }
        }
    }
    return true;
}

void saveUsers() {
    std::ofstream out("users.txt");
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        out << p->username << '|' << p->password << '|' << p->role << '|' << p->name << '\n';
    }
}

void saveDishes() {
    std::ofstream out("dishes.txt");
    for (DishNode* p = g_dishes; p != NULL; p = p->next) {
        out << p->id << '|' << p->name << '|' << p->category << '|'
        << std::fixed << std::setprecision(2) << p->price << '|'
        << p->stock << '|' << p->sold << '\n';
    }
}

void saveTables() {
    std::ofstream out("tables.txt");
    for (TableNode* p = g_tables; p != NULL; p = p->next) {
        out << p->id << '|' << p->capacity << '|' << p->status << '|'
        << std::fixed << std::setprecision(2) << p->currentAmount << '|'
        << p->activeOrderId << '\n';
    }
}

void saveOrders() {
    std::ofstream out("orders.txt");
    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        out << p->orderId << '|' << p->tableId << '|' << p->waiter << '|'
        << p->status << '|' << std::fixed << std::setprecision(2) << p->totalAmount
        << '|' << p->createdAt << '|' << p->paidAt << '\n';
    }
}

void saveOrderItems() {
    std::ofstream out("order_items.txt");
    for (OrderNode* order = g_orders; order != NULL; order = order->next) {
        // 一个订单可能有多条明细，所以这里是两层链表遍历。
        for (OrderItemNode* item = order->items; item != NULL; item = item->next) {
            out << order->orderId << '|' << item->dishId << '|' << item->dishName << '|'
            << item->quantity << '|' << std::fixed << std::setprecision(2)
            << item->price << '|' << item->subtotal << '\n';
        }
    }
}

void saveAll() {
    // 所有修改类操作完成后都调用 saveAll，保证异常退出时数据丢失尽量少。
    saveUsers();
    saveDishes();
    saveTables();
    saveOrders();
    saveOrderItems();
}

void ensureDataLoaded() {
    if (g_loaded) {
        return;
    }

    bool changed = false;
    // 首次运行没有数据文件时，自动写入默认账号、菜品和餐桌。
    if (!loadUsers()) {
        seedUsers();
        changed = true;
    }
    if (!loadDishes()) {
        seedDishes();
        changed = true;
    }
    if (!loadTables()) {
        seedTables();
        changed = true;
    }
    // 订单文件可以为空，表示当前没有历史订单。
    loadOrders();
    loadOrderItems();

    if (g_nextOrderId < 1) {
        g_nextOrderId = 1;
    }
    if (changed) {
        saveAll();
    }
    g_loaded = true;
}
