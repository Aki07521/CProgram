#pragma once

#include <cstddef>
#include <string>

// 后端数据结构和业务接口
namespace Backend {

// 链表节点模板
template <typename T>
struct ListNode {
    T data;
    ListNode* next;
    explicit ListNode(const T& value) : data(value), next(0) {}
};

// 单向链表模板
template <typename T>
class LinkedList {
public:
    typedef T ValueType;

    // 可修改链表迭代器
    class Iterator {
    public:
        explicit Iterator(ListNode<T>* node) : node_(node) {}
        T& operator*() const { return node_->data; }
        Iterator& operator++() { node_ = node_->next; return *this; }
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
    private:
        ListNode<T>* node_;
    };

    // 只读链表迭代器
    class ConstIterator {
    public:
        explicit ConstIterator(const ListNode<T>* node) : node_(node) {}
        const T& operator*() const { return node_->data; }
        ConstIterator& operator++() { node_ = node_->next; return *this; }
        bool operator!=(const ConstIterator& other) const { return node_ != other.node_; }
    private:
        const ListNode<T>* node_;
    };

    LinkedList() : head_(0), tail_(0), size_(0) {}
    LinkedList(const LinkedList& other) : head_(0), tail_(0), size_(0) { copyFrom(other); }
    // 析构时释放全部节点
    ~LinkedList() { clear(); }

    // 赋值时先清空旧节点再复制新节点
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ListNode<T>* head() { return head_; }
    const ListNode<T>* head() const { return head_; }
    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(0); }
    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(0); }
    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

    // 清空链表并释放节点
    void clear() {
        while (head_ != 0) {
            ListNode<T>* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = 0;
        size_ = 0;
    }

    // 在链表尾部追加数据节点
    void push_back(const T& value) {
        ListNode<T>* node = new ListNode<T>(value);
        if (tail_ == 0) {
            head_ = node;
        } else {
            tail_->next = node;
        }
        tail_ = node;
        ++size_;
    }

    // 删除第一个满足条件的节点
    template <typename Predicate>
    bool removeFirst(Predicate predicate) {
        ListNode<T>* previous = 0;
        ListNode<T>* current = head_;
        while (current != 0) {
            if (predicate(current->data)) {
                if (previous == 0) {
                    head_ = current->next;
                } else {
                    previous->next = current->next;
                }
                if (tail_ == current) {
                    tail_ = previous;
                }
                delete current;
                --size_;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

private:
    ListNode<T>* head_;
    ListNode<T>* tail_;
    std::size_t size_;

    // 从另一个链表复制所有节点
    void copyFrom(const LinkedList& other) {
        for (const ListNode<T>* node = other.head_; node != 0; node = node->next) {
            push_back(node->data);
        }
    }
};

// 餐桌状态枚举
enum TableStatus {
    TableIdle = 0,
    TableUsing = 1,
    TableWaitPay = 2
};

// 订单状态枚举
enum OrderStatus {
    OrderActive = 0,
    OrderWaitPay = 1,
    OrderPaid = 2,
    OrderCancelled = 3
};

// 用户数据结构
struct User {
    std::string username;
    std::string password;
    std::string role;
    std::string name;
};

// 菜品数据结构
struct Dish {
    int id = 0;
    std::string name;
    std::string category;
    double price = 0.0;
    int stock = 0;
    int sold = 0;
};

// 餐桌数据结构
struct Table {
    int id = 0;
    int capacity = 0;
    int status = TableIdle;
    double currentAmount = 0.0;
    int activeOrderId = 0;
};

// 订单明细数据结构
struct OrderItem {
    int orderId = 0;
    int dishId = 0;
    std::string dishName;
    int quantity = 0;
    double price = 0.0;
    double subtotal = 0.0;
};

// 订单数据结构
struct Order {
    int orderId = 0;
    int tableId = 0;
    std::string waiter;
    int status = OrderActive;
    double totalAmount = 0.0;
    std::string createdAt;
    std::string paidAt;
    // 订单明细链表
    LinkedList<OrderItem> items;
};

// 后端业务类
class RestaurantBackend {
public:
    // 接口位置 设置数据文件目录
    static void setDataDirectory(const std::string& directory);
    // 接口位置 从数据文件加载链表
    bool load();
    // 接口位置 将链表数据保存到文件
    void save() const;
    // 接口位置 清除历史订单数据
    bool clearHistoricalData(std::string& error);
    // 接口位置 登录校验
    bool login(const std::string& role, const std::string& username,
               const std::string& password, std::string* displayName = 0) const;

    // 接口位置 获取用户链表
    const LinkedList<User>& users() const;
    // 接口位置 获取菜品链表
    const LinkedList<Dish>& dishes() const;
    // 接口位置 获取餐桌链表
    const LinkedList<Table>& tables() const;
    // 接口位置 获取订单链表
    const LinkedList<Order>& orders() const;

    // 接口位置 新增用户
    bool addUser(const User& user, std::string& error);
    // 接口位置 修改用户
    bool updateUser(const User& user, std::string& error);
    // 接口位置 删除用户
    bool deleteUser(const std::string& username, std::string& error);

    // 接口位置 新增菜品
    bool addDish(const Dish& dish, std::string& error);
    // 接口位置 修改菜品
    bool updateDish(const Dish& dish, std::string& error);
    // 接口位置 删除菜品
    bool deleteDish(int id, std::string& error);
    // 接口位置 新增餐桌
    bool addTable(const Table& table, std::string& error);
    // 接口位置 修改餐桌
    bool updateTable(const Table& table, std::string& error);
    // 接口位置 删除餐桌
    bool deleteTable(int id, std::string& error);
    // 接口位置 开台并创建订单
    bool openTable(int tableId, const std::string& waiter, int& orderId, std::string& error);
    // 接口位置 向订单添加菜品
    bool addDishToOrder(int orderId, int dishId, int quantity, std::string& error);
    // 接口位置 从订单退菜
    bool returnDish(int orderId, int dishId, int quantity, std::string& error);
    // 接口位置 将餐桌订单设置为待结账
    bool markWaitPay(int tableId, std::string& error);
    // 接口位置 结账指定餐桌
    bool checkout(int tableId, std::string& error);
    // 接口位置 取消订单
    bool cancelOrder(int orderId, std::string& error);

    // 接口位置 格式化金额
    static std::string money(double value);
    // 接口位置 获取餐桌状态文本
    static std::string tableStatusName(int status);
    // 接口位置 获取订单状态文本
    static std::string orderStatusName(int status);
    // 接口位置 获取角色文本
    static std::string roleName(const std::string& role);

private:
    LinkedList<User> users_;
    LinkedList<Dish> dishes_;
    LinkedList<Table> tables_;
    LinkedList<Order> orders_;

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
    bool reconcileTableOrders();
    bool dishInUnpaidOrder(int dishId) const;
    int countAdmins() const;
};

}

