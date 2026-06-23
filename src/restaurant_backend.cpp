#include "restaurant_backend.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

// 后端数据读写和业务规则
namespace Backend {
namespace {
    std::string g_dataDirectory = ".";

    // 拼接数据文件路径
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

    // 按分隔符拆分一行文本
    std::vector<std::string> split(const std::string& line, char delimiter) {
        std::vector<std::string> parts;
        std::string part;
        std::stringstream ss(line);
        while (std::getline(ss, part, delimiter)) {
            parts.push_back(part);
        }
        if (!line.empty() && line[line.size() - 1] == delimiter) {
            parts.push_back("");
        }
        return parts;
    }

    std::string nowText() {
        std::time_t t = std::time(0);
        std::tm* local = std::localtime(&t);
        char buffer[32] = {0};
        if (local != 0) {
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
        }
        return buffer;
    }

    bool unfinished(int status) {
        return status == OrderActive || status == OrderWaitPay;
    }

    // 在链表中查找符合条件的节点数据
    template <typename Container, typename Predicate>
    typename Container::ValueType* findPtr(Container& items, Predicate predicate) {
        for (typename Container::ValueType& item : items) {
            if (predicate(item)) return &item;
        }
        return 0;
    }

    template <typename Container, typename Predicate>
    const typename Container::ValueType* findPtr(const Container& items, Predicate predicate) {
        for (const typename Container::ValueType& item : items) {
            if (predicate(item)) return &item;
        }
        return 0;
    }
}

// 接口实现 设置数据文件目录
void RestaurantBackend::setDataDirectory(const std::string& directory) {
    g_dataDirectory = directory.empty() ? "." : directory;
}

// 接口实现 加载所有数据文件到链表
bool RestaurantBackend::load() {
    // 每次加载前先清空旧链表
    users_.clear();
    dishes_.clear();
    tables_.clear();
    orders_.clear();
    nextOrderId_ = 1;

    std::ifstream usersIn(dataPath("users.txt").c_str());
    std::string line;
    while (std::getline(usersIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 4) {
            users_.push_back(User{p[0], p[1], p[2], p[3]});
        }
    }

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

    std::ifstream ordersIn(dataPath("orders.txt").c_str());
    while (std::getline(ordersIn, line)) {
        std::vector<std::string> p = split(line, '|');
        if (p.size() >= 6) {
            Order order;
            order.orderId = std::atoi(p[0].c_str());
            order.tableId = std::atoi(p[1].c_str());
            order.waiter = p[2];
            order.status = std::atoi(p[3].c_str());
            order.totalAmount = std::atof(p[4].c_str());
            order.createdAt = p[5];
            order.paidAt = p.size() >= 7 ? p[6] : "";
            orders_.push_back(order);
            if (order.orderId >= nextOrderId_) {
                nextOrderId_ = order.orderId + 1;
            }
        }
    }

    // 读取订单明细文件并挂到对应订单链表
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

    // 加载后修正餐桌和订单的关联状态
    if (reconcileTableOrders()) {
        save();
    }
    return true;
}

// 接口实现 将链表数据保存回文本文件
void RestaurantBackend::save() const {
    std::ofstream usersOut(dataPath("users.txt").c_str());
    for (const User& user : users_) {
        usersOut << user.username << '|' << user.password << '|'
                 << user.role << '|' << user.name << '\n';
    }

    std::ofstream dishesOut(dataPath("dishes.txt").c_str());
    for (const Dish& dish : dishes_) {
        dishesOut << dish.id << '|' << dish.name << '|' << dish.category
                  << '|' << std::fixed << std::setprecision(2) << dish.price << '|'
                  << dish.stock << '|' << dish.sold << '\n';
    }

    std::ofstream tablesOut(dataPath("tables.txt").c_str());
    for (const Table& table : tables_) {
        tablesOut << table.id << '|' << table.capacity << '|' << table.status
                  << '|' << std::fixed << std::setprecision(2) << table.currentAmount
                  << '|' << table.activeOrderId << '\n';
    }

    std::ofstream ordersOut(dataPath("orders.txt").c_str());
    std::ofstream itemsOut(dataPath("order_items.txt").c_str());
    for (const Order& order : orders_) {
        ordersOut << order.orderId << '|' << order.tableId << '|' << order.waiter << '|'
                  << order.status << '|' << std::fixed << std::setprecision(2) << order.totalAmount
                  << '|' << order.createdAt << '|' << order.paidAt << '\n';
        for (const OrderItem& item : order.items) {
            itemsOut << order.orderId << '|' << item.dishId << '|' << item.dishName << '|'
                     << item.quantity << '|' << std::fixed << std::setprecision(2)
                     << item.price << '|' << item.subtotal << '\n';
        }
    }
}

// 接口实现 清除历史订单和销售统计
bool RestaurantBackend::clearHistoricalData(std::string& error) {
    error.clear();

    // 有未完成订单时不允许清理
    for (const Order& order : orders_) {
        if (unfinished(order.status)) {
            error = "仍有进行中或待结账订单，请先结账或取消后再清除历史数据。";
            return false;
        }
    }

    orders_.clear();
    nextOrderId_ = 1;

    for (Dish& dish : dishes_) {
        dish.sold = 0;
    }

    for (Table& table : tables_) {
        table.status = TableIdle;
        table.currentAmount = 0.0;
        table.activeOrderId = 0;
    }

    save();
    return true;
}

// 接口实现 登录校验
bool RestaurantBackend::login(const std::string& role, const std::string& username,
                              const std::string& password, std::string* displayName) const {
    for (const User& user : users_) {
        if (user.role == role && user.username == username && user.password == password) {
            if (displayName != 0) {
                *displayName = user.name.empty() ? user.username : user.name;
            }
            return true;
        }
    }
    return false;
}

const LinkedList<User>& RestaurantBackend::users() const { return users_; }
const LinkedList<Dish>& RestaurantBackend::dishes() const { return dishes_; }
const LinkedList<Table>& RestaurantBackend::tables() const { return tables_; }
const LinkedList<Order>& RestaurantBackend::orders() const { return orders_; }

// 接口实现 新增用户
bool RestaurantBackend::addUser(const User& user, std::string& error) {
    if (user.username.empty() || user.password.empty()) {
        error = "账号和密码不能为空。";
        return false;
    }
    for (const User& current : users_) {
        if (current.username == user.username) {
            error = "账号已存在。";
            return false;
        }
    }
    users_.push_back(user);
    save();
    return true;
}

// 接口实现 修改用户
bool RestaurantBackend::updateUser(const User& user, std::string& error) {
    for (User& current : users_) {
        if (current.username == user.username) {
            if (current.role == "admin" && user.role != "admin" && countAdmins() <= 1) {
                error = "系统至少需要保留一个管理员。";
                return false;
            }
            current = user;
            save();
            return true;
        }
    }
    error = "没有找到该用户。";
    return false;
}

// 接口实现 删除用户
bool RestaurantBackend::deleteUser(const std::string& username, std::string& error) {
    // 先检查是否会删除最后一个管理员
    for (const User& user : users_) {
        if (user.username == username) {
            if (user.role == "admin" && countAdmins() <= 1) {
                error = "系统至少需要保留一个管理员。";
                return false;
            }
            users_.removeFirst([&username](const User& item) { return item.username == username; });
            save();
            return true;
        }
    }
    error = "没有找到该用户。";
    return false;
}

// 接口实现 新增菜品
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

// 接口实现 修改菜品
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
    *old = dish;
    save();
    return true;
}

// 接口实现 删除菜品
bool RestaurantBackend::deleteDish(int id, std::string& error) {
    if (dishInUnpaidOrder(id)) {
        error = "该菜品仍存在于未结账订单中，不能删除。";
        return false;
    }
    if (dishes_.removeFirst([id](const Dish& dish) { return dish.id == id; })) {
        save();
        return true;
    }
    error = "没有找到该菜品。";
    return false;
}

// 接口实现 新增餐桌
bool RestaurantBackend::addTable(const Table& table, std::string& error) {
    if (table.id <= 0 || table.capacity <= 0) {
        error = "餐桌编号和容量不合法。";
        return false;
    }
    if (findTable(table.id) != 0) {
        error = "餐桌编号已存在。";
        return false;
    }
    // 新餐桌从空闲状态开始
    Table created = table;
    created.status = TableIdle;
    created.currentAmount = 0.0;
    created.activeOrderId = 0;
    tables_.push_back(created);
    save();
    return true;
}

// 接口实现 修改餐桌
bool RestaurantBackend::updateTable(const Table& table, std::string& error) {
    // 只允许修改容量
    Table* old = findTable(table.id);
    if (old == 0) {
        error = "没有找到该餐桌。";
        return false;
    }
    if (table.capacity <= 0) {
        error = "餐桌容量不合法。";
        return false;
    }
    old->capacity = table.capacity;
    save();
    return true;
}

// 接口实现 删除餐桌
bool RestaurantBackend::deleteTable(int id, std::string& error) {
    Table* table = findTable(id);
    if (table != 0 && table->status != TableIdle) {
        error = "餐桌正在使用，不能删除。";
        return false;
    }
    if (tables_.removeFirst([id](const Table& item) { return item.id == id; })) {
        save();
        return true;
    }
    error = "没有找到该餐桌。";
    return false;
}

// 接口实现 开台并创建订单
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

    Order order;
    order.orderId = nextOrderId_++;
    order.tableId = tableId;
    order.waiter = waiter;
    order.status = OrderActive;
    order.createdAt = nowText();
    orders_.push_back(order);

    table->status = TableUsing;
    table->activeOrderId = order.orderId;
    table->currentAmount = 0.0;
    orderId = order.orderId;
    save();
    return true;
}

// 接口实现 向订单添加菜品
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

    // 如果订单里已有该菜品则直接累加数量
    for (OrderItem& item : order->items) {
        if (item.dishId == dishId) {
            item.quantity += quantity;
            item.subtotal = item.quantity * item.price;
            dish->stock -= quantity;
            dish->sold += quantity;
            recalcOrder(*order);
            save();
            return true;
        }
    }

    // 如果订单里没有该菜品则新增明细节点
    OrderItem item;
    item.orderId = order->orderId;
    item.dishId = dish->id;
    item.dishName = dish->name;
    item.quantity = quantity;
    item.price = dish->price;
    item.subtotal = dish->price * quantity;
    order->items.push_back(item);
    dish->stock -= quantity;
    dish->sold += quantity;
    recalcOrder(*order);
    save();
    return true;
}

// 接口实现 从订单退菜
bool RestaurantBackend::returnDish(int orderId, int dishId, int quantity, std::string& error) {
    Order* order = findOrder(orderId);
    if (order == 0 || order->status == OrderPaid || order->status == OrderCancelled) {
        error = "订单不存在或不可退菜。";
        return false;
    }
    // 遍历订单明细链表寻找要退的菜品
    for (OrderItem& item : order->items) {
        if (item.dishId == dishId) {
            if (quantity <= 0 || quantity > item.quantity) {
                error = "退菜数量不合法。";
                return false;
            }
            Dish* dish = findDish(dishId);
            if (dish != 0) {
                dish->stock += quantity;
                dish->sold = std::max(0, dish->sold - quantity);
            }
            // 退完后数量为零则删除明细节点
            item.quantity -= quantity;
            if (item.quantity == 0) {
                order->items.removeFirst([dishId](const OrderItem& value) { return value.dishId == dishId; });
            } else {
                item.subtotal = item.quantity * item.price;
            }
            recalcOrder(*order);
            if (order->items.empty()) {
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

// 接口实现 设置待结账状态
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
    order->status = OrderWaitPay;
    table->status = TableWaitPay;
    table->currentAmount = order->totalAmount;
    save();
    return true;
}

// 接口实现 结账餐桌
bool RestaurantBackend::checkout(int tableId, std::string& error) {
    Order* order = findActiveOrderByTable(tableId);
    Table* table = findTable(tableId);
    if (order == 0 || table == 0) {
        error = "该餐桌没有可结账订单。";
        return false;
    }
    if (order->items.empty()) {
        error = "订单为空，不能结账。";
        return false;
    }
    // 订单转为已结账并释放餐桌
    order->status = OrderPaid;
    order->paidAt = nowText();
    table->status = TableIdle;
    table->currentAmount = 0.0;
    table->activeOrderId = 0;
    save();
    return true;
}

// 接口实现 取消订单
bool RestaurantBackend::cancelOrder(int orderId, std::string& error) {
    Order* order = findOrder(orderId);
    if (order == 0 || order->status == OrderPaid) {
        error = "订单不存在或已结账，不能取消。";
        return false;
    }
    // 取消订单时把菜品数量退回库存
    for (const OrderItem& item : order->items) {
        Dish* dish = findDish(item.dishId);
        if (dish != 0) {
            dish->stock += item.quantity;
            dish->sold = std::max(0, dish->sold - item.quantity);
        }
    }
    order->items.clear();
    order->totalAmount = 0.0;
    order->status = OrderCancelled;

    // 如果餐桌还绑定该订单则释放餐桌
    Table* table = findTable(order->tableId);
    if (table != 0 && table->activeOrderId == order->orderId) {
        table->status = TableIdle;
        table->currentAmount = 0.0;
        table->activeOrderId = 0;
    }
    save();
    return true;
}

std::string RestaurantBackend::money(double value) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << value;
    return os.str();
}

std::string RestaurantBackend::tableStatusName(int status) {
    if (status == TableIdle) return "空闲";
    if (status == TableUsing) return "使用中";
    return "待结账";
}

std::string RestaurantBackend::orderStatusName(int status) {
    if (status == OrderActive) return "进行中";
    if (status == OrderWaitPay) return "待结账";
    if (status == OrderPaid) return "已结账";
    return "已取消";
}

std::string RestaurantBackend::roleName(const std::string& role) {
    return role == "admin" ? "管理员" : "服务员";
}

// 私有查找接口实现
Dish* RestaurantBackend::findDish(int id) {
    return findPtr(dishes_, [id](const Dish& dish) { return dish.id == id; });
}

const Dish* RestaurantBackend::findDish(int id) const {
    return findPtr(dishes_, [id](const Dish& dish) { return dish.id == id; });
}

Table* RestaurantBackend::findTable(int id) {
    return findPtr(tables_, [id](const Table& table) { return table.id == id; });
}

const Table* RestaurantBackend::findTable(int id) const {
    return findPtr(tables_, [id](const Table& table) { return table.id == id; });
}

Order* RestaurantBackend::findOrder(int id) {
    return findPtr(orders_, [id](const Order& order) { return order.orderId == id; });
}

const Order* RestaurantBackend::findOrder(int id) const {
    return findPtr(orders_, [id](const Order& order) { return order.orderId == id; });
}

// 私有接口实现 根据餐桌查找未完成订单
Order* RestaurantBackend::findActiveOrderByTable(int tableId) {
    const Table* table = findTable(tableId);
    if (table != 0 && table->activeOrderId > 0) {
        return findOrder(table->activeOrderId);
    }
    for (Order& order : orders_) {
        if (order.tableId == tableId && unfinished(order.status)) {
            return &order;
        }
    }
    return 0;
}

// 私有接口实现 根据餐桌只读查找未完成订单
const Order* RestaurantBackend::findActiveOrderByTable(int tableId) const {
    const Table* table = findTable(tableId);
    if (table != 0 && table->activeOrderId > 0) {
        return findOrder(table->activeOrderId);
    }
    for (const Order& order : orders_) {
        if (order.tableId == tableId && unfinished(order.status)) {
            return &order;
        }
    }
    return 0;
}

// 私有接口实现 重新计算订单金额
void RestaurantBackend::recalcOrder(Order& order) {
    double total = 0.0;
    for (OrderItem& item : order.items) {
        item.subtotal = item.price * item.quantity;
        total += item.subtotal;
    }
    order.totalAmount = total;

    Table* table = findTable(order.tableId);
    if (table != 0 && table->activeOrderId == order.orderId) {
        table->currentAmount = total;
    }
}

// 私有接口实现 修正加载后的状态关系
bool RestaurantBackend::reconcileTableOrders() {
    bool changed = false;

    // 根据未完成订单修正餐桌状态
    for (Order& order : orders_) {
        if (!unfinished(order.status)) {
            continue;
        }

        double total = 0.0;
        // 重新计算订单明细小计和订单总额
        for (OrderItem& item : order.items) {
            const double subtotal = item.price * item.quantity;
            if (item.subtotal != subtotal) {
                item.subtotal = subtotal;
                changed = true;
            }
            total += subtotal;
        }
        if (order.totalAmount != total) {
            order.totalAmount = total;
            changed = true;
        }

        Table* table = findTable(order.tableId);
        if (table == 0) {
            continue;
        }

        const int expectedStatus = order.status == OrderWaitPay ? TableWaitPay : TableUsing;
        if (table->status != expectedStatus) {
            table->status = expectedStatus;
            changed = true;
        }
        if (table->activeOrderId != order.orderId) {
            table->activeOrderId = order.orderId;
            changed = true;
        }
        if (table->currentAmount != order.totalAmount) {
            table->currentAmount = order.totalAmount;
            changed = true;
        }
    }

    // 清理没有未完成订单绑定的餐桌
    for (Table& table : tables_) {
        if (table.status == TableIdle && table.activeOrderId == 0 && table.currentAmount == 0.0) {
            continue;
        }

        bool hasUnfinishedOrder = false;
        for (const Order& order : orders_) {
            if (order.tableId == table.id &&
                order.orderId == table.activeOrderId &&
                unfinished(order.status)) {
                hasUnfinishedOrder = true;
                break;
            }
        }

        if (!hasUnfinishedOrder) {
            table.status = TableIdle;
            table.currentAmount = 0.0;
            table.activeOrderId = 0;
            changed = true;
        }
    }

    return changed;
}

// 私有接口实现 判断菜品是否存在于未完成订单
bool RestaurantBackend::dishInUnpaidOrder(int dishId) const {
    for (const Order& order : orders_) {
        if (unfinished(order.status)) {
            for (const OrderItem& item : order.items) {
                if (item.dishId == dishId) {
                    return true;
                }
            }
        }
    }
    return false;
}

// 私有接口实现 统计管理员数量
int RestaurantBackend::countAdmins() const {
    int count = 0;
    for (const User& user : users_) {
        if (user.role == "admin") {
            ++count;
        }
    }
    return count;
}

}
