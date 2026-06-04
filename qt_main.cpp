#include "restaurant_backend.hpp"

#include <QtWidgets/QAbstractItemView>
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

// Qt 前端实现文件。
// 这个文件只负责界面、输入弹窗、按钮事件和表格刷新；
// 具体业务规则全部交给 RestaurantBackend，保持前后端分离。
namespace {
    // std::string -> QString。后端统一使用 UTF-8 字符串，Qt 显示前转成 QString。
    QString qs(const std::string& value) {
        return QString::fromUtf8(value.c_str());
    }

    // QString -> std::string。前端输入框的内容传给后端前统一转成 UTF-8。
    std::string ss(const QString& value) {
        return value.toUtf8().constData();
    }

    // 创建只读表格单元格，避免用户直接在表格里改数据而绕过后端校验。
    QTableWidgetItem* item(const QString& text) {
        QTableWidgetItem* cell = new QTableWidgetItem(text);
        cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
        cell->setTextAlignment(Qt::AlignCenter);
        return cell;
    }

    // 统一表格视觉：去掉焦点黄框、禁止直接编辑、数字和文字居中。
    void configureTable(QTableWidget* table) {
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setAlternatingRowColors(true);
        table->setShowGrid(true);
        table->setGridStyle(Qt::SolidLine);
        table->verticalHeader()->setDefaultSectionSize(34);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget {"
            "  background: #ffffff;"
            "  alternate-background-color: #f8fafc;"
            "  color: #111827;"
            "  border: 1px solid #d7dee8;"
            "  border-radius: 6px;"
            "  gridline-color: #e5eaf1;"
            "  outline: 0;"
            "}"
            "QTableWidget::item {"
            "  color: #111827;"
            "  padding: 6px;"
            "  border: 0;"
            "}"
            "QTableWidget::item:alternate {"
            "  color: #111827;"
            "  background: #f8fafc;"
            "}"
            "QTableWidget::item:selected {"
            "  background: #cfe4ff;"
            "  color: #111827;"
            "}"
            "QTableWidget::item:selected:!active {"
            "  background: #dbeafe;"
            "  color: #111827;"
            "}"
            "QTableWidget::item:focus {"
            "  border: 0;"
            "  outline: none;"
            "}"
            "QHeaderView::section {"
            "  background: #edf2f7;"
            "  color: #111827;"
            "  border: 0;"
            "  border-right: 1px solid #d7dee8;"
            "  border-bottom: 1px solid #d7dee8;"
            "  padding: 7px;"
            "  font-weight: 600;"
            "}"
            "QTableCornerButton::section {"
            "  background: #edf2f7;"
            "  border: 0;"
            "}"
        );
    }

    // 返回当前表格选中的第一行。所有“修改/删除/加菜”等操作都依赖这个工具函数。
    int selectedRow(QTableWidget* table) {
        QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
        if (ranges.isEmpty()) {
            return -1;
        }
        return ranges.first().topRow();
    }
}

// 主窗口负责把登录页、工作页和所有业务操作串起来。
// 它持有一个后端对象 backend_，界面事件只调用后端公开接口，不直接改数据文件。
class MainWindow : public QMainWindow {
public:
    MainWindow() {
        // 启动时先加载数据，再搭建页面；页面刷新时会直接读取 backend_ 内存数据。
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
    // 后端实例：保存当前内存数据，负责所有业务规则和持久化。
    Backend::RestaurantBackend backend_;

    // 页面容器。stack_ 用于在登录页和工作页之间切换。
    QStackedWidget* stack_ = 0;
    QWidget* loginPage_ = 0;
    QWidget* workPage_ = 0;

    // 登录页控件。
    QComboBox* roleCombo_ = 0;
    QLineEdit* usernameEdit_ = 0;
    QLineEdit* passwordEdit_ = 0;

    // 工作页顶部和标签页。
    QLabel* currentUserLabel_ = 0;
    QTabWidget* tabs_ = 0;

    // 各业务表格。表格只负责展示，真实数据始终以 backend_ 为准。
    QTableWidget* dishesTable_ = 0;
    QTableWidget* tablesTable_ = 0;
    QTableWidget* ordersTable_ = 0;
    QTableWidget* orderItemsTable_ = 0;
    QTableWidget* usersTable_ = 0;
    QTableWidget* dataSummaryTable_ = 0;
    QTableWidget* dishStatsTable_ = 0;

    // 这些按钮只有管理员可用，登录成功后由 applyPermissions() 统一控制。
    QPushButton* addUserButton_ = 0;
    QPushButton* editUserButton_ = 0;
    QPushButton* deleteUserButton_ = 0;
    QPushButton* addDishButton_ = 0;
    QPushButton* editDishButton_ = 0;
    QPushButton* deleteDishButton_ = 0;
    QPushButton* addTableButton_ = 0;
    QPushButton* editTableButton_ = 0;
    QPushButton* deleteTableButton_ = 0;

    // 当前登录用户信息。业务操作需要知道当前角色和服务员账号。
    QString currentRole_;
    QString currentUsername_;
    QString currentDisplayName_;

    void buildLoginPage() {
        // 登录页只收集角色、账号、密码；是否能登录由后端 login() 判断。
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
        // 点击登录或在密码框按回车，走同一套登录逻辑。
        connect(loginButton, &QPushButton::clicked, this, [this]() { login(); });
        connect(passwordEdit_, &QLineEdit::returnPressed, this, [this]() { login(); });

        boxLayout->addWidget(title);
        boxLayout->addLayout(form);
        boxLayout->addWidget(loginButton);
        outer->addWidget(box);
        stack_->addWidget(loginPage_);
    }

    void buildWorkPage() {
        // 工作页由顶部用户信息栏和下方业务标签页组成。
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
        tabs_->addTab(buildDataTab(), QString::fromUtf8("数据查看"));
        tabs_->addTab(buildUsersTab(), QString::fromUtf8("用户"));

        root->addLayout(top);
        root->addWidget(tabs_);
        stack_->addWidget(workPage_);
    }

    QWidget* buildDishesTab() {
        // 菜品页：管理员维护菜品，服务员也可以从这里向选中订单加菜。
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        dishesTable_ = new QTableWidget;
        dishesTable_->setColumnCount(6);
        dishesTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("编号") << QString::fromUtf8("名称") << QString::fromUtf8("分类")
            << QString::fromUtf8("价格") << QString::fromUtf8("库存") << QString::fromUtf8("已售"));
        configureTable(dishesTable_);

        QHBoxLayout* buttons = new QHBoxLayout;
        addDishButton_ = new QPushButton(QString::fromUtf8("新增菜品"));
        editDishButton_ = new QPushButton(QString::fromUtf8("修改菜品"));
        deleteDishButton_ = new QPushButton(QString::fromUtf8("删除菜品"));
        connect(addDishButton_, &QPushButton::clicked, this, [this]() { editDish(false); });
        connect(editDishButton_, &QPushButton::clicked, this, [this]() { editDish(true); });
        connect(deleteDishButton_, &QPushButton::clicked, this, [this]() { deleteDish(); });
        buttons->addWidget(addDishButton_);
        buttons->addWidget(editDishButton_);
        buttons->addWidget(deleteDishButton_);
        buttons->addStretch();

        layout->addLayout(buttons);
        layout->addWidget(dishesTable_);
        return page;
    }

    QWidget* buildTablesTab() {
        // 餐桌页：展示桌台状态，也是开台、待结账、结账流程的入口。
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        tablesTable_ = new QTableWidget;
        tablesTable_->setColumnCount(5);
        tablesTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("桌号") << QString::fromUtf8("容量") << QString::fromUtf8("状态")
            << QString::fromUtf8("当前金额") << QString::fromUtf8("关联订单"));
        configureTable(tablesTable_);

        QHBoxLayout* buttons = new QHBoxLayout;
        addTableButton_ = new QPushButton(QString::fromUtf8("新增餐桌"));
        editTableButton_ = new QPushButton(QString::fromUtf8("修改容量"));
        deleteTableButton_ = new QPushButton(QString::fromUtf8("删除餐桌"));
        QPushButton* openTable = new QPushButton(QString::fromUtf8("开台/进入订单"));
        QPushButton* checkout = new QPushButton(QString::fromUtf8("本桌结账"));
        connect(addTableButton_, &QPushButton::clicked, this, [this]() { editTable(false); });
        connect(editTableButton_, &QPushButton::clicked, this, [this]() { editTable(true); });
        connect(deleteTableButton_, &QPushButton::clicked, this, [this]() { deleteTable(); });
        connect(openTable, &QPushButton::clicked, this, [this]() { openSelectedTable(); });
        connect(checkout, &QPushButton::clicked, this, [this]() { checkoutSelectedTable(); });
        buttons->addWidget(addTableButton_);
        buttons->addWidget(editTableButton_);
        buttons->addWidget(deleteTableButton_);
        buttons->addStretch();
        buttons->addWidget(openTable);
        buttons->addWidget(checkout);

        layout->addLayout(buttons);
        layout->addWidget(tablesTable_);
        return page;
    }

    QWidget* buildOrdersTab() {
        // 订单页：上方显示订单主表，下方显示当前选中订单的明细。
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);

        ordersTable_ = new QTableWidget;
        ordersTable_->setColumnCount(6);
        ordersTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("订单号") << QString::fromUtf8("桌号") << QString::fromUtf8("服务员")
            << QString::fromUtf8("状态") << QString::fromUtf8("金额") << QString::fromUtf8("创建时间"));
        configureTable(ordersTable_);

        orderItemsTable_ = new QTableWidget;
        orderItemsTable_->setColumnCount(5);
        orderItemsTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("菜品编号") << QString::fromUtf8("菜品名称") << QString::fromUtf8("数量")
            << QString::fromUtf8("单价") << QString::fromUtf8("小计"));
        configureTable(orderItemsTable_);

        connect(ordersTable_, &QTableWidget::itemSelectionChanged, this, [this]() { refreshOrderItems(); });

        QHBoxLayout* buttons = new QHBoxLayout;
        QPushButton* addDish = new QPushButton(QString::fromUtf8("加菜"));
        QPushButton* returnDish = new QPushButton(QString::fromUtf8("退菜"));
        QPushButton* waitPay = new QPushButton(QString::fromUtf8("设为待结账"));
        QPushButton* checkout = new QPushButton(QString::fromUtf8("结账"));
        QPushButton* cancelOrder = new QPushButton(QString::fromUtf8("取消订单"));
        connect(addDish, &QPushButton::clicked, this, [this]() { addDishToSelectedOrder(); });
        connect(returnDish, &QPushButton::clicked, this, [this]() { returnDishFromOrder(); });
        connect(waitPay, &QPushButton::clicked, this, [this]() { markSelectedOrderWaitPay(); });
        connect(checkout, &QPushButton::clicked, this, [this]() { checkoutSelectedOrder(); });
        connect(cancelOrder, &QPushButton::clicked, this, [this]() { cancelSelectedOrder(); });
        buttons->addWidget(addDish);
        buttons->addWidget(returnDish);
        buttons->addWidget(waitPay);
        buttons->addWidget(checkout);
        buttons->addWidget(cancelOrder);
        buttons->addStretch();

        layout->addWidget(new QLabel(QString::fromUtf8("订单列表")));
        layout->addWidget(ordersTable_, 2);
        layout->addLayout(buttons);
        layout->addWidget(new QLabel(QString::fromUtf8("订单明细")));
        layout->addWidget(orderItemsTable_, 1);
        return page;
    }

    QWidget* buildDataTab() {
        // 管理员数据页：集中查看经营概览、订单状态和菜品销售数据。
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);

        QLabel* summaryTitle = new QLabel(QString::fromUtf8("经营数据总览"));
        QFont titleFont = summaryTitle->font();
        titleFont.setBold(true);
        titleFont.setPointSize(12);
        summaryTitle->setFont(titleFont);

        dataSummaryTable_ = new QTableWidget;
        dataSummaryTable_->setColumnCount(2);
        dataSummaryTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("指标") << QString::fromUtf8("数值"));
        configureTable(dataSummaryTable_);

        QLabel* dishTitle = new QLabel(QString::fromUtf8("菜品销量与库存"));
        dishTitle->setFont(titleFont);

        dishStatsTable_ = new QTableWidget;
        dishStatsTable_->setColumnCount(5);
        dishStatsTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("编号") << QString::fromUtf8("名称") << QString::fromUtf8("分类")
            << QString::fromUtf8("已售") << QString::fromUtf8("库存"));
        configureTable(dishStatsTable_);

        layout->addWidget(summaryTitle);
        layout->addWidget(dataSummaryTable_, 1);
        layout->addWidget(dishTitle);
        layout->addWidget(dishStatsTable_, 2);
        return page;
    }

    QWidget* buildUsersTab() {
        // 用户页只给管理员使用；服务员登录后整个标签页会被禁用。
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        usersTable_ = new QTableWidget;
        usersTable_->setColumnCount(4);
        usersTable_->setHorizontalHeaderLabels(QStringList()
            << QString::fromUtf8("账号") << QString::fromUtf8("密码") << QString::fromUtf8("角色") << QString::fromUtf8("姓名"));
        configureTable(usersTable_);

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
        // 登录时只把输入交给后端校验；前端不直接判断账号密码是否正确。
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
        // 登录成功后记录当前用户，并根据角色刷新权限和所有表格。
        currentUserLabel_->setText(QString::fromUtf8("当前用户：%1（%2）")
            .arg(currentDisplayName_, qs(Backend::RestaurantBackend::roleName(role))));
        applyPermissions();
        refreshAll();
        stack_->setCurrentWidget(workPage_);
    }

    void applyPermissions() {
        // 管理员拥有资料维护权限；服务员只能做点餐、退菜、待结账和结账。
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
        tabs_->setTabEnabled(4, admin);
    }

    void refreshAll() {
        // 每次业务操作后统一刷新所有表格，避免某个页面还显示旧状态。
        refreshDishes();
        refreshTables();
        refreshOrders();
        refreshUsers();
        refreshDataView();
        refreshOrderItems();
    }

    void refreshDishes() {
        // 用后端菜品列表重建菜品表格；表格内容不作为真实数据源。
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
        // 餐桌金额和状态由后端维护，前端只负责格式化展示。
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
        // 订单表展示主表信息，具体菜品明细由 refreshOrderItems() 单独刷新。
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
        // 用户表包含密码字段，这是课程演示版；真实系统不应明文展示密码。
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

    void refreshDataView() {
        if (dataSummaryTable_ == 0 || dishStatsTable_ == 0) {
            return;
        }

        const std::vector<Backend::Order>& orders = backend_.orders();
        const std::vector<Backend::Table>& tables = backend_.tables();
        const std::vector<Backend::Dish>& dishes = backend_.dishes();

        int paidOrders = 0;
        int activeOrders = 0;
        int waitPayOrders = 0;
        int cancelledOrders = 0;
        double revenue = 0.0;
        for (size_t i = 0; i < orders.size(); ++i) {
            if (orders[i].status == Backend::OrderPaid) {
                ++paidOrders;
                revenue += orders[i].totalAmount;
            } else if (orders[i].status == Backend::OrderActive) {
                ++activeOrders;
            } else if (orders[i].status == Backend::OrderWaitPay) {
                ++waitPayOrders;
            } else {
                ++cancelledOrders;
            }
        }

        int occupiedTables = 0;
        for (size_t i = 0; i < tables.size(); ++i) {
            if (tables[i].status != Backend::TableIdle) {
                ++occupiedTables;
            }
        }

        int totalSold = 0;
        QString topDish = QString::fromUtf8("暂无");
        int topSold = -1;
        for (size_t i = 0; i < dishes.size(); ++i) {
            totalSold += dishes[i].sold;
            if (dishes[i].sold > topSold) {
                topSold = dishes[i].sold;
                topDish = qs(dishes[i].name);
            }
        }

        dataSummaryTable_->setRowCount(7);
        dataSummaryTable_->setItem(0, 0, item(QString::fromUtf8("累计营业额")));
        dataSummaryTable_->setItem(0, 1, item(qs(Backend::RestaurantBackend::money(revenue))));
        dataSummaryTable_->setItem(1, 0, item(QString::fromUtf8("已结账订单")));
        dataSummaryTable_->setItem(1, 1, item(QString::number(paidOrders)));
        dataSummaryTable_->setItem(2, 0, item(QString::fromUtf8("进行中订单")));
        dataSummaryTable_->setItem(2, 1, item(QString::number(activeOrders)));
        dataSummaryTable_->setItem(3, 0, item(QString::fromUtf8("待结账订单")));
        dataSummaryTable_->setItem(3, 1, item(QString::number(waitPayOrders)));
        dataSummaryTable_->setItem(4, 0, item(QString::fromUtf8("已取消订单")));
        dataSummaryTable_->setItem(4, 1, item(QString::number(cancelledOrders)));
        dataSummaryTable_->setItem(5, 0, item(QString::fromUtf8("占用餐桌")));
        dataSummaryTable_->setItem(5, 1, item(QString::number(occupiedTables) + "/" + QString::number(tables.size())));
        dataSummaryTable_->setItem(6, 0, item(QString::fromUtf8("菜品总销量 / 热销菜品")));
        dataSummaryTable_->setItem(6, 1, item(QString::number(totalSold) + " / " + topDish));

        dishStatsTable_->setRowCount(static_cast<int>(dishes.size()));
        for (int row = 0; row < static_cast<int>(dishes.size()); ++row) {
            const Backend::Dish& d = dishes[static_cast<size_t>(row)];
            dishStatsTable_->setItem(row, 0, item(QString::number(d.id)));
            dishStatsTable_->setItem(row, 1, item(qs(d.name)));
            dishStatsTable_->setItem(row, 2, item(qs(d.category)));
            dishStatsTable_->setItem(row, 3, item(QString::number(d.sold)));
            dishStatsTable_->setItem(row, 4, item(QString::number(d.stock)));
        }
    }

    void refreshOrderItems() {
        // 明细表跟随订单表当前选中行变化；没选中订单时清空明细。
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
        // 从指定表格的选中行读取整数主键；没有选择时给用户提示。
        int row = selectedRow(table);
        if (row < 0 || table->item(row, column) == 0) {
            QMessageBox::information(this, title, QString::fromUtf8("请先选择一行。"));
            return -1;
        }
        return table->item(row, column)->text().toInt();
    }

    const Backend::Order* findOrderById(int orderId) const {
        const std::vector<Backend::Order>& orders = backend_.orders();
        for (size_t i = 0; i < orders.size(); ++i) {
            if (orders[i].orderId == orderId) {
                return &orders[i];
            }
        }
        return 0;
    }

    int activeOrderIdForTable(int tableId) const {
        const std::vector<Backend::Table>& tables = backend_.tables();
        for (size_t i = 0; i < tables.size(); ++i) {
            if (tables[i].id == tableId) {
                return tables[i].activeOrderId;
            }
        }
        return 0;
    }

    void selectOrderById(int orderId) {
        for (int row = 0; row < ordersTable_->rowCount(); ++row) {
            if (ordersTable_->item(row, 0) != 0 && ordersTable_->item(row, 0)->text().toInt() == orderId) {
                ordersTable_->selectRow(row);
                refreshOrderItems();
                return;
            }
        }
    }

    int selectedOrderTableId(const QString& title) {
        int orderId = selectedOrderId();
        if (orderId < 0) {
            return -1;
        }
        const Backend::Order* order = findOrderById(orderId);
        if (order == 0) {
            QMessageBox::warning(this, title, QString::fromUtf8("没有找到当前订单。"));
            return -1;
        }
        return order->tableId;
    }

    void showBackendError(const std::string& error) {
        // 后端已经生成可读错误信息，前端只负责弹窗展示。
        QMessageBox::warning(this, QString::fromUtf8("操作失败"), qs(error));
    }

    void editDish(bool existing) {
        // existing=false 表示新增；existing=true 表示根据当前选中行修改。
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

        // 用一个临时 QDialog 收集菜品字段，点击确定后再交给后端保存。
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
        // 前端不直接改 dishes_，只调用后端 add/update，保证规则和保存逻辑统一。
        bool okResult = existing ? backend_.updateDish(dish, error) : backend_.addDish(dish, error);
        if (!okResult) showBackendError(error);
        refreshAll();
    }

    void deleteDish() {
        // 删除前只取菜品编号，是否允许删除由后端检查未结账订单引用。
        int id = selectedInt(dishesTable_, 0, QString::fromUtf8("删除菜品"));
        if (id < 0) return;
        std::string error;
        if (!backend_.deleteDish(id, error)) showBackendError(error);
        refreshAll();
    }

    void editTable(bool existing) {
        // 餐桌编辑目前只处理桌号和容量，状态字段由开台/结账流程维护。
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

        // 桌号新增后不可在修改流程里编辑，避免破坏订单和餐桌的关联。
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
        // 后端会拒绝删除正在使用或待结账的餐桌。
        int id = selectedInt(tablesTable_, 0, QString::fromUtf8("删除餐桌"));
        if (id < 0) return;
        std::string error;
        if (!backend_.deleteTable(id, error)) showBackendError(error);
        refreshAll();
    }

    void editUser(bool existing) {
        // 用户编辑只对管理员开放；服务员登录时用户标签页会被禁用。
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

        // 角色使用下拉框，避免手写字符串导致 role 值不合法。
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
        // 删除用户需要当前选中行；最后一个管理员的保护由后端完成。
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
        // 开台需要先选中餐桌；后端会创建订单并把餐桌状态改为使用中。
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("开台/进入订单"));
        if (tableId < 0) return;

        int activeOrderId = activeOrderIdForTable(tableId);
        if (activeOrderId > 0) {
            tabs_->setCurrentIndex(2);
            selectOrderById(activeOrderId);
            return;
        }

        int orderId = 0;
        std::string error;
        if (!backend_.openTable(tableId, ss(currentUsername_), orderId, error)) {
            showBackendError(error);
            return;
        }
        refreshAll();
        tabs_->setCurrentIndex(2);
        selectOrderById(orderId);
    }

    int selectedOrderId() {
        // 所有订单操作都基于订单表当前选中的订单号。
        return selectedInt(ordersTable_, 0, QString::fromUtf8("订单操作"));
    }

    void addDishToSelectedOrder() {
        // 加菜需要订单号、菜品编号和数量；库存不足等规则由后端判断。
        int orderId = selectedOrderId();
        if (orderId < 0) return;

        const std::vector<Backend::Dish>& dishes = backend_.dishes();
        if (dishes.empty()) {
            QMessageBox::information(this, QString::fromUtf8("加菜"), QString::fromUtf8("当前没有可选菜品。"));
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle(QString::fromUtf8("加菜"));
        QFormLayout form(&dialog);
        QComboBox dishCombo;
        for (size_t i = 0; i < dishes.size(); ++i) {
            if (dishes[i].stock <= 0) {
                continue;
            }
            QString text = QString::fromUtf8("%1  库存:%2  单价:%3")
                .arg(qs(dishes[i].name))
                .arg(dishes[i].stock)
                .arg(qs(Backend::RestaurantBackend::money(dishes[i].price)));
            dishCombo.addItem(text, dishes[i].id);
        }
        if (dishCombo.count() == 0) {
            QMessageBox::information(this, QString::fromUtf8("加菜"), QString::fromUtf8("当前没有库存充足的菜品。"));
            return;
        }
        QSpinBox quantity;
        quantity.setRange(1, 999999);
        QPushButton ok(QString::fromUtf8("加入订单"));
        form.addRow(QString::fromUtf8("菜品"), &dishCombo);
        form.addRow(QString::fromUtf8("数量"), &quantity);
        form.addRow(&ok);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        int dishId = dishCombo.currentData().toInt();
        std::string error;
        if (!backend_.addDishToOrder(orderId, dishId, quantity.value(), error)) showBackendError(error);
        refreshAll();
        selectOrderById(orderId);
    }

    void returnDishFromOrder() {
        // 退菜会恢复库存并重新计算订单金额。
        int orderId = selectedOrderId();
        if (orderId < 0) return;
        int itemRow = selectedRow(orderItemsTable_);
        if (itemRow < 0 || orderItemsTable_->item(itemRow, 0) == 0 || orderItemsTable_->item(itemRow, 2) == 0) {
            QMessageBox::information(this, QString::fromUtf8("退菜"), QString::fromUtf8("请先选择要退的订单明细。"));
            return;
        }
        int dishId = orderItemsTable_->item(itemRow, 0)->text().toInt();
        int maxQuantity = orderItemsTable_->item(itemRow, 2)->text().toInt();
        int quantity = QInputDialog::getInt(this, QString::fromUtf8("退菜"), QString::fromUtf8("退菜数量"), 1, 1, maxQuantity);
        std::string error;
        if (!backend_.returnDish(orderId, dishId, quantity, error)) showBackendError(error);
        refreshAll();
        selectOrderById(orderId);
    }

    void markSelectedOrderWaitPay() {
        int tableId = selectedOrderTableId(QString::fromUtf8("待结账"));
        if (tableId < 0) return;
        int orderId = selectedOrderId();
        std::string error;
        if (!backend_.markWaitPay(tableId, error)) showBackendError(error);
        refreshAll();
        selectOrderById(orderId);
    }

    void checkoutSelectedOrder() {
        int tableId = selectedOrderTableId(QString::fromUtf8("结账"));
        if (tableId < 0) return;
        std::string error;
        if (!backend_.checkout(tableId, error)) showBackendError(error);
        refreshAll();
    }

    void markSelectedTableWaitPay() {
        // 待结账按餐桌操作，因为收银场景通常先看桌台。
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("待结账"));
        if (tableId < 0) return;
        std::string error;
        if (!backend_.markWaitPay(tableId, error)) showBackendError(error);
        refreshAll();
    }

    void checkoutSelectedTable() {
        // 结账成功后后端会释放餐桌，订单保留为历史记录。
        int tableId = selectedInt(tablesTable_, 0, QString::fromUtf8("结账"));
        if (tableId < 0) return;
        std::string error;
        if (!backend_.checkout(tableId, error)) showBackendError(error);
        refreshAll();
    }

    void cancelSelectedOrder() {
        // 取消订单会退回已点菜品库存，并在必要时释放关联餐桌。
        int orderId = selectedOrderId();
        if (orderId < 0) return;
        std::string error;
        if (!backend_.cancelOrder(orderId, error)) showBackendError(error);
        refreshAll();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    // exe 位于 dist/，数据文件放在项目根目录；启动时把后端数据目录切到 dist 的上一级。
    QDir dataDir(QApplication::applicationDirPath());
    dataDir.cdUp();
    Backend::RestaurantBackend::setDataDirectory(dataDir.absolutePath().toStdString());
    MainWindow window;
    window.show();
    return app.exec();
}
