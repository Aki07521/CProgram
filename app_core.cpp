#include "app_internal.hpp"
#include "ui.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// 本文件保存“核心数据 + 链表算法 + 订单通用规则”。
// 学习时建议先看 app_internal.hpp 中的数据结构，再回到这里看节点如何创建、查找和释放。

// 四个全局链表头指针，对应用户、菜品、餐桌和订单四类核心数据。
UserNode* g_users = NULL;
DishNode* g_dishes = NULL;
TableNode* g_tables = NULL;
OrderNode* g_orders = NULL;

// g_loaded 防止重复从文件加载；g_nextOrderId 用于生成不重复订单号。
// g_currentUsername/g_currentRole 保存当前登录用户，供统计和权限判断使用。
bool g_loaded = false;
int g_nextOrderId = 1;
std::string g_currentUsername;
std::string g_currentRole;


void printAt(int x, int y, const std::string& text) {
    UI::gotoXY(x, y);
    std::cout << text;
}

// 绘制统一工作页面：上方标题 + 中间内容边框。
void drawWorkPage(const std::string& title) {
    UI::drawHeader(title);
    UI::drawBox(LEFT, 5, WIDTH, 18);
}

void printLine(int y, const std::string& text) {
    printAt(14, y, text);
}

// 读取文本文件时按分隔符拆字段，例如 users.txt 中一行：
// admin|123456|admin|系统管理员
std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::string part;
    std::stringstream ss(line);
    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

// 金额统一保留两位小数，保证表格、小票和文件中的格式一致。
std::string money(double value) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

// 获取当前时间字符串，用于订单创建时间和结账时间。
std::string nowText() {
    std::time_t t = std::time(NULL);
    std::tm* local = std::localtime(&t);
    char buffer[32] = {0};
    if (local != NULL) {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
    }
    return buffer;
}

std::string roleName(const std::string& role) {
    return role == "admin" ? "管理员" : "服务员";
}

std::string tableStatusName(int status) {
    if (status == TABLE_IDLE) {
        return "空闲";
    }
    if (status == TABLE_USING) {
        return "使用中";
    }
    return "待结账";
}

std::string orderStatusName(int status) {
    if (status == ORDER_ACTIVE) {
        return "进行中";
    }
    if (status == ORDER_WAIT_PAY) {
        return "待结账";
    }
    if (status == ORDER_PAID) {
        return "已结账";
    }
    return "已取消";
}

// makeXxx 函数统一负责创建链表节点，并把 next 初始化为空指针。
UserNode* makeUser(const std::string& username, const std::string& password,
                   const std::string& role, const std::string& name) {
    UserNode* node = new UserNode;
    node->username = username;
    node->password = password;
    node->role = role;
    node->name = name;
    node->next = NULL;
    return node;
}

DishNode* makeDish(int id, const std::string& name, const std::string& category,
                   double price, int stock, int sold) {
    DishNode* node = new DishNode;
    node->id = id;
    node->name = name;
    node->category = category;
    node->price = price;
    node->stock = stock;
    node->sold = sold;
    node->next = NULL;
    return node;
}

TableNode* makeTable(int id, int capacity, int status, double amount, int orderId) {
    TableNode* node = new TableNode;
    node->id = id;
    node->capacity = capacity;
    node->status = status;
    node->currentAmount = amount;
    node->activeOrderId = orderId;
    node->next = NULL;
    return node;
}

OrderNode* makeOrder(int orderId, int tableId, const std::string& waiter, int status,
                     double total, const std::string& createdAt, const std::string& paidAt) {
    OrderNode* node = new OrderNode;
    node->orderId = orderId;
    node->tableId = tableId;
    node->waiter = waiter;
    node->status = status;
    node->totalAmount = total;
    node->createdAt = createdAt;
    node->paidAt = paidAt;
    node->items = NULL;
    node->next = NULL;
    return node;
}

OrderItemNode* makeOrderItem(int dishId, const std::string& dishName, int quantity,
                             double price, double subtotal) {
    OrderItemNode* node = new OrderItemNode;
    node->dishId = dishId;
    node->dishName = dishName;
    node->quantity = quantity;
    node->price = price;
    node->subtotal = subtotal;
    node->next = NULL;
    return node;
}

// appendXxx 函数把新节点追加到链表尾部。
// 课程设计要求使用链表，新增数据时不移动原有节点，只修改 next 指针。
void appendUser(UserNode* node) {
    if (g_users == NULL) {
        g_users = node;
        return;
    }
    UserNode* p = g_users;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = node;
}

void appendDish(DishNode* node) {
    if (g_dishes == NULL) {
        g_dishes = node;
        return;
    }
    DishNode* p = g_dishes;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = node;
}

void appendTable(TableNode* node) {
    if (g_tables == NULL) {
        g_tables = node;
        return;
    }
    TableNode* p = g_tables;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = node;
}

void appendOrder(OrderNode* node) {
    if (g_orders == NULL) {
        g_orders = node;
        return;
    }
    OrderNode* p = g_orders;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = node;
}

void appendOrderItem(OrderNode* order, OrderItemNode* item) {
    if (order->items == NULL) {
        order->items = item;
        return;
    }
    OrderItemNode* p = order->items;
    while (p->next != NULL) {
        p = p->next;
    }
    p->next = item;
}

// clearXxx 函数用于释放链表内存，主要在重新加载或重置演示数据时使用。
void clearUsers() {
    while (g_users != NULL) {
        UserNode* next = g_users->next;
        delete g_users;
        g_users = next;
    }
}

void clearDishes() {
    while (g_dishes != NULL) {
        DishNode* next = g_dishes->next;
        delete g_dishes;
        g_dishes = next;
    }
}

void clearTables() {
    while (g_tables != NULL) {
        TableNode* next = g_tables->next;
        delete g_tables;
        g_tables = next;
    }
}

// 订单明细是订单节点内部的子链表，因此清理订单时要先清理明细链表。
void clearOrderItems(OrderItemNode*& head) {
    while (head != NULL) {
        OrderItemNode* next = head->next;
        delete head;
        head = next;
    }
}

void clearOrders() {
    while (g_orders != NULL) {
        OrderNode* next = g_orders->next;
        clearOrderItems(g_orders->items);
        delete g_orders;
        g_orders = next;
    }
}

// 清空所有链表，并把订单号重新从 1 开始。
void clearAllData() {
    clearUsers();
    clearDishes();
    clearTables();
    clearOrders();
    g_nextOrderId = 1;
}

// findXxx 函数按主键顺序遍历链表，找到则返回节点指针，找不到返回 NULL。
// 返回指针后可以直接修改节点字段，因此增删改查都能复用这些查找函数。
UserNode* findUser(const std::string& username) {
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        if (p->username == username) {
            return p;
        }
    }
    return NULL;
}

DishNode* findDish(int id) {
    for (DishNode* p = g_dishes; p != NULL; p = p->next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

TableNode* findTable(int id) {
    for (TableNode* p = g_tables; p != NULL; p = p->next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

OrderNode* findOrder(int id) {
    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        if (p->orderId == id) {
            return p;
        }
    }
    return NULL;
}

// 餐桌和订单之间通过 activeOrderId 关联。
// 如果餐桌记录异常缺少 activeOrderId，则再从订单链表中按餐桌号兜底查找。
OrderNode* findOrderByTable(int tableId) {
    TableNode* table = findTable(tableId);
    if (table != NULL && table->activeOrderId > 0) {
        return findOrder(table->activeOrderId);
    }

    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        if (p->tableId == tableId && (p->status == ORDER_ACTIVE || p->status == ORDER_WAIT_PAY)) {
            return p;
        }
    }
    return NULL;
}

// 在指定订单的明细链表中查找某一道菜。
OrderItemNode* findOrderItem(OrderNode* order, int dishId) {
    if (order == NULL) {
        return NULL;
    }
    for (OrderItemNode* p = order->items; p != NULL; p = p->next) {
        if (p->dishId == dishId) {
            return p;
        }
    }
    return NULL;
}

// 删除或修改管理员角色时使用，防止系统没有任何管理员账号。
int countAdmins() {
    int count = 0;
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        if (p->role == "admin") {
            ++count;
        }
    }
    return count;
}

// seedXxx 函数提供首次运行时的演示数据。
// 如果对应 txt 文件不存在或为空，系统会自动写入这些默认数据。
void seedUsers() {
    clearUsers();
    appendUser(makeUser("admin", "123456", "admin", "系统管理员"));
    appendUser(makeUser("waiter", "123456", "waiter", "默认服务员"));
}

void seedDishes() {
    clearDishes();
    appendDish(makeDish(1001, "宫保鸡丁", "热菜", 28.0, 50, 0));
    appendDish(makeDish(1002, "鱼香肉丝", "热菜", 26.0, 40, 0));
    appendDish(makeDish(1003, "米饭", "主食", 2.0, 200, 0));
    appendDish(makeDish(1004, "可乐", "饮品", 5.0, 80, 0));
    appendDish(makeDish(1005, "西红柿鸡蛋汤", "汤品", 18.0, 30, 0));
}

void seedTables() {
    clearTables();
    appendTable(makeTable(1, 4, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(2, 6, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(3, 4, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(4, 8, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(5, 2, TABLE_IDLE, 0.0, 0));
}

// 以下 load/save 函数把链表数据持久化为文本文件，字段之间用 | 分隔。

bool checkAccount(const std::string& role, const std::string& username, const std::string& password) {
    ensureDataLoaded();
    UserNode* user = findUser(username);
    // 登录时不仅验证账号密码，还验证用户角色，保证管理员和服务员进入不同菜单。
    return user != NULL && user->password == password && user->role == role;
}

std::string currentDisplayName() {
    UserNode* user = findUser(g_currentUsername);
    if (user != NULL && !user->name.empty()) {
        return user->name;
    }
    return g_currentUsername;
}

std::string readRequiredString(const std::string& label) {
    while (true) {
        std::string value = UI::inputString(label);
        if (!value.empty()) {
            return value;
        }
        // 字符串输入统一要求非空，避免生成账号为空、菜名为空等无效数据。
        std::cout << "内容不能为空，请重新输入。" << std::endl;
    }
}

double inputDouble(const std::string& label, double minValue, double maxValue) {
    while (true) {
        std::string text = UI::inputString(label);
        std::stringstream ss(text);
        double value = 0.0;
        char extra = '\0';
        if ((ss >> value) && !(ss >> extra) && value >= minValue && value <= maxValue) {
            return value;
        }
        std::cout << "输入错误，请输入 " << money(minValue) << " 到 " << money(maxValue)
        << " 之间的数字。" << std::endl;
    }
}

int inputExistingDishId(const std::string& label, bool allowZero) {
    while (true) {
        int minValue = allowZero ? 0 : 1;
        int id = UI::inputInt(label, minValue, 999999);
        if (allowZero && id == 0) {
            // 部分业务流程中 0 表示返回或结束点菜。
            return 0;
        }
        if (findDish(id) != NULL) {
            return id;
        }
        std::cout << "没有找到该菜品编号，请重新输入。" << std::endl;
    }
}

int inputExistingTableId(const std::string& label, bool allowZero) {
    while (true) {
        int minValue = allowZero ? 0 : 1;
        int id = UI::inputInt(label, minValue, 999999);
        if (allowZero && id == 0) {
            return 0;
        }
        if (findTable(id) != NULL) {
            return id;
        }
        std::cout << "没有找到该餐桌号，请重新输入。" << std::endl;
    }
}

void recalcOrderTotal(OrderNode* order) {
    if (order == NULL) {
        return;
    }

    // 每次加菜、退菜、改数量后都重新计算订单总金额，避免金额不同步。
    double total = 0.0;
    for (OrderItemNode* item = order->items; item != NULL; item = item->next) {
        item->subtotal = item->price * item->quantity;
        total += item->subtotal;
    }
    order->totalAmount = total;

    TableNode* table = findTable(order->tableId);
    if (table != NULL && table->activeOrderId == order->orderId) {
        // 餐桌页面显示的是当前消费金额，需要和订单总额保持一致。
        table->currentAmount = total;
    }
}

bool orderHasItems(OrderNode* order) {
    return order != NULL && order->items != NULL;
}

// 点餐时同步扣减库存并记录销量，退菜或取消订单时会做反向恢复。
void addDishToOrder(OrderNode* order, DishNode* dish, int quantity) {
    if (order == NULL || dish == NULL || quantity <= 0) {
        return;
    }

    OrderItemNode* item = findOrderItem(order, dish->id);
    if (item == NULL) {
        appendOrderItem(order, makeOrderItem(dish->id, dish->name, quantity, dish->price,
        dish->price * quantity));
    } else {
        item->quantity += quantity;
    }
    dish->stock -= quantity;
    dish->sold += quantity;
    recalcOrderTotal(order);
}

bool removeOrReduceItem(OrderNode* order, int dishId, int quantity) {
    if (order == NULL || quantity <= 0) {
        return false;
    }

    // 删除链表节点时需要保留前驱指针 prev，便于修改 prev->next。
    OrderItemNode* prev = NULL;
    OrderItemNode* item = order->items;
    while (item != NULL && item->dishId != dishId) {
        prev = item;
        item = item->next;
    }
    if (item == NULL || quantity > item->quantity) {
        return false;
    }

    DishNode* dish = findDish(dishId);
    if (dish != NULL) {
        // 退菜或减少数量时恢复库存，同时扣回累计销量。
        dish->stock += quantity;
        dish->sold -= quantity;
        if (dish->sold < 0) {
            dish->sold = 0;
        }
    }

    item->quantity -= quantity;
    if (item->quantity == 0) {
        if (prev == NULL) {
            order->items = item->next;
        } else {
            prev->next = item->next;
        }
        delete item;
    }

    recalcOrderTotal(order);
    return true;
}

void restoreOrderStock(OrderNode* order) {
    if (order == NULL || order->status == ORDER_PAID) {
        return;
    }

    // 取消未结账订单时，把订单中的所有菜品数量退回库存。
    for (OrderItemNode* item = order->items; item != NULL; item = item->next) {
        DishNode* dish = findDish(item->dishId);
        if (dish != NULL) {
            dish->stock += item->quantity;
            dish->sold -= item->quantity;
            if (dish->sold < 0) {
                dish->sold = 0;
            }
        }
    }
}

void cancelOrder(OrderNode* order) {
    if (order == NULL || order->status == ORDER_PAID) {
        return;
    }

    // 已取消订单保留在订单链表中，便于订单查询；但金额清零且不计入营收。
    restoreOrderStock(order);
    order->status = ORDER_CANCELLED;
    order->totalAmount = 0.0;

    TableNode* table = findTable(order->tableId);
    if (table != NULL && table->activeOrderId == order->orderId) {
        table->status = TABLE_IDLE;
        table->currentAmount = 0.0;
        table->activeOrderId = 0;
    }
}

void updateEmptyOrder(OrderNode* order) {
    // 如果订单中的菜全部被删除，则自动取消订单并释放餐桌。
    if (order != NULL && order->items == NULL && order->status != ORDER_PAID) {
        cancelOrder(order);
    }
}

bool dishInUnpaidOrder(int dishId) {
    // 未结账订单中还在使用的菜品不能删除，否则订单明细会失去对应菜品。
    for (OrderNode* order = g_orders; order != NULL; order = order->next) {
        if (order->status == ORDER_ACTIVE || order->status == ORDER_WAIT_PAY) {
            if (findOrderItem(order, dishId) != NULL) {
                return true;
            }
        }
    }
    return false;
}

