#include "app_internal.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>

// 本文件只负责“链表 <-> 文本文件”的转换。
// 每种数据对应一个 txt 文件，字段用 | 分隔，便于课程设计展示持久化过程。

/*
    函数用途：
    从 users.txt 读取用户数据，恢复用户链表。

    接口说明：
    - 无参数。
    - 返回 true：文件存在，并且至少读取到一个用户。
    - 返回 false：文件不存在，或没有有效用户数据。

    文件格式：
    账号|密码|角色|姓名

    语法说明：
    - std::ifstream 是输入文件流，用来读文件。
    - std::getline(in, line) 每次读取一整行。
*/
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

/*
    函数用途：
    从 dishes.txt 读取菜品数据，恢复菜品链表。

    文件格式：
    编号|名称|分类|价格|库存|累计销量

    语法说明：
    - std::atoi 把字符串转换为 int。
    - std::atof 把字符串转换为 double。
*/
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

/*
    函数用途：
    从 tables.txt 读取餐桌数据，恢复餐桌链表。

    文件格式：
    餐桌号|容量|状态|当前金额|关联订单号
*/
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

/*
    函数用途：
    从 orders.txt 读取订单主表数据，恢复订单链表。

    文件格式：
    订单号|餐桌号|服务员|状态|总金额|创建时间|结账时间

    业务注意：
    读取订单时会记录最大订单号，随后把 g_nextOrderId 设置成 maxId + 1，
    这样程序重启后不会生成重复订单号。
*/
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

/*
    函数用途：
    从 order_items.txt 读取订单明细数据，并挂回对应订单。

    文件格式：
    订单号|菜品编号|菜品名称|数量|单价|小计

    业务注意：
    明细必须先找到对应的订单主节点，才能追加到 order->items 子链表。
    因此加载顺序必须是先 loadOrders，再 loadOrderItems。
*/
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

/*
    函数用途：
    把用户链表写入 users.txt。

    语法说明：
    - std::ofstream 是输出文件流，用来写文件。
    - 默认打开方式会覆盖原文件内容。
*/
void saveUsers() {
    std::ofstream out("users.txt");
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        out << p->username << '|' << p->password << '|' << p->role << '|' << p->name << '\n';
    }
}

/*
    函数用途：
    把菜品链表写入 dishes.txt。

    注意：
    价格使用 fixed + setprecision(2) 保存为两位小数，方便下次读取和人工查看。
*/
void saveDishes() {
    std::ofstream out("dishes.txt");
    for (DishNode* p = g_dishes; p != NULL; p = p->next) {
        out << p->id << '|' << p->name << '|' << p->category << '|'
            << std::fixed << std::setprecision(2) << p->price << '|'
            << p->stock << '|' << p->sold << '\n';
    }
}

/*
    函数用途：
    把餐桌链表写入 tables.txt。
*/
void saveTables() {
    std::ofstream out("tables.txt");
    for (TableNode* p = g_tables; p != NULL; p = p->next) {
        out << p->id << '|' << p->capacity << '|' << p->status << '|'
            << std::fixed << std::setprecision(2) << p->currentAmount << '|'
            << p->activeOrderId << '\n';
    }
}

/*
    函数用途：
    把订单主链表写入 orders.txt。

    注意：
    这里只保存订单主信息，不保存每一道菜的明细。
    明细由 saveOrderItems 单独保存。
*/
void saveOrders() {
    std::ofstream out("orders.txt");
    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        out << p->orderId << '|' << p->tableId << '|' << p->waiter << '|'
            << p->status << '|' << std::fixed << std::setprecision(2) << p->totalAmount
            << '|' << p->createdAt << '|' << p->paidAt << '\n';
    }
}

/*
    函数用途：
    把所有订单的明细子链表写入 order_items.txt。

    链表知识点：
    这里需要两层循环：
    - 外层遍历订单链表。
    - 内层遍历每个订单的 items 明细子链表。
*/
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

/*
    函数用途：
    一次性保存全部业务数据。

    使用约定：
    只要业务操作修改了链表数据，就应该在流程结束时调用 saveAll。
*/
void saveAll() {
    // 所有修改类操作完成后都调用 saveAll，保证异常退出时数据丢失尽量少。
    saveUsers();
    saveDishes();
    saveTables();
    saveOrders();
    saveOrderItems();
}

/*
    函数用途：
    确保程序启动后只加载一次数据。

    工作流程：
    1. 如果 g_loaded 已经是 true，直接返回。
    2. 尝试加载用户、菜品、餐桌。
    3. 如果基础数据文件不存在，就写入演示数据。
    4. 加载订单主表和订单明细。
    5. 必要时保存新生成的演示数据。

    业务注意：
    g_loaded 是全局加载标记，避免每次打开页面都重复读文件、重复创建链表。
*/
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
