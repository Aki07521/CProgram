#include "restaurant_backend.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

// 后端实现文件。
// 这里故意不包含 Qt 头文件，保证业务层可以被其他前端复用。
namespace Backend {
namespace {
    // 数据目录是进程级配置：Qt 前端启动时设置一次，后续所有读写都走这个目录。
    std::string g_dataDirectory = ".";

    // 把文件名拼成完整数据路径，兼容目录末尾有没有斜杠的情况。
    std::string dataPath(const char* filename) {
        if (g_dataDirectory.empty() || g_dataDirectory == ".") {
            return filename;
        }
        const char last = g_dataDirectory[g_dataDirectory.size() - 1];
        if (last == '/' || last == '\\') {
            return g_dataDirectory + filename;
        }
        return g_dataDirectory + "/" + filename;
    }

    // txt 数据使用竖线分隔字段，例如 dishes.txt 中一行就是“编号|名称|分类|价格|库存|销量”。
    std::vector<std::string> split(const std::string& line, char delimiter) {
        std::vector<std::string> parts;
        std::string part;
        std::stringstream ss(line);
        while (std::getline(ss, part, delimiter)) {
            parts.push_back(part);
        }
        return parts;
    }

    // 生成订单创建/结账时间，统一使用固定格式，便于保存和展示。
    std::string nowText() {
        std::time_t t = std::time(0);
        std::tm* local = std::localtime(&t);
        char buffer[32] = {0};
        if (local != 0) {
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
        }
        return buffer;
    }
}

void RestaurantBackend::setDataDirectory(const std::string& directory) {
    // 空字符串退回当前目录，避免调用方传错路径后拼出非法文件名。
    g_dataDirectory = directory.empty() ? "." : directory;
}

bool RestaurantBackend::load() {
    // 先清空内存状态，避免重复 load() 时把旧数据追加进去。
    users_.clear();
    dishes_.clear();
    tables_.clear();
    orders_.clear();
    nextOrderId_ = 1;

    // 1. 加载用户表。用户是独立数据，不依赖其他表。
    std::ifstream usersIn(dataPath("users.txt").c_str());
    std::string line;
    while (std::getline(usersIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 4) {
            users_.push_back(User{p[0], p[1], p[2], p[3]});
        }
    }

    // 2. 加载菜品表。库存和销量直接作为当前状态恢复。
    std::ifstream dishesIn(dataPath("dishes.txt").c_str());
    while (std::getline(dishesIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 6) {
            Dish dish;
            dish.id = std::atoi(p[0].c_str());
            dish.name = p[1];
            dish.category = p[2];
            dish.price = std::atof(p[3].c_str());
            dish.stock = std::atoi(p[4].c_str());
            dish.sold = std::atoi(p[5].c_str());
            dishes_.push_back(dish);
        }
    }

    // 3. 加载餐桌表。activeOrderId 会在后续操作中用于快速找到当前订单。
    std::ifstream tablesIn(dataPath("tables.txt").c_str());
    while (std::getline(tablesIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 5) {
            Table table;
            table.id = std::atoi(p[0].c_str());
            table.capacity = std::atoi(p[1].c_str());
            table.status = std::atoi(p[2].c_str());
            table.currentAmount = std::atof(p[3].c_str());
            table.activeOrderId = std::atoi(p[4].c_str());
            tables_.push_back(table);
        }
    }

    // 4. 加载订单主表，并根据历史最大订单号推算下一个订单号。
    std::ifstream ordersIn(dataPath("orders.txt").c_str());
    while (std::getline(ordersIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 7) {
            Order order;
            order.orderId = std::atoi(p[0].c_str());
            order.tableId = std::atoi(p[1].c_str());
            order.waiter = p[2];
            order.status = std::atoi(p[3].c_str());
            order.totalAmount = std::atof(p[4].c_str());
            order.createdAt = p[5];
            order.paidAt = p[6];
            orders_.push_back(order);
            if (order.orderId >= nextOrderId_) {
                nextOrderId_ = order.orderId + 1;
            }
        }
    }

    // 5. 加载订单明细。明细依赖订单主表，所以必须在订单主表之后处理。
    std::ifstream itemsIn(dataPath("order_items.txt").c_str());
    while (std::getline(itemsIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 6) {
            Order* order = findOrder(std::atoi(p[0].c_str()));
            if (order != 0) {
                OrderItem item;
                item.orderId = order->orderId;
                item.dishId = std::atoi(p[1].c_str());
                item.dishName = p[2];
                item.quantity = std::atoi(p[3].c_str());
                item.price = std::atof(p[4].c_str());
                item.subtotal = std::atof(p[5].c_str());
                order->items.push_back(item);
            }
        }
    }

    // 首次运行或数据文件缺失时，自动补一份最小可运行数据。
    if (users_.empty() || dishes_.empty() || tables_.empty()) {
        resetDemoData();
        save();
    }
    return true;
}

void RestaurantBackend::save() const {
    // save() 使用“全量覆盖写回”，逻辑简单且能确保 txt 文件与内存状态一致。
    std::ofstream usersOut(dataPath("users.txt").c_str());
    for (size_t i = 0; i < users_.size(); ++i) {
        usersOut << users_[i].username << '|' << users_[i].password << '|'
                 << users_[i].role << '|' << users_[i].name << '\n';
    }

    std::ofstream dishesOut(dataPath("dishes.txt").c_str());
    for (size_t i = 0; i < dishes_.size(); ++i) {
        dishesOut << dishes_[i].id << '|' << dishes_[i].name << '|' << dishes_[i].category
                  << '|' << std::fixed << std::setprecision(2) << dishes_[i].price << '|'
                  << dishes_[i].stock << '|' << dishes_[i].sold << '\n';
    }

    std::ofstream tablesOut(dataPath("tables.txt").c_str());
    for (size_t i = 0; i < tables_.size(); ++i) {
        tablesOut << tables_[i].id << '|' << tables_[i].capacity << '|' << tables_[i].status
                  << '|' << std::fixed << std::setprecision(2) << tables_[i].currentAmount
                  << '|' << tables_[i].activeOrderId << '\n';
    }

    // 订单拆成主表和明细表：主表保存订单状态，明细表保存每道菜的数量和价格快照。
    std::ofstream ordersOut(dataPath("orders.txt").c_str());
    std::ofstream itemsOut(dataPath("order_items.txt").c_str());
    for (size_t i = 0; i < orders_.size(); ++i) {
        const Order& order = orders_[i];
        ordersOut << order.orderId << '|' << order.tableId << '|' << order.waiter << '|'
                  << order.status << '|' << std::fixed << std::setprecision(2) << order.totalAmount
                  << '|' << order.createdAt << '|' << order.paidAt << '\n';
        for (size_t j = 0; j < order.items.size(); ++j) {
            const OrderItem& item = order.items[j];
            itemsOut << order.orderId << '|' << item.dishId << '|' << item.dishName << '|'
                     << item.quantity << '|' << std::fixed << std::setprecision(2)
                     << item.price << '|' << item.subtotal << '\n';
        }
    }
}

void RestaurantBackend::resetDemoData() {
    // 演示数据只负责让系统能立即登录和试用，不承载真实业务记录。
    users_.clear();
    dishes_.clear();
    tables_.clear();
    orders_.clear();
    nextOrderId_ = 1;

    users_.push_back(User{"admin", "123456", "admin", "系统管理员"});
    users_.push_back(User{"waiter", "123456", "waiter", "默认服务员"});

    dishes_.push_back(Dish{1001, "宫保鸡丁", "热菜", 28.0, 50, 0});
    dishes_.push_back(Dish{1002, "鱼香肉丝", "热菜", 26.0, 40, 0});
    dishes_.push_back(Dish{1003, "米饭", "主食", 2.0, 200, 0});
    dishes_.push_back(Dish{1004, "可乐", "饮品", 5.0, 80, 0});
    dishes_.push_back(Dish{1005, "西红柿鸡蛋汤", "汤品", 18.0, 30, 0});

    tables_.push_back(Table{1, 4, TableIdle, 0.0, 0});
    tables_.push_back(Table{2, 6, TableIdle, 0.0, 0});
    tables_.push_back(Table{3, 4, TableIdle, 0.0, 0});
    tables_.push_back(Table{4, 8, TableIdle, 0.0, 0});
    tables_.push_back(Table{5, 2, TableIdle, 0.0, 0});
}

bool RestaurantBackend::login(const std::string& role, const std::string& username,
                              const std::string& password, std::string* displayName) const {
    // 登录同时校验角色，避免服务员账号以管理员身份进入。
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].role == role && users_[i].username == username && users_[i].password == password) {
            if (displayName != 0) {
                *displayName = users_[i].name.empty() ? users_[i].username : users_[i].name;
            }
            return true;
        }
    }
    return false;
}

const std::vector<User>& RestaurantBackend::users() const { return users_; }
const std::vector<Dish>& RestaurantBackend::dishes() const { return dishes_; }
const std::vector<Table>& RestaurantBackend::tables() const { return tables_; }
const std::vector<Order>& RestaurantBackend::orders() const { return orders_; }

// 新增用户只做最小合法性和唯一性检查；角色合法性由前端下拉框保证。
bool RestaurantBackend::addUser(const User& user, std::string& error) {
    if (user.username.empty() || user.password.empty()) {
        error = "账号和密码不能为空。";
        return false;
    }
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].username == user.username) {
            error = "账号已存在。";
            return false;
        }
    }
    users_.push_back(user);
    save();
    return true;
}

bool RestaurantBackend::updateUser(const User& user, std::string& error) {
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].username == user.username) {
            // 防止把最后一个管理员改成服务员，否则后续无法进入管理功能。
            if (users_[i].role == "admin" && user.role != "admin" && countAdmins() <= 1) {
                error = "系统至少需要保留一个管理员。";
                return false;
            }
            users_[i] = user;
            save();
            return true;
        }
    }
    error = "没有找到该用户。";
    return false;
}

bool RestaurantBackend::deleteUser(const std::string& username, std::string& error) {
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].username == username) {
            // 同样不允许删除最后一个管理员账号。
            if (users_[i].role == "admin" && countAdmins() <= 1) {
                error = "系统至少需要保留一个管理员。";
                return false;
            }
            users_.erase(users_.begin() + static_cast<long long>(i));
            save();
            return true;
        }
    }
    error = "没有找到该用户。";
    return false;
}

// 新增菜品时只允许正编号、正价格和非负库存，避免订单计算出现异常。
bool RestaurantBackend::addDish(const Dish& dish, std::string& error) {
    if (dish.id <= 0 || dish.name.empty() || dish.price <= 0.0 || dish.stock < 0) {
        error = "菜品编号、名称、价格和库存不合法。";
        return false;
    }
    if (findDish(dish.id) != 0) {
        error = "菜品编号已存在。";
        return false;
    }
    dishes_.push_back(dish);
    save();
    return true;
}

bool RestaurantBackend::updateDish(const Dish& dish, std::string& error) {
    Dish* old = findDish(dish.id);
    if (old == 0) {
        error = "没有找到该菜品。";
        return false;
    }
    if (dish.name.empty() || dish.price <= 0.0 || dish.stock < 0) {
        error = "菜品名称、价格和库存不合法。";
        return false;
    }
    // 更新基础资料不回写历史订单明细，因为订单明细保存的是下单时快照。
    *old = dish;
    save();
    return true;
}

bool RestaurantBackend::deleteDish(int id, std::string& error) {
    // 未结账订单还引用该菜品时禁止删除，避免订单里出现“找不到菜品”的情况。
    if (dishInUnpaidOrder(id)) {
        error = "该菜品仍存在于未结账订单中，不能删除。";
        return false;
    }
    for (size_t i = 0; i < dishes_.size(); ++i) {
        if (dishes_[i].id == id) {
            dishes_.erase(dishes_.begin() + static_cast<long long>(i));
            save();
            return true;
        }
    }
    error = "没有找到该菜品。";
    return false;
}

bool RestaurantBackend::addTable(const Table& table, std::string& error) {
    if (table.id <= 0 || table.capacity <= 0) {
        error = "餐桌编号和容量不合法。";
        return false;
    }
    if (findTable(table.id) != 0) {
        error = "餐桌编号已存在。";
        return false;
    }
    Table created = table;
    // 新增餐桌必须从空闲状态开始，不能继承前端传入的占用状态。
    created.status = TableIdle;
    created.currentAmount = 0.0;
    created.activeOrderId = 0;
    tables_.push_back(created);
    save();
    return true;
}

bool RestaurantBackend::updateTable(const Table& table, std::string& error) {
    Table* old = findTable(table.id);
    if (old == 0) {
        error = "没有找到该餐桌。";
        return false;
    }
    if (table.capacity <= 0) {
        error = "餐桌容量不合法。";
        return false;
    }
    // 目前只允许修改容量，不允许通过资料维护绕过开台/结账状态机。
    old->capacity = table.capacity;
    save();
    return true;
}

bool RestaurantBackend::deleteTable(int id, std::string& error) {
    Table* table = findTable(id);
    // 正在使用或待结账的餐桌有业务状态，必须先结账/取消后才能删除。
    if (table != 0 && table->status != TableIdle) {
        error = "餐桌正在使用，不能删除。";
        return false;
    }
    for (size_t i = 0; i < tables_.size(); ++i) {
        if (tables_[i].id == id) {
            tables_.erase(tables_.begin() + static_cast<long long>(i));
            save();
            return true;
        }
    }
    error = "没有找到该餐桌。";
    return false;
}

bool RestaurantBackend::openTable(int tableId, const std::string& waiter, int& orderId, std::string& error) {
    Table* table = findTable(tableId);
    if (table == 0) {
        error = "没有找到该餐桌。";
        return false;
    }
    if (table->status != TableIdle) {
        error = "该餐桌不是空闲状态。";
        return false;
    }

    // 开台本质上是“创建一张进行中的订单”，并把餐桌绑定到这张订单。
    Order order;
    order.orderId = nextOrderId_++;
    order.tableId = tableId;
    order.waiter = waiter;
    order.status = OrderActive;
    order.createdAt = nowText();
    orders_.push_back(order);

    // 餐桌状态和订单状态必须同时变化，否则界面上会出现桌台与订单不同步。
    table->status = TableUsing;
    table->activeOrderId = order.orderId;
    table->currentAmount = 0.0;
    orderId = order.orderId;
    save();
    return true;
}

bool RestaurantBackend::addDishToOrder(int orderId, int dishId, int quantity, std::string& error) {
    Order* order = findOrder(orderId);
    Dish* dish = findDish(dishId);
    if (order == 0 || dish == 0) {
        error = "订单或菜品不存在。";
        return false;
    }
    if (order->status != OrderActive) {
        error = "只有进行中的订单可以加菜。";
        return false;
    }
    if (quantity <= 0 || quantity > dish->stock) {
        error = "数量不合法或库存不足。";
        return false;
    }

    // 如果订单里已经有这道菜，只累加数量，不重复新增明细行。
    for (size_t i = 0; i < order->items.size(); ++i) {
        if (order->items[i].dishId == dishId) {
            order->items[i].quantity += quantity;
            order->items[i].subtotal = order->items[i].quantity * order->items[i].price;
            // 点菜成功后立即扣库存、加销量，保证库存展示和订单操作实时同步。
            dish->stock -= quantity;
            dish->sold += quantity;
            recalcOrder(*order);
            save();
            return true;
        }
    }

    // 第一次点这道菜时，记录菜名和单价快照，后续菜品改名/调价不影响本订单。
    OrderItem item;
    item.orderId = order->orderId;
    item.dishId = dish->id;
    item.dishName = dish->name;
    item.quantity = quantity;
    item.price = dish->price;
    item.subtotal = dish->price * quantity;
    order->items.push_back(item);
    // 明细加入后同样要更新库存和订单金额。
    dish->stock -= quantity;
    dish->sold += quantity;
    recalcOrder(*order);
    save();
    return true;
}

bool RestaurantBackend::returnDish(int orderId, int dishId, int quantity, std::string& error) {
    Order* order = findOrder(orderId);
    if (order == 0 || order->status == OrderPaid || order->status == OrderCancelled) {
        error = "订单不存在或不可退菜。";
        return false;
    }
    for (size_t i = 0; i < order->items.size(); ++i) {
        if (order->items[i].dishId == dishId) {
            if (quantity <= 0 || quantity > order->items[i].quantity) {
                error = "退菜数量不合法。";
                return false;
            }
            Dish* dish = findDish(dishId);
            if (dish != 0) {
                // 退菜要反向恢复库存和销量，且销量不能低于 0。
                dish->stock += quantity;
                dish->sold = std::max(0, dish->sold - quantity);
            }
            order->items[i].quantity -= quantity;
            // 数量退到 0 时删除明细行，否则重新计算这行小计。
            if (order->items[i].quantity == 0) {
                order->items.erase(order->items.begin() + static_cast<long long>(i));
            } else {
                order->items[i].subtotal = order->items[i].quantity * order->items[i].price;
            }
            recalcOrder(*order);
            if (order->items.empty()) {
                // 没有任何菜品的订单没有结账价值，直接取消并释放餐桌。
                cancelOrder(order->orderId, error);
            } else {
                save();
            }
            return true;
        }
    }
    error = "订单中没有该菜品。";
    return false;
}

bool RestaurantBackend::markWaitPay(int tableId, std::string& error) {
    Order* order = findActiveOrderByTable(tableId);
    Table* table = findTable(tableId);
    if (order == 0 || table == 0) {
        error = "该餐桌没有未结账订单。";
        return false;
    }
    if (order->items.empty()) {
        error = "订单为空，不能设置为待结账。";
        return false;
    }
    // 待结账表示不再继续点菜，餐桌仍被占用但状态切换到收银流程。
    order->status = OrderWaitPay;
    table->status = TableWaitPay;
    table->currentAmount = order->totalAmount;
    save();
    return true;
}

bool RestaurantBackend::checkout(int tableId, std::string& error) {
    Order* order = findActiveOrderByTable(tableId);
    Table* table = findTable(tableId);
    if (order == 0 || table == 0) {
        error = "该餐桌没有未结账订单。";
        return false;
    }
    if (order->items.empty()) {
        error = "订单为空，不能结账。";
        return false;
    }
    // 结账完成后订单成为历史记录，餐桌恢复空闲并解除 activeOrderId 绑定。
    order->status = OrderPaid;
    order->paidAt = nowText();
    table->status = TableIdle;
    table->currentAmount = 0.0;
    table->activeOrderId = 0;
    save();
    return true;
}

bool RestaurantBackend::cancelOrder(int orderId, std::string& error) {
    Order* order = findOrder(orderId);
    if (order == 0 || order->status == OrderPaid) {
        error = "订单不存在或已结账，不能取消。";
        return false;
    }
    // 取消未结账订单时，要把所有已点菜品退回库存。
    for (size_t i = 0; i < order->items.size(); ++i) {
        Dish* dish = findDish(order->items[i].dishId);
        if (dish != 0) {
            dish->stock += order->items[i].quantity;
            dish->sold = std::max(0, dish->sold - order->items[i].quantity);
        }
    }
    order->items.clear();
    order->totalAmount = 0.0;
    order->status = OrderCancelled;

    Table* table = findTable(order->tableId);
    if (table != 0 && table->activeOrderId == order->orderId) {
        // 如果餐桌当前仍绑定这张订单，取消后同步释放餐桌。
        table->status = TableIdle;
        table->currentAmount = 0.0;
        table->activeOrderId = 0;
    }
    save();
    return true;
}

std::string RestaurantBackend::money(double value) {
    // 金额统一保留两位小数，避免不同界面显示不一致。
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

std::string RestaurantBackend::tableStatusName(int status) {
    // 状态值转中文展示文本；前端表格直接使用这些字符串。
    if (status == TableIdle) return "空闲";
    if (status == TableUsing) return "使用中";
    return "待结账";
}

std::string RestaurantBackend::orderStatusName(int status) {
    // 订单状态展示文本集中在后端，避免前端重复写 switch/if。
    if (status == OrderActive) return "进行中";
    if (status == OrderWaitPay) return "待结账";
    if (status == OrderPaid) return "已结账";
    return "已取消";
}

std::string RestaurantBackend::roleName(const std::string& role) {
    // role 的内部值保持英文，展示时再转换成中文。
    return role == "admin" ? "管理员" : "服务员";
}

Dish* RestaurantBackend::findDish(int id) {
    // 目前数据量很小，线性查找足够简单可靠；后续数据量大时可改为 map 索引。
    for (size_t i = 0; i < dishes_.size(); ++i) {
        if (dishes_[i].id == id) return &dishes_[i];
    }
    return 0;
}

const Dish* RestaurantBackend::findDish(int id) const {
    // const 版本用于只读函数，防止无意修改后端状态。
    for (size_t i = 0; i < dishes_.size(); ++i) {
        if (dishes_[i].id == id) return &dishes_[i];
    }
    return 0;
}

Table* RestaurantBackend::findTable(int id) {
    // 返回指针方便调用方直接修改餐桌状态、金额和当前订单号。
    for (size_t i = 0; i < tables_.size(); ++i) {
        if (tables_[i].id == id) return &tables_[i];
    }
    return 0;
}

const Table* RestaurantBackend::findTable(int id) const {
    // 只读查找餐桌，供状态判断类函数使用。
    for (size_t i = 0; i < tables_.size(); ++i) {
        if (tables_[i].id == id) return &tables_[i];
    }
    return 0;
}

Order* RestaurantBackend::findOrder(int id) {
    // 按订单号查找订单主表，订单明细挂在 Order.items 内。
    for (size_t i = 0; i < orders_.size(); ++i) {
        if (orders_[i].orderId == id) return &orders_[i];
    }
    return 0;
}

const Order* RestaurantBackend::findOrder(int id) const {
    // 只读查找订单，避免展示逻辑改动订单内容。
    for (size_t i = 0; i < orders_.size(); ++i) {
        if (orders_[i].orderId == id) return &orders_[i];
    }
    return 0;
}

Order* RestaurantBackend::findActiveOrderByTable(int tableId) {
    // 优先使用餐桌上的 activeOrderId，这是开台时建立的直接关联。
    const Table* table = findTable(tableId);
    if (table != 0 && table->activeOrderId > 0) {
        return findOrder(table->activeOrderId);
    }
    // 兼容旧数据或异常数据：如果 activeOrderId 丢失，就按桌号扫描未完成订单。
    for (size_t i = 0; i < orders_.size(); ++i) {
        if (orders_[i].tableId == tableId &&
            (orders_[i].status == OrderActive || orders_[i].status == OrderWaitPay)) {
            return &orders_[i];
        }
    }
    return 0;
}

const Order* RestaurantBackend::findActiveOrderByTable(int tableId) const {
    // const 版本逻辑与可写版本保持一致。
    const Table* table = findTable(tableId);
    if (table != 0 && table->activeOrderId > 0) {
        return findOrder(table->activeOrderId);
    }
    for (size_t i = 0; i < orders_.size(); ++i) {
        if (orders_[i].tableId == tableId &&
            (orders_[i].status == OrderActive || orders_[i].status == OrderWaitPay)) {
            return &orders_[i];
        }
    }
    return 0;
}

void RestaurantBackend::recalcOrder(Order& order) {
    // 订单总金额永远从明细重新汇总，避免多次加菜/退菜后累计误差。
    double total = 0.0;
    for (size_t i = 0; i < order.items.size(); ++i) {
        order.items[i].subtotal = order.items[i].price * order.items[i].quantity;
        total += order.items[i].subtotal;
    }
    order.totalAmount = total;

    Table* table = findTable(order.tableId);
    if (table != 0 && table->activeOrderId == order.orderId) {
        // 餐桌当前金额是订单金额的镜像，前端餐桌表依赖这个字段展示。
        table->currentAmount = total;
    }
}

bool RestaurantBackend::dishInUnpaidOrder(int dishId) const {
    // 只检查未完成订单；已结账/已取消订单保留的是历史快照，不阻止菜品删除。
    for (size_t i = 0; i < orders_.size(); ++i) {
        if (orders_[i].status == OrderActive || orders_[i].status == OrderWaitPay) {
            for (size_t j = 0; j < orders_[i].items.size(); ++j) {
                if (orders_[i].items[j].dishId == dishId) {
                    return true;
                }
            }
        }
    }
    return false;
}

int RestaurantBackend::countAdmins() const {
    // 管理员数量用于保护账号体系，避免所有管理员被降级或删除。
    int count = 0;
    for (size_t i = 0; i < users_.size(); ++i) {
        if (users_[i].role == "admin") {
            ++count;
        }
    }
    return count;
}

}
