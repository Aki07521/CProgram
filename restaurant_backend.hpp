#ifndef RESTAURANT_BACKEND_HPP
#define RESTAURANT_BACKEND_HPP

#include <string>
#include <vector>

namespace Backend {

enum TableStatus {
    TableIdle = 0,
    TableUsing = 1,
    TableWaitPay = 2
};

enum OrderStatus {
    OrderActive = 0,
    OrderWaitPay = 1,
    OrderPaid = 2,
    OrderCancelled = 3
};

struct User {
    std::string username;
    std::string password;
    std::string role;
    std::string name;
};

struct Dish {
    int id = 0;
    std::string name;
    std::string category;
    double price = 0.0;
    int stock = 0;
    int sold = 0;
};

struct Table {
    int id = 0;
    int capacity = 0;
    int status = TableIdle;
    double currentAmount = 0.0;
    int activeOrderId = 0;
};

struct OrderItem {
    int orderId = 0;
    int dishId = 0;
    std::string dishName;
    int quantity = 0;
    double price = 0.0;
    double subtotal = 0.0;
};

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

class RestaurantBackend {
public:
    static void setDataDirectory(const std::string& directory);

    bool load();
    void save() const;
    void resetDemoData();

    bool login(const std::string& role, const std::string& username,
               const std::string& password, std::string* displayName = 0) const;

    const std::vector<User>& users() const;
    const std::vector<Dish>& dishes() const;
    const std::vector<Table>& tables() const;
    const std::vector<Order>& orders() const;

    bool addUser(const User& user, std::string& error);
    bool updateUser(const User& user, std::string& error);
    bool deleteUser(const std::string& username, std::string& error);

    bool addDish(const Dish& dish, std::string& error);
    bool updateDish(const Dish& dish, std::string& error);
    bool deleteDish(int id, std::string& error);

    bool addTable(const Table& table, std::string& error);
    bool updateTable(const Table& table, std::string& error);
    bool deleteTable(int id, std::string& error);

    bool openTable(int tableId, const std::string& waiter, int& orderId, std::string& error);
    bool addDishToOrder(int orderId, int dishId, int quantity, std::string& error);
    bool returnDish(int orderId, int dishId, int quantity, std::string& error);
    bool markWaitPay(int tableId, std::string& error);
    bool checkout(int tableId, std::string& error);
    bool cancelOrder(int orderId, std::string& error);

    static std::string money(double value);
    static std::string tableStatusName(int status);
    static std::string orderStatusName(int status);
    static std::string roleName(const std::string& role);

private:
    std::vector<User> users_;
    std::vector<Dish> dishes_;
    std::vector<Table> tables_;
    std::vector<Order> orders_;
    int nextOrderId_ = 1;

    Dish* findDish(int id);
    const Dish* findDish(int id) const;
    Table* findTable(int id);
    const Table* findTable(int id) const;
    Order* findOrder(int id);
    const Order* findOrder(int id) const;
    Order* findActiveOrderByTable(int tableId);
    const Order* findActiveOrderByTable(int tableId) const;

    void recalcOrder(Order& order);
    bool dishInUnpaidOrder(int dishId) const;
    int countAdmins() const;
};

}

#endif
