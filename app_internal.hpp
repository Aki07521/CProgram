#ifndef APP_INTERNAL_HPP
#define APP_INTERNAL_HPP

#include <string>
#include <vector>

/*
    app_internal.hpp 是 app 模块的“内部说明书”。

    为什么还需要这个头文件？
    - app.hpp 只暴露给 main.cpp 使用，里面放系统入口和各业务页面函数。
    - app_internal.hpp 只给 app_*.cpp 之间互相调用，里面放链表节点、全局链表和工具函数声明。

    学习阅读建议：
    1. 先看下面四类链表节点，理解系统保存了哪些数据。
    2. 再看 app_core.cpp，理解节点如何创建、追加、查找、删除和同步订单金额。
    3. 然后看 app_persistence.cpp，理解链表如何保存到 txt 文件。
    4. 最后看 app_admin.cpp / app_waiter.cpp，理解页面如何调用底层业务函数。
*/

// 页面统一使用 80 列宽度，与 ui.cpp 中的布局常量保持一致。
const int LEFT = 10;
const int WIDTH = 80;

/*
    餐桌状态枚举：
    enum 可以给一组整数状态起有意义的名字。

    状态流转：
    TABLE_IDLE -> TABLE_USING -> TABLE_WAIT_PAY -> TABLE_IDLE
*/
enum TableStatus {
    TABLE_IDLE = 0,
    TABLE_USING = 1,
    TABLE_WAIT_PAY = 2
};

/*
    订单状态枚举：
    已结账订单计入营收；已取消订单保留记录但不计入营收。
*/
enum OrderStatus {
    ORDER_ACTIVE = 0,
    ORDER_WAIT_PAY = 1,
    ORDER_PAID = 2,
    ORDER_CANCELLED = 3
};

/*
    用户链表节点：
    每个 UserNode 表示一个系统账号。

    链表结构说明：
    next 指向下一个用户节点；如果 next 为 NULL，表示这是链表最后一个节点。
*/
struct UserNode {
    std::string username;
    std::string password;
    std::string role;
    std::string name;
    UserNode* next;
};

/*
    菜品链表节点：
    保存菜单基础资料、库存和累计销量。

    注意：
    stock 会随着点菜、退菜变化；sold 用于销售统计参考。
*/
struct DishNode {
    int id;
    std::string name;
    std::string category;
    double price;
    int stock;
    int sold;
    DishNode* next;
};

/*
    餐桌链表节点：
    记录餐桌容量、状态、当前消费金额，以及当前关联订单号。

    关键字段：
    activeOrderId 把餐桌和订单连接起来。
    0 表示该餐桌当前没有关联订单。
*/
struct TableNode {
    int id;
    int capacity;
    int status;
    double currentAmount;
    int activeOrderId;
    TableNode* next;
};

/*
    订单明细链表节点：
    一个订单下面可以挂多条菜品明细，每条明细表示一道菜。

    为什么保存 dishName 和 price？
    因为菜品后续可能改名或改价，历史小票仍要保持下单时的信息。
*/
struct OrderItemNode {
    int dishId;
    std::string dishName;
    int quantity;
    double price;
    double subtotal;
    OrderItemNode* next;
};

/*
    订单链表节点：
    订单本身是一条主记录，items 指向它的明细子链表。

    主从关系：
    OrderNode 是“订单主表”，OrderItemNode 是“订单明细表”。
*/
struct OrderNode {
    int orderId;
    int tableId;
    std::string waiter;
    int status;
    double totalAmount;
    std::string createdAt;
    std::string paidAt;
    OrderItemNode* items;
    OrderNode* next;
};

/*
    四个核心链表头指针：
    使用 extern 声明，真正的变量定义在 app_core.cpp 中。

    语法说明：
    extern 表示“这个变量在别的 .cpp 文件里已经定义了，这里只是声明给大家使用”。
*/
extern UserNode* g_users;
extern DishNode* g_dishes;
extern TableNode* g_tables;
extern OrderNode* g_orders;

extern bool g_loaded;
extern int g_nextOrderId;
extern std::string g_currentUsername;
extern std::string g_currentRole;

/*
    下面是 app 模块内部函数声明。

    为什么头文件只写声明？
    - 声明告诉编译器函数名字、参数和返回值。
    - 真正的函数实现放在对应 .cpp 文件里。
    - 多个 .cpp 只要包含这个头文件，就能互相调用这些函数。
*/

// 通用界面和格式化工具，主要实现在 app_core.cpp。
void printAt(int x, int y, const std::string& text);
void drawWorkPage(const std::string& title);
void printLine(int y, const std::string& text);
std::vector<std::string> split(const std::string& line, char delimiter);
std::string money(double value);
std::string nowText();
std::string roleName(const std::string& role);
std::string tableStatusName(int status);
std::string orderStatusName(int status);

// 链表节点创建、追加、释放和查找。
UserNode* makeUser(const std::string& username, const std::string& password,
                   const std::string& role, const std::string& name);
DishNode* makeDish(int id, const std::string& name, const std::string& category,
                   double price, int stock, int sold);
TableNode* makeTable(int id, int capacity, int status, double amount, int orderId);
OrderNode* makeOrder(int orderId, int tableId, const std::string& waiter, int status,
                     double total, const std::string& createdAt, const std::string& paidAt);
OrderItemNode* makeOrderItem(int dishId, const std::string& dishName, int quantity,
                             double price, double subtotal);

void appendUser(UserNode* node);
void appendDish(DishNode* node);
void appendTable(TableNode* node);
void appendOrder(OrderNode* node);
void appendOrderItem(OrderNode* order, OrderItemNode* item);

void clearUsers();
void clearDishes();
void clearTables();
void clearOrderItems(OrderItemNode*& head);
void clearOrders();
void clearAllData();

UserNode* findUser(const std::string& username);
DishNode* findDish(int id);
TableNode* findTable(int id);
OrderNode* findOrder(int id);
OrderNode* findOrderByTable(int tableId);
OrderItemNode* findOrderItem(OrderNode* order, int dishId);
int countAdmins();

// 默认演示数据和文件持久化。
void seedUsers();
void seedDishes();
void seedTables();
bool loadUsers();
bool loadDishes();
bool loadTables();
bool loadOrders();
bool loadOrderItems();
void saveUsers();
void saveDishes();
void saveTables();
void saveOrders();
void saveOrderItems();
void saveAll();
void ensureDataLoaded();

// 登录、输入校验和订单业务规则。
bool checkAccount(const std::string& role, const std::string& username, const std::string& password);
std::string currentDisplayName();
std::string readRequiredString(const std::string& label);
double inputDouble(const std::string& label, double minValue, double maxValue);
int inputExistingDishId(const std::string& label, bool allowZero);
int inputExistingTableId(const std::string& label, bool allowZero);

void recalcOrderTotal(OrderNode* order);
bool orderHasItems(OrderNode* order);
void addDishToOrder(OrderNode* order, DishNode* dish, int quantity);
bool removeOrReduceItem(OrderNode* order, int dishId, int quantity);
void restoreOrderStock(OrderNode* order);
void cancelOrder(OrderNode* order);
void updateEmptyOrder(OrderNode* order);
bool dishInUnpaidOrder(int dishId);

// 展示型页面和查询辅助函数。
void showReceiptForOrder(OrderNode* order, bool waitKey);
void showDishListPage(const std::string& title);
void showTableListPage(const std::string& title);
void showUserListPage();
void showOrderListPage();
OrderNode* latestOrder();

#endif
