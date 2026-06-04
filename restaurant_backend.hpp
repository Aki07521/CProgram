#ifndef RESTAURANT_BACKEND_HPP
#define RESTAURANT_BACKEND_HPP

#include <string>
#include <vector>

namespace Backend {

// 餐桌的运行状态。
// 约定：状态值会直接保存到 tables.txt，所以不要随意调整已有枚举值。
enum TableStatus {
    TableIdle = 0,
    TableUsing = 1,
    TableWaitPay = 2
};

// 订单的生命周期状态。
// Active/WaitPay 都属于“未完成订单”，会影响餐桌占用和菜品删除判断。
enum OrderStatus {
    OrderActive = 0,
    OrderWaitPay = 1,
    OrderPaid = 2,
    OrderCancelled = 3
};

// 系统用户。role 使用 "admin" 或 "waiter"，由前端权限控制决定能看到哪些操作。
struct User {
    std::string username;
    std::string password;
    std::string role;
    std::string name;
};

// 菜品基础资料。stock 是当前库存，sold 是已经售出的数量。
struct Dish {
    int id = 0;
    std::string name;
    std::string category;
    double price = 0.0;
    int stock = 0;
    int sold = 0;
};

// 餐桌资料和当前占用信息。
// activeOrderId 把餐桌和“当前未完成订单”连接起来，是开台、结账、取消订单的关键字段。
struct Table {
    int id = 0;
    int capacity = 0;
    int status = TableIdle;
    double currentAmount = 0.0;
    int activeOrderId = 0;
};

// 订单明细快照。
// dishName 和 price 会保存下单时的菜名、单价，后续修改菜品资料不会影响历史订单。
struct OrderItem {
    int orderId = 0;
    int dishId = 0;
    std::string dishName;
    int quantity = 0;
    double price = 0.0;
    double subtotal = 0.0;
};

// 一张订单主表。items 是订单明细列表，totalAmount 由明细小计汇总得到。
struct Order {
    int orderId = 0;
    int tableId = 0;
    std::string waiter;
    int status = OrderActive;
    double totalAmount = 0.0;
    std::string createdAt;
    std::string paidAt;
    std::vector<OrderItem> items;
};

// 业务后端：只处理数据和业务规则，不依赖 Qt 或 EasyX。
// 前端通过这个类完成登录、资料维护、点餐、退菜、结账等操作。
class RestaurantBackend {
public:
    // 设置数据文件所在目录。Qt 版本把 exe 放在 dist/，所以启动时会把目录指回项目根目录。
    static void setDataDirectory(const std::string& directory);

    // 从 txt 文件加载全部数据；如果关键数据为空，会自动生成一份演示初始数据。
    bool load();

    // 把当前内存数据完整写回 txt 文件。每个会改变数据的业务操作成功后都会调用它。
    void save() const;

    // 重置内存中的演示数据，不直接保存；调用方需要再调用 save()。
    void resetDemoData();

    // 登录校验。displayName 可选，用来把真实姓名返回给前端显示。
    bool login(const std::string& role, const std::string& username,
               const std::string& password, std::string* displayName = 0) const;

    // 只读访问器：前端刷新表格时读取这些列表，但不直接修改内部数据。
    const std::vector<User>& users() const;
    const std::vector<Dish>& dishes() const;
    const std::vector<Table>& tables() const;
    const std::vector<Order>& orders() const;

    // 用户维护。失败时通过 error 返回适合直接展示给用户的错误提示。
    bool addUser(const User& user, std::string& error);
    bool updateUser(const User& user, std::string& error);
    bool deleteUser(const std::string& username, std::string& error);

    // 菜品维护。删除菜品前会检查它是否仍出现在未结账订单中。
    bool addDish(const Dish& dish, std::string& error);
    bool updateDish(const Dish& dish, std::string& error);
    bool deleteDish(int id, std::string& error);

    // 餐桌维护。新增餐桌强制从空闲状态开始；正在使用的餐桌不允许删除。
    bool addTable(const Table& table, std::string& error);
    bool updateTable(const Table& table, std::string& error);
    bool deleteTable(int id, std::string& error);

    // 点餐主流程：开台 -> 加菜/退菜 -> 待结账 -> 结账，也支持取消未结账订单。
    bool openTable(int tableId, const std::string& waiter, int& orderId, std::string& error);
    bool addDishToOrder(int orderId, int dishId, int quantity, std::string& error);
    bool returnDish(int orderId, int dishId, int quantity, std::string& error);
    bool markWaitPay(int tableId, std::string& error);
    bool checkout(int tableId, std::string& error);
    bool cancelOrder(int orderId, std::string& error);

    // 展示用格式化函数：把后端内部值转换成界面上更友好的文本。
    static std::string money(double value);
    static std::string tableStatusName(int status);
    static std::string orderStatusName(int status);
    static std::string roleName(const std::string& role);

private:
    std::vector<User> users_;
    std::vector<Dish> dishes_;
    std::vector<Table> tables_;
    std::vector<Order> orders_;

    // 下一个订单号。加载历史订单时会自动调整为“最大订单号 + 1”。
    int nextOrderId_ = 1;

    // 内部查找函数返回指针，便于业务操作直接修改目标对象。
    Dish* findDish(int id);
    const Dish* findDish(int id) const;
    Table* findTable(int id);
    const Table* findTable(int id) const;
    Order* findOrder(int id);
    const Order* findOrder(int id) const;
    Order* findActiveOrderByTable(int tableId);
    const Order* findActiveOrderByTable(int tableId) const;

    // 重新计算订单金额，并同步更新关联餐桌的 currentAmount。
    void recalcOrder(Order& order);

    // 判断菜品是否仍在未完成订单中，避免删除后订单明细失去引用。
    bool dishInUnpaidOrder(int dishId) const;

    // 统计管理员数量，确保系统里至少保留一个管理员账号。
    int countAdmins() const;
};

}

#endif
