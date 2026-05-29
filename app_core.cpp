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


/*
    函数用途：
    在控制台指定坐标输出一段文字。

    接口说明：
    - x：横坐标，表示第几列。
    - y：纵坐标，表示第几行。
    - text：要输出的文字。

    语法说明：
    - const std::string& 表示“以引用方式传入字符串，并且函数内部不修改它”。
    - 这样可以避免复制整段字符串，效率更高。
*/
void printAt(int x, int y, const std::string& text) {
    UI::gotoXY(x, y);
    std::cout << text;
}

/*
    函数用途：
    绘制业务页面的统一框架：顶部标题 + 中间内容区域。

    接口说明：
    - title：页面标题，例如“用户管理”“开台点餐”。

    用法示例：
    drawWorkPage("新增用户");
*/
void drawWorkPage(const std::string& title) {
    UI::drawHeader(title);
    UI::drawBox(LEFT, 5, WIDTH, 18);
}

/*
    函数用途：
    在业务内容区输出一行文字。

    接口说明：
    - y：输出所在行。
    - text：要输出的文字。

    说明：
    这里把横坐标固定为 14，是为了让业务页面内容左边对齐。
*/
void printLine(int y, const std::string& text) {
    printAt(14, y, text);
}

/*
    函数用途：
    按指定分隔符拆分一行文本。

    接口说明：
    - line：原始文本，例如 "admin|123456|admin|系统管理员"。
    - delimiter：分隔符，这里常用 '|'。
    - 返回值：拆分后的字符串数组。

    语法说明：
    - std::stringstream 可以把字符串当成输入流来读。
    - std::getline(ss, part, delimiter) 表示从 ss 中读到 delimiter 为止。

    用法示例：
    std::vector<std::string> parts = split(line, '|');
*/
std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> parts;
    std::string part;
    std::stringstream ss(line);
    while (std::getline(ss, part, delimiter)) {
        parts.push_back(part);
    }
    return parts;
}

/*
    函数用途：
    把 double 类型金额转换成保留两位小数的字符串。

    接口说明：
    - value：金额数值。
    - 返回值：格式化后的字符串，例如 28 会变成 "28.00"。

    语法说明：
    - std::fixed 表示使用普通小数格式，不用科学计数法。
    - std::setprecision(2) 表示小数点后保留 2 位。
*/
std::string money(double value) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

/*
    函数用途：
    获取当前本地时间，并转换成字符串。

    返回值：
    - 时间格式为 "年-月-日 时:分:秒"，例如 "2026-05-29 14:30:00"。

    语法说明：
    - std::time(NULL) 获取当前时间戳。
    - std::localtime 把时间戳转换成本地时间结构体。
    - std::strftime 按指定格式生成字符串。
*/
std::string nowText() {
    std::time_t t = std::time(NULL);
    std::tm* local = std::localtime(&t);
    char buffer[32] = {0};
    if (local != NULL) {
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
    }
    return buffer;
}

/*
    函数用途：
    把内部角色代码转换成中文显示文本。

    接口说明：
    - role："admin" 表示管理员，其他情况按服务员处理。
    - 返回值："管理员" 或 "服务员"。

    语法说明：
    - 条件运算符 A ? B : C 表示如果 A 成立返回 B，否则返回 C。
*/
std::string roleName(const std::string& role) {
    return role == "admin" ? "管理员" : "服务员";
}

/*
    函数用途：
    把餐桌状态数字转换成中文显示文本。

    接口说明：
    - status：TABLE_IDLE / TABLE_USING / TABLE_WAIT_PAY。
    - 返回值：页面上显示的状态文字。
*/
std::string tableStatusName(int status) {
    if (status == TABLE_IDLE) {
        return "空闲";
    }
    if (status == TABLE_USING) {
        return "使用中";
    }
    return "待结账";
}

/*
    函数用途：
    把订单状态数字转换成中文显示文本。

    接口说明：
    - status：ORDER_ACTIVE / ORDER_WAIT_PAY / ORDER_PAID / ORDER_CANCELLED。
    - 返回值：页面上显示的状态文字。
*/
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

/*
    makeXxx 系列函数用途：
    创建一类链表节点，填入字段，并把 next 指针初始化为空。

    为什么要统一写 makeXxx？
    - 避免每次 new 节点时都重复写初始化代码。
    - 避免忘记把 next 设置为 NULL。
    - 让其他业务代码只关心“创建什么数据”，不用关心节点细节。

    语法说明：
    - new UserNode 会在堆区创建一个 UserNode 对象，并返回它的地址。
    - UserNode* node 表示 node 是一个指向 UserNode 的指针。
    - node->username 等价于 (*node).username，是访问指针所指对象字段的常用写法。
*/
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

/*
    函数用途：
    创建一个菜品节点。

    接口说明：
    - id：菜品编号。
    - name：菜品名称。
    - category：菜品分类。
    - price：菜品单价。
    - stock：库存数量。
    - sold：累计售出数量。
    - 返回值：新建好的菜品节点指针。
*/
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

/*
    函数用途：
    创建一个餐桌节点。

    接口说明：
    - id：餐桌号。
    - capacity：可容纳人数。
    - status：餐桌状态。
    - amount：当前消费金额。
    - orderId：当前关联订单号，0 表示没有关联订单。
*/
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

/*
    函数用途：
    创建一个订单主节点。

    接口说明：
    - orderId：订单号。
    - tableId：餐桌号。
    - waiter：服务员账号。
    - status：订单状态。
    - total：订单总金额。
    - createdAt：创建时间。
    - paidAt：结账时间，未结账时为空字符串。

    注意：
    订单节点内部还有一条子链表 items，用来保存每道菜的明细。
*/
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

/*
    函数用途：
    创建一条订单明细节点。

    接口说明：
    - dishId：菜品编号。
    - dishName：下单时的菜名。
    - quantity：数量。
    - price：下单时单价。
    - subtotal：小计金额。

    注意：
    明细中保存 dishName 和 price，是为了保证历史订单不受后续菜品改名、改价影响。
*/
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

/*
    appendXxx 系列函数用途：
    把一个已经创建好的节点追加到对应链表尾部。

    链表知识点：
    - 头指针为空，说明链表没有任何节点，新节点直接成为第一个节点。
    - 头指针不为空，就从头节点开始沿着 next 找到最后一个节点。
    - 最后一个节点的特点是 p->next == NULL。
    - 追加节点时，只需要让最后一个节点的 next 指向新节点。
*/
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

/*
    函数用途：
    把菜品节点追加到菜品链表尾部。

    接口说明：
    - node：由 makeDish 创建好的菜品节点。
*/
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

/*
    函数用途：
    把餐桌节点追加到餐桌链表尾部。
*/
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

/*
    函数用途：
    把订单节点追加到订单链表尾部。
*/
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

/*
    函数用途：
    把订单明细追加到某个订单的明细子链表尾部。

    接口说明：
    - order：订单主节点。
    - item：要追加的订单明细节点。

    注意：
    这里操作的不是全局 g_orders，而是 order->items 这条子链表。
*/
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

/*
    clearXxx 系列函数用途：
    释放链表中所有节点占用的内存，并把头指针向后移动直到为空。

    语法说明：
    - delete 用来释放 new 出来的对象。
    - 删除当前节点前，必须先保存 next，否则当前节点删掉后就找不到后面的节点了。
*/
void clearUsers() {
    while (g_users != NULL) {
        UserNode* next = g_users->next;
        delete g_users;
        g_users = next;
    }
}

/*
    函数用途：
    清空菜品链表。
*/
void clearDishes() {
    while (g_dishes != NULL) {
        DishNode* next = g_dishes->next;
        delete g_dishes;
        g_dishes = next;
    }
}

/*
    函数用途：
    清空餐桌链表。
*/
void clearTables() {
    while (g_tables != NULL) {
        TableNode* next = g_tables->next;
        delete g_tables;
        g_tables = next;
    }
}

/*
    函数用途：
    清空某个订单里的明细子链表。

    接口说明：
    - head：订单明细链表头指针的引用。

    语法说明：
    - OrderItemNode*& 表示“指针的引用”。
    - 使用引用后，函数内部把 head 改为 NULL，会同步影响调用方的 head。
*/
void clearOrderItems(OrderItemNode*& head) {
    while (head != NULL) {
        OrderItemNode* next = head->next;
        delete head;
        head = next;
    }
}

/*
    函数用途：
    清空订单链表。

    注意：
    每个订单节点内部还有 items 子链表，所以删除订单节点前必须先清空它的订单明细。
*/
void clearOrders() {
    while (g_orders != NULL) {
        OrderNode* next = g_orders->next;
        clearOrderItems(g_orders->items);
        delete g_orders;
        g_orders = next;
    }
}

/*
    函数用途：
    清空所有业务数据，并把下一个订单号重置为 1。

    使用场景：
    - 系统设置里的“重置为演示数据”。
    - 重新初始化系统数据。
*/
void clearAllData() {
    clearUsers();
    clearDishes();
    clearTables();
    clearOrders();
    g_nextOrderId = 1;
}

/*
    findXxx 系列函数用途：
    在链表中按关键字段查找节点。

    返回值规则：
    - 找到：返回目标节点指针。
    - 找不到：返回 NULL。

    重要用法：
    返回的是节点指针，所以调用方可以直接修改该节点内容。
*/
UserNode* findUser(const std::string& username) {
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        if (p->username == username) {
            return p;
        }
    }
    return NULL;
}

/*
    函数用途：
    按菜品编号查找菜品节点。
*/
DishNode* findDish(int id) {
    for (DishNode* p = g_dishes; p != NULL; p = p->next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

/*
    函数用途：
    按餐桌号查找餐桌节点。
*/
TableNode* findTable(int id) {
    for (TableNode* p = g_tables; p != NULL; p = p->next) {
        if (p->id == id) {
            return p;
        }
    }
    return NULL;
}

/*
    函数用途：
    按订单号查找订单节点。
*/
OrderNode* findOrder(int id) {
    for (OrderNode* p = g_orders; p != NULL; p = p->next) {
        if (p->orderId == id) {
            return p;
        }
    }
    return NULL;
}

/*
    函数用途：
    根据餐桌号查找当前未结账订单。

    查找逻辑：
    1. 先通过餐桌节点的 activeOrderId 找订单，这是最快、最准确的方式。
    2. 如果 activeOrderId 异常缺失，再遍历订单链表兜底查找。

    接口说明：
    - tableId：餐桌号。
    - 返回值：当前进行中或待结账的订单；没有则返回 NULL。
*/
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

/*
    函数用途：
    在指定订单的明细链表中查找某一道菜。

    接口说明：
    - order：订单节点。
    - dishId：菜品编号。
    - 返回值：订单明细节点，找不到返回 NULL。
*/
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

/*
    函数用途：
    统计当前用户链表中管理员账号数量。

    使用场景：
    删除管理员或修改管理员角色前调用，防止系统没有任何管理员账号。
*/
int countAdmins() {
    int count = 0;
    for (UserNode* p = g_users; p != NULL; p = p->next) {
        if (p->role == "admin") {
            ++count;
        }
    }
    return count;
}

/*
    seedXxx 系列函数用途：
    生成首次运行时的默认演示数据。

    使用场景：
    如果 users.txt、dishes.txt、tables.txt 不存在或为空，
    ensureDataLoaded 会调用这些函数创建默认账号、菜品和餐桌。
*/
void seedUsers() {
    clearUsers();
    appendUser(makeUser("admin", "123456", "admin", "系统管理员"));
    appendUser(makeUser("waiter", "123456", "waiter", "默认服务员"));
}

/*
    函数用途：
    初始化默认菜品。
*/
void seedDishes() {
    clearDishes();
    appendDish(makeDish(1001, "宫保鸡丁", "热菜", 28.0, 50, 0));
    appendDish(makeDish(1002, "鱼香肉丝", "热菜", 26.0, 40, 0));
    appendDish(makeDish(1003, "米饭", "主食", 2.0, 200, 0));
    appendDish(makeDish(1004, "可乐", "饮品", 5.0, 80, 0));
    appendDish(makeDish(1005, "西红柿鸡蛋汤", "汤品", 18.0, 30, 0));
}

/*
    函数用途：
    初始化默认餐桌。
*/
void seedTables() {
    clearTables();
    appendTable(makeTable(1, 4, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(2, 6, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(3, 4, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(4, 8, TABLE_IDLE, 0.0, 0));
    appendTable(makeTable(5, 2, TABLE_IDLE, 0.0, 0));
}

/*
    函数用途：
    校验登录账号、密码和角色是否匹配。

    接口说明：
    - role：期望登录角色，"admin" 或 "waiter"。
    - username：输入的账号。
    - password：输入的密码。
    - 返回值：匹配返回 true，否则返回 false。

    注意：
    同一个账号必须角色也匹配，管理员入口不能用服务员账号登录。
*/
bool checkAccount(const std::string& role, const std::string& username, const std::string& password) {
    ensureDataLoaded();
    UserNode* user = findUser(username);
    // 登录时不仅验证账号密码，还验证用户角色，保证管理员和服务员进入不同菜单。
    return user != NULL && user->password == password && user->role == role;
}

/*
    函数用途：
    获取当前登录用户的显示姓名。

    返回值：
    - 如果能在用户链表中找到当前用户，并且 name 不为空，返回姓名。
    - 否则返回账号名。
*/
std::string currentDisplayName() {
    UserNode* user = findUser(g_currentUsername);
    if (user != NULL && !user->name.empty()) {
        return user->name;
    }
    return g_currentUsername;
}

/*
    函数用途：
    读取一个非空字符串。

    接口说明：
    - label：输入提示文字。
    - 返回值：用户输入的非空字符串。

    语法说明：
    - while (true) 表示一直循环，直到函数内部 return。
*/
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

/*
    函数用途：
    读取指定范围内的 double 数字。

    接口说明：
    - label：输入提示。
    - minValue：允许的最小值。
    - maxValue：允许的最大值。
    - 返回值：合法的 double 数字。

    语法说明：
    - char extra 用来检测 "12abc" 这种非法输入。
    - 如果 ss >> extra 还能读到内容，说明输入不只是一个纯数字。
*/
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

/*
    函数用途：
    输入一个已经存在的菜品编号。

    接口说明：
    - label：输入提示。
    - allowZero：是否允许输入 0。
    - 返回值：合法菜品编号；如果 allowZero 为 true，输入 0 会直接返回 0。

    使用场景：
    点餐时输入 0 可以表示“结束点餐”或“返回”。
*/
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

/*
    函数用途：
    输入一个已经存在的餐桌号。

    接口说明：
    - label：输入提示。
    - allowZero：是否允许输入 0。
    - 返回值：合法餐桌号；如果 allowZero 为 true，输入 0 会直接返回 0。
*/
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

/*
    函数用途：
    重新计算订单总金额，并同步餐桌当前消费金额。

    接口说明：
    - order：要重新计算的订单。

    业务规则：
    - 订单总额等于所有订单明细小计之和。
    - 如果餐桌当前关联的正是这个订单，餐桌 currentAmount 也要同步更新。
*/
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

/*
    函数用途：
    判断订单中是否至少有一条菜品明细。

    返回值：
    - true：订单存在，并且 items 不为空。
    - false：订单不存在，或订单没有任何明细。
*/
bool orderHasItems(OrderNode* order) {
    return order != NULL && order->items != NULL;
}

/*
    函数用途：
    把菜品加入订单，并同步扣库存、加销量、重算金额。

    接口说明：
    - order：目标订单。
    - dish：要加入的菜品。
    - quantity：加入数量。

    业务规则：
    - 如果订单中没有这道菜，新建一条订单明细。
    - 如果订单中已有这道菜，只增加数量。
    - 点菜成功后，菜品库存减少，累计销量增加。
*/
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

/*
    函数用途：
    从订单中删除或减少某道菜的数量。

    接口说明：
    - order：目标订单。
    - dishId：要减少的菜品编号。
    - quantity：减少数量。
    - 返回值：操作成功返回 true，失败返回 false。

    链表知识点：
    删除订单明细节点时，需要 prev 和 item 两个指针。
    - prev 指向当前节点的前一个节点。
    - item 指向当前要检查或删除的节点。
    如果删除的是第一个明细，prev 为 NULL，需要单独修改 order->items。
*/
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

/*
    函数用途：
    取消未结账订单时，把订单中所有菜品数量恢复到库存。

    接口说明：
    - order：要恢复库存的订单。

    注意：
    已结账订单不能恢复库存，因为它已经是真实消费记录。
*/
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

/*
    函数用途：
    取消订单，并释放它占用的餐桌。

    业务规则：
    - 已结账订单不能取消。
    - 未结账订单取消时要恢复库存。
    - 订单状态改成 ORDER_CANCELLED。
    - 餐桌状态恢复为空闲。
*/
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

/*
    函数用途：
    检查订单是否已经没有任何菜品，如果为空则自动取消订单。

    使用场景：
    修改订单、删除菜品、退菜之后调用。
*/
void updateEmptyOrder(OrderNode* order) {
    // 如果订单中的菜全部被删除，则自动取消订单并释放餐桌。
    if (order != NULL && order->items == NULL && order->status != ORDER_PAID) {
        cancelOrder(order);
    }
}

/*
    函数用途：
    判断某道菜是否还存在于未结账订单中。

    接口说明：
    - dishId：菜品编号。
    - 返回值：存在于进行中或待结账订单里则返回 true。

    使用场景：
    管理员删除菜品前调用，防止删除仍在使用中的菜品。
*/
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

