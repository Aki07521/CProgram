#include "restaurant_backend.hpp"

#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QDir>

namespace {
    QString qs(const std::string& value) {
        return QString::fromUtf8(value.c_str());
    }

    std::string ss(const QString& value) {
        return value.toUtf8().constData();
    }

    QTableWidgetItem* item(const QString& text) {
        QTableWidgetItem* cell = new QTableWidgetItem(text);
        cell->setFlags(cell->flags() ^ Qt::ItemIsEditable);
        return cell;
    }

    int selectedRow(QTableWidget* table) {
        QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
        if (ranges.isEmpty()) {
            return -1;
        }
        return ranges.first().topRow();
    }
}

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        backend_.load();
        stack_ = new QStackedWidget(this);
        setCentralWidget(stack_);
        buildLoginPage();
        buildWorkPage();
        stack_->setCurrentWidget(loginPage_);
        resize(1180, 760);
        setWindowTitle(QString::fromUtf8("中小饭店点餐管理系统 - Qt 前端"));
    }

private:
    Backend::RestaurantBackend backend_;
    QStackedWidget* stack_ = 0;
    QWidget* loginPage_ = 0;
    QWidget* workPage_ = 0;
    QComboBox* roleCombo_ = 0;
    QLineEdit* usernameEdit_ = 0;
    QLineEdit* passwordEdit_ = 0;
    QLabel* currentUserLabel_ = 0;
    QTabWidget* tabs_ = 0;
    QTableWidget* dishesTable_ = 0;
    QTableWidget* tablesTable_ = 0;
    QTableWidget* ordersTable_ = 0;
    QTableWidget* orderItemsTable_ = 0;
    QTableWidget* usersTable_ = 0;
    QPushButton* addUserButton_ = 0;
    QPushButton* editUserButton_ = 0;
    QPushButton* deleteUserButton_ = 0;
    QPushButton* addDishButton_ = 0;
    QPushButton* editDishButton_ = 0;
    QPushButton* deleteDishButton_ = 0;
    QPushButton* addTableButton_ = 0;
    QPushButton* editTableButton_ = 0;
    QPushButton* deleteTableButton_ = 0;

    QString currentRole_;
    QString currentUsername_;
    QString currentDisplayName_;

    void buildLoginPage() {
        loginPage_ = new QWidget;
        QVBoxLayout* outer = new QVBoxLayout(loginPage_);
        outer->setContentsMargins(320, 120, 320, 120);

        QGroupBox* box = new QGroupBox(QString::fromUtf8("系统登录"));
        QVBoxLayout* boxLayout = new QVBoxLayout(box);
        QLabel* title = new QLabel(QString::fromUtf8("中小饭店点餐管理系统"));
        QFont titleFont = title->font();
        titleFont.setPointSize(20);
        titleFont.setBold(true);
        title->setFont(titleFont);

        roleCombo_ = new QComboBox;
        roleCombo_->addItem(QString::fromUtf8("管理员"), "admin");
        roleCombo_->addItem(QString::fromUtf8("服务员"), "waiter");
        usernameEdit_ = new QLineEdit("admin");
        passwordEdit_ = new QLineEdit("123456");
        passwordEdit_->setEchoMode(QLineEdit::Password);

        QFormLayout* form = new QFormLayout;
        form->addRow(QString::fromUtf8("角色"), roleCombo_);
        form->addRow(QString::fromUtf8("账号"), usernameEdit_);
        form->addRow(QString::fromUtf8("密码"), passwordEdit_);

        QPushButton* loginButton = new QPushButton(QString::fromUtf8("登录"));
        connect(loginButton, &QPushButton::clicked, this, [this]() { login(); });
        connect(passwordEdit_, &QLineEdit::returnPressed, this, [this]() { login(); });

        boxLayout->addWidget(title);
        boxLayout->addLayout(form);
        boxLayout->addWidget(loginButton);
        outer->addWidget(box);
        stack_->addWidget(loginPage_);
    }

    void buildWorkPage() {
        workPage_ = new QWidget;
        QVBoxLayout* root = new QVBoxLayout(workPage_);

        QHBoxLayout* top = new QHBoxLayout;
        currentUserLabel_ = new QLabel;
        QPushButton* logout = new QPushButton(QString::fromUtf8("退出登录"));
        connect(logout, &QPushButton::clicked, this, [this]() {
            currentRole_.clear();
            currentUsername_.clear();
            stack_->setCurrentWidget(loginPage_);
        });
        top->addWidget(currentUserLabel_);
        top->addStretch();
        top->addWidget(logout);

        tabs_ = new QTabWidget;
        tabs_->addTab(buildDishesTab(), QString::fromUtf8("菜品"));
        tabs_->addTab(buildTablesTab(), QString::fromUtf8("餐桌"));
        tabs_->addTab(buildOrdersTab(), QString::fromUtf8("订单"));
        tabs_->addTab(buildUsersTab(), QString::fromUtf8("用户"));

        root->addLayout(top);
        root->addWidget(tabs_);
        stack_->addWidget(workPage_);
    }

    QWidget* buildDishesTab() {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        dishesTable_ = new QTableWidget;
        dishesTable_->setColumnCount(6);
        dishesTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("编号") << QString::fromUtf8("名称") << QString::fromUtf8("分类")
            << QString::fromUtf8("价格") << QString::fromUtf8("库存") << QString::fromUtf8("已售"));
        dishesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        dishesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        dishesTable_->setSelectionMode(QAbstractItemView::SingleSelection);

        QHBoxLayout* buttons = new QHBoxLayout;
        addDishButton_ = new QPushButton(QString::fromUtf8("新增菜品"));
        editDishButton_ = new QPushButton(QString::fromUtf8("修改菜品"));
        deleteDishButton_ = new QPushButton(QString::fromUtf8("删除菜品"));
        QPushButton* addToOrder = new QPushButton(QString::fromUtf8("加入订单"));
        connect(addDishButton_, &QPushButton::clicked, this, [this]() { editDish(false); });
        connect(editDishButton_, &QPushButton::clicked, this, [this]() { editDish(true); });
        connect(deleteDishButton_, &QPushButton::clicked, this, [this]() { deleteDish(); });
        connect(addToOrder, &QPushButton::clicked, this, [this]() { addDishToSelectedOrder(); });
        buttons->addWidget(addDishButton_);
        buttons->addWidget(editDishButton_);
        buttons->addWidget(deleteDishButton_);
        buttons->addStretch();
        buttons->addWidget(addToOrder);

        layout->addLayout(buttons);
        layout->addWidget(dishesTable_);
        return page;
    }

    QWidget* buildTablesTab() {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        tablesTable_ = new QTableWidget;
        tablesTable_->setColumnCount(5);
        tablesTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("桌号") << QString::fromUtf8("容量") << QString::fromUtf8("状态")
            << QString::fromUtf8("当前金额") << QString::fromUtf8("关联订单"));
        tablesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tablesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        tablesTable_->setSelectionMode(QAbstractItemView::SingleSelection);

        QHBoxLayout* buttons = new QHBoxLayout;
        addTableButton_ = new QPushButton(QString::fromUtf8("新增餐桌"));
        editTableButton_ = new QPushButton(QString::fromUtf8("修改容量"));
        deleteTableButton_ = new QPushButton(QString::fromUtf8("删除餐桌"));
        QPushButton* openTable = new QPushButton(QString::fromUtf8("开台"));
        QPushButton* waitPay = new QPushButton(QString::fromUtf8("设为待结账"));
        QPushButton* checkout = new QPushButton(QString::fromUtf8("结账"));
        connect(addTableButton_, &QPushButton::clicked, this, [this]() { editTable(false); });
        connect(editTableButton_, &QPushButton::clicked, this, [this]() { editTable(true); });
        connect(deleteTableButton_, &QPushButton::clicked, this, [this]() { deleteTable(); });
        connect(openTable, &QPushButton::clicked, this, [this]() { openSelectedTable(); });
        connect(waitPay, &QPushButton::clicked, this, [this]() { markSelectedTableWaitPay(); });
        connect(checkout, &QPushButton::clicked, this, [this]() { checkoutSelectedTable(); });
        buttons->addWidget(addTableButton_);
        buttons->addWidget(editTableButton_);
        buttons->addWidget(deleteTableButton_);
        buttons->addStretch();
        buttons->addWidget(openTable);
        buttons->addWidget(waitPay);
        buttons->addWidget(checkout);

        layout->addLayout(buttons);
        layout->addWidget(tablesTable_);
        return page;
    }

    QWidget* buildOrdersTab() {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);

        ordersTable_ = new QTableWidget;
        ordersTable_->setColumnCount(6);
        ordersTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("订单号") << QString::fromUtf8("桌号") << QString::fromUtf8("服务员")
            << QString::fromUtf8("状态") << QString::fromUtf8("金额") << QString::fromUtf8("创建时间"));
        ordersTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ordersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        ordersTable_->setSelectionMode(QAbstractItemView::SingleSelection);

        orderItemsTable_ = new QTableWidget;
        orderItemsTable_->setColumnCount(5);
        orderItemsTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("菜品编号") << QString::fromUtf8("菜品名称") << QString::fromUtf8("数量")
            << QString::fromUtf8("单价") << QString::fromUtf8("小计"));
        orderItemsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        connect(ordersTable_, &QTableWidget::itemSelectionChanged, this, [this]() { refreshOrderItems(); });

        QHBoxLayout* buttons = new QHBoxLayout;
        QPushButton* addDish = new QPushButton(QString::fromUtf8("加菜"));
        QPushButton* returnDish = new QPushButton(QString::fromUtf8("退菜"));
        QPushButton* cancelOrder = new QPushButton(QString::fromUtf8("取消订单"));
        connect(addDish, &QPushButton::clicked, this, [this]() { addDishToSelectedOrder(); });
        connect(returnDish, &QPushButton::clicked, this, [this]() { returnDishFromOrder(); });
        connect(cancelOrder, &QPushButton::clicked, this, [this]() { cancelSelectedOrder(); });
        buttons->addWidget(addDish);
        buttons->addWidget(returnDish);
        buttons->addWidget(cancelOrder);
        buttons->addStretch();

        layout->addWidget(new QLabel(QString::fromUtf8("订单列表")));
        layout->addWidget(ordersTable_, 2);
        layout->addLayout(buttons);
        layout->addWidget(new QLabel(QString::fromUtf8("订单明细")));
        layout->addWidget(orderItemsTable_, 1);
        return page;
    }

    QWidget* buildUsersTab() {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        usersTable_ = new QTableWidget;
        usersTable_->setColumnCount(4);
        usersTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("账号") << QString::fromUtf8("密码") << QString::fromUtf8("角色") << QString::fromUtf8("姓名"));
        usersTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        usersTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        usersTable_->setSelectionMode(QAbstractItemView::SingleSelection);

        QHBoxLayout* buttons = new QHBoxLayout;
        addUserButton_ = new QPushButton(QString::fromUtf8("新增用户"));
        editUserButton_ = new QPushButton(QString::fromUtf8("修改用户"));
        deleteUserButton_ = new QPushButton(QString::fromUtf8("删除用户"));
        connect(addUserButton_, &QPushButton::clicked, this, [this]() { editUser(false); });
        connect(editUserButton_, &QPushButton::clicked, this, [this]() { editUser(true); });
        connect(deleteUserButton_, &QPushButton::clicked, this, [this]() { deleteUser(); });
        buttons->addWidget(addUserButton_);
        buttons->addWidget(editUserButton_);
        buttons->addWidget(deleteUserButton_);
        buttons->addStretch();

        layout->addLayout(buttons);
        layout->addWidget(usersTable_);
        return page;
    }

    void login() {
        std::string display;
        std::string role = ss(roleCombo_->currentData().toString());
        bool ok = backend_.login(role, ss(usernameEdit_->text()), ss(passwordEdit_->text()), &display);
        if (!ok) {
            QMessageBox::warning(this, QString::fromUtf8("登录失败"), QString::fromUtf8("账号、密码或角色不匹配。"));
            return;
        }

        currentRole_ = roleCombo_->currentData().toString();
        currentUsername_ = usernameEdit_->text();
        currentDisplayName_ = qs(display);
        currentUserLabel_->setText(QString::fromUtf8("当前用户：%1（%2）")
            .arg(currentDisplayName_, qs(Backend::RestaurantBackend::roleName(role))));
        applyPermissions();
        refreshAll();
        stack_->setCurrentWidget(workPage_);
    }

    void applyPermissions() {
        bool admin = currentRole_ == "admin";
        addUserButton_->setEnabled(admin);
        editUserButton_->setEnabled(admin);
        deleteUserButton_->setEnabled(admin);
        addDishButton_->setEnabled(admin);
        editDishButton_->setEnabled(admin);
        deleteDishButton_->setEnabled(admin);
        addTableButton_->setEnabled(admin);
        editTableButton_->setEnabled(admin);
        deleteTableButton_->setEnabled(admin);
        tabs_->setTabEnabled(3, admin);
    }

    void refreshAll() {
        refreshDishes();
        refreshTables();
        refreshOrders();
        refreshUsers();
        refreshOrderItems();
    }

    void refreshDishes() {
        const std::vector<Backend::Dish>& dishes = backend_.dishes();
        dishesTable_->setRowCount(static_cast<int>(dishes.size()));
        for (int row = 0; row < static_cast<int>(dishes.size()); ++row) {
            const Backend::Dish& d = dishes[static_cast<size_t>(row)];
            dishesTable_->setItem(row, 0, item(QString::number(d.id)));
            dishesTable_->setItem(row, 1, item(qs(d.name)));
            dishesTable_->setItem(row, 2, item(qs(d.category)));
            dishesTable_->setItem(row, 3, item(qs(Backend::RestaurantBackend::money(d.price))));
            dishesTable_->setItem(row, 4, item(QString::number(d.stock)));
            dishesTable_->setItem(row, 5, item(QString::number(d.sold)));
        }
    }

    void refreshTables() {
        const std::vector<Backend::Table>& tables = backend_.tables();
        tablesTable_->setRowCount(static_cast<int>(tables.size()));
        for (int row = 0; row < static_cast<int>(tables.size()); ++row) {
            const Backend::Table& t = tables[static_cast<size_t>(row)];
            tablesTable_->setItem(row, 0, item(QString::number(t.id)));
            tablesTable_->setItem(row, 1, item(QString::number(t.capacity)));
            tablesTable_->setItem(row, 2, item(qs(Backend::RestaurantBackend::tableStatusName(t.status))));
            tablesTable_->setItem(row, 3, item(qs(Backend::RestaurantBackend::money(t.currentAmount))));
            tablesTable_->setItem(row, 4, item(QString::number(t.activeOrderId)));
        }
    }

    void refreshOrders() {
        const std::vector<Backend::Order>& orders = backend_.orders();
        ordersTable_->setRowCount(static_cast<int>(orders.size()));
        for (int row = 0; row < static_cast<int>(orders.size()); ++row) {
            const Backend::Order& o = orders[static_cast<size_t>(row)];
            ordersTable_->setItem(row, 0, item(QString::number(o.orderId)));
            ordersTable_->setItem(row, 1, item(QString::number(o.tableId)));
            ordersTable_->setItem(row, 2, item(qs(o.waiter)));
            ordersTable_->setItem(row, 3, item(qs(Backend::RestaurantBackend::orderStatusName(o.status))));
            ordersTable_->setItem(row, 4, item(qs(Backend::RestaurantBackend::money(o.totalAmount))));
            ordersTable_->setItem(row, 5, item(qs(o.createdAt)));
        }
    }

    void refreshUsers() {
        const std::vector<Backend::User>& users = backend_.users();
        usersTable_->setRowCount(static_cast<int>(users.size()));
        for (int row = 0; row < static_cast<int>(users.size()); ++row) {
            const Backend::User& u = users[static_cast<size_t>(row)];
            usersTable_->setItem(row, 0, item(qs(u.username)));
            usersTable_->setItem(row, 1, item(qs(u.password)));
            usersTable_->setItem(row, 2, item(qs(Backend::RestaurantBackend::roleName(u.role))));
            usersTable_->setItem(row, 3, item(qs(u.name)));
        }
    }

    void refreshOrderItems() {
        int row = selectedRow(ordersTable_);
        if (row < 0 || ordersTable_->item(row, 0) == 0) {
            orderItemsTable_->setRowCount(0);
            return;
        }
        int orderId = ordersTable_->item(row, 0)->text().toInt();
        const std::vector<Backend::Order>& orders = backend_.orders();
        for (size_t i = 0; i < orders.size(); ++i) {
            if (orders[i].orderId == orderId) {
                orderItemsTable_->setRowCount(static_cast<int>(orders[i].items.size()));
                for (int r = 0; r < static_cast<int>(orders[i].items.size()); ++r) {
                    const Backend::OrderItem& it = orders[i].items[static_cast<size_t>(r)];
                    orderItemsTable_->setItem(r, 0, item(QString::number(it.dishId)));
                    orderItemsTable_->setItem(r, 1, item(qs(it.dishName)));
                    orderItemsTable_->setItem(r, 2, item(QString::number(it.quantity)));
                    orderItemsTable_->setItem(r, 3, item(qs(Backend::RestaurantBackend::money(it.price))));
                    orderItemsTable_->setItem(r, 4, item(qs(Backend::RestaurantBackend::money(it.subtotal))));
                }
                return;
            }
        }
        orderItemsTable_->setRowCount(0);
    }

    int selectedInt(QTableWidget* table, int column, const QString& title) {
        int row = selectedRow(table);
        if (row < 0 || table->item(row, column) == 0) {
            QMessageBox::information(this, title, QString::fromUtf8("请先选择一行。"));
            return -1;
        }
        return table->item(row, column)->text().toInt();
    }

    void showBackendError(const std::string& error) {
        QMessageBox::warning(this, QString::fromUtf8("操作失败"), qs(error));
    }

    void editDish(bool existing) {
        Backend::Dish dish;
        if (existing) {
            int row = selectedRow(dishesTable_);
            if (row < 0) {
                QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择菜品。"));
                return;
            }
            dish.id = dishesTable_->item(row, 0)->text().toInt();
            dish.name = ss(dishesTable_->item(row, 1)->text());
            dish.category = ss(dishesTable_->item(row, 2)->text());
            dish.price = dishesTable_->item(row, 3)->text().toDouble();
            dish.stock = dishesTable_->item(row, 4)->text().toInt();
            dish.sold = dishesTable_->item(row, 5)->text().toInt();
        }

        QDialog dialog(this);
        dialog.setWindowTitle(existing ? QString::fromUtf8("修改菜品") : QString::fromUtf8("新增菜品"));
        QFormLayout form(&dialog);
        QSpinBox id; id.setRange(1, 999999); id.setValue(existing ? dish.id : 1001); id.setEnabled(!existing);
        QLineEdit name(qs(dish.name));
        QLineEdit category(qs(dish.category));
        QDoubleSpinBox price; price.setRange(0.01, 99999); price.setDecimals(2); price.setValue(existing ? dish.price : 1.0);
        QSpinBox stock; stock.setRange(0, 999999); stock.setValue(dish.stock);
        QPushButton ok(QString::fromUtf8("确定"));
        form.addRow(QString::fromUtf8("编号"), &id);
        form.addRow(QString::fromUtf8("名称"), &name);
        form.addRow(QString::fromUtf8("分类"), &category);
        form.addRow(QString::fromUtf8("价格"), &price);
        form.addRow(QString::fromUtf8("库存"), &stock);
        form.addRow(&ok);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        dish.id = id.value();
        dish.name = ss(name.text());
        dish.category = ss(category.text());
        dish.price = price.value();
        dish.stock = stock.value();
        std::string error;
        bool okResult = existing ? backend_.updateDish(dish, error) : backend_.addDish(dish, error);
        if (!okResult) showBackendError(error);
        refreshAll();
    }

    void deleteDish() {
        int id = selectedInt(dishesTable_, 0, QString::fromUtf8("删除菜品"));
        if (id < 0) return;
        std::string error;
        if (!backend_.deleteDish(id, error)) showBackendError(error);
        refreshAll();
    }

    void editTable(bool existing) {
        Backend::Table table;
        if (existing) {
            int row = selectedRow(tablesTable_);
            if (row < 0) {
                QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择餐桌。"));
                return;
            }
            table.id = tablesTable_->item(row, 0)->text().toInt();
            table.capacity = tablesTable_->item(row, 1)->text().toInt();
        }

        QDialog dialog(this);
        dialog.setWindowTitle(existing ? QString::fromUtf8("修改餐桌") : QString::fromUtf8("新增餐桌"));
        QFormLayout form(&dialog);
        QSpinBox id; id.setRange(1, 999999); id.setValue(existing ? table.id : 1); id.setEnabled(!existing);
        QSpinBox capacity; capacity.setRange(1, 99); capacity.setValue(existing ? table.capacity : 4);
        QPushButton ok(QString::fromUtf8("确定"));
        form.addRow(QString::fromUtf8("桌号"), &id);
        form.addRow(QString::fromUtf8("容量"), &capacity);
        form.addRow(&ok);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        table.id = id.value();
        table.capacity = capacity.value();
        std::string error;
        bool okResult = existing ? backend_.updateTable(table, error) : backend_.addTable(table, error);
        if (!okResult) showBackendError(error);
        refreshAll();
    }

    void deleteTable() {
        int id = selectedInt(tablesTable_, 0, QString::fromUtf8("删除餐桌"));
        if (id < 0) return;
        std::string error;
        if (!backend_.deleteTable(id, error)) showBackendError(error);
        refreshAll();
    }

    void editUser(bool existing) {
        Backend::User user;
        if (existing) {
            int row = selectedRow(usersTable_);
            if (row < 0) {
                QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择用户。"));
                return;
            }
            user.username = ss(usersTable_->item(row, 0)->text());
            user.password = ss(usersTable_->item(row, 1)->text());
            user.role = usersTable_->item(row, 2)->text() == QString::fromUtf8("管理员") ? "admin" : "waiter";
            user.name = ss(usersTable_->item(row, 3)->text());
        }

        QDialog dialog(this);
        dialog.setWindowTitle(existing ? QString::fromUtf8("修改用户") : QString::fromUtf8("新增用户"));
        QFormLayout form(&dialog);
        QLineEdit username(qs(user.username)); username.setEnabled(!existing);
        QLineEdit password(qs(user.password));
        QComboBox role; role.addItem(QString::fromUtf8("管理员"), "admin"); role.addItem(QString::fromUtf8("服务员"), "waiter");
        role.setCurrentIndex(user.role == "waiter" ? 1 : 0);
        QLineEdit name(qs(user.name));
        QPushButton ok(QString::fromUtf8("确定"));
        form.addRow(QString::fromUtf8("账号"), &username);
        form.addRow(QString::fromUtf8("密码"), &password);
        form.addRow(QString::fromUtf8("角色"), &role);
        form.addRow(QString::fromUtf8("姓名"), &name);
        form.addRow(&ok);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        user.username = ss(username.text());
        user.password = ss(password.text());
        user.role = ss(role.currentData().toString());
        user.name = ss(name.text());
        std::string error;
        bool okResult = existing ? backend_.updateUser(user, error) : backend_.addUser(user, error);
        if (!okResult) showBackendError(error);
        refreshAll();
    }

    void deleteUser() {
        int row = selectedRow(usersTable_);
        if (row < 0) {
            QMessageBox::information(this, QString::fromUtf8("删除用户"), QString::fromUtf8("请先选择用户。"));
            return;
        }
        std::string error;
        if (!backend_.deleteUser(ss(usersTable_->item(row, 0)->text()), error)) showBackendError(error);
        refreshAll();
    }

    void openSelectedTable() {
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("开台"));
        if (tableId < 0) return;
        int orderId = 0;
        std::string error;
        if (!backend_.openTable(tableId, ss(currentUsername_), orderId, error)) showBackendError(error);
        refreshAll();
    }

    int selectedOrderId() {
        return selectedInt(ordersTable_, 0, QString::fromUtf8("订单操作"));
    }

    void addDishToSelectedOrder() {
        int orderId = selectedOrderId();
        if (orderId < 0) return;
        int dishId = QInputDialog::getInt(this, QString::fromUtf8("加菜"), QString::fromUtf8("菜品编号"), 1001, 1, 999999);
        int quantity = QInputDialog::getInt(this, QString::fromUtf8("加菜"), QString::fromUtf8("数量"), 1, 1, 999999);
        std::string error;
        if (!backend_.addDishToOrder(orderId, dishId, quantity, error)) showBackendError(error);
        refreshAll();
    }

    void returnDishFromOrder() {
        int orderId = selectedOrderId();
        if (orderId < 0) return;
        int dishId = QInputDialog::getInt(this, QString::fromUtf8("退菜"), QString::fromUtf8("菜品编号"), 1001, 1, 999999);
        int quantity = QInputDialog::getInt(this, QString::fromUtf8("退菜"), QString::fromUtf8("数量"), 1, 1, 999999);
        std::string error;
        if (!backend_.returnDish(orderId, dishId, quantity, error)) showBackendError(error);
        refreshAll();
    }

    void markSelectedTableWaitPay() {
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("待结账"));
        if (tableId < 0) return;
        std::string error;
        if (!backend_.markWaitPay(tableId, error)) showBackendError(error);
        refreshAll();
    }

    void checkoutSelectedTable() {
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("结账"));
        if (tableId < 0) return;
        std::string error;
        if (!backend_.checkout(tableId, error)) showBackendError(error);
        refreshAll();
    }

    void cancelSelectedOrder() {
        int orderId = selectedOrderId();
        if (orderId < 0) return;
        std::string error;
        if (!backend_.cancelOrder(orderId, error)) showBackendError(error);
        refreshAll();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QDir dataDir(QApplication::applicationDirPath());
    dataDir.cdUp();
    Backend::RestaurantBackend::setDataDirectory(dataDir.absolutePath().toStdString());
    MainWindow window;
    window.show();
    return app.exec();
}
