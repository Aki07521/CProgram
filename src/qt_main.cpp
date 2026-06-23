#include "restaurant_backend.hpp"

#include <QtCore/QDate>
#include <QtCore/QFile>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
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
#include <QtWidgets/QListView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include <initializer_list>

// Qt 前端界面和交互逻辑
namespace {
    QString q(const char* value) {
        return QString::fromUtf8(value);
    }

    QString dataDirectory(const QString& appDir) {
        const QString path = qEnvironmentVariable("RESTAURANT_DATA_DIR");
        return path.isEmpty() ? appDir + "/data" : path;
    }

    // 从数据目录读取界面样式表
    QString appStyleSheet(const QString& dataDir) {
        QFile file(dataDir + "/style.qss");
        return file.open(QFile::ReadOnly | QFile::Text) ? QString::fromUtf8(file.readAll()) : QString();
    }

    QString qs(const std::string& value) {
        return QString::fromUtf8(value.c_str());
    }

    std::string ss(const QString& value) {
        return value.toUtf8().constData();
    }

    // 表格单元格统一设置为不可编辑和居中
    QTableWidgetItem* item(const QString& text) {
        QTableWidgetItem* cell = new QTableWidgetItem(text);
        cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
        cell->setTextAlignment(Qt::AlignCenter);
        return cell;
    }

    // 设置表格通用行为
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
    }

    QStringList labels(std::initializer_list<const char*> names) {
        QStringList result;
        for (std::initializer_list<const char*>::const_iterator it = names.begin(); it != names.end(); ++it) {
            result << q(*it);
        }
        return result;
    }

    void setupTable(QTableWidget* table, std::initializer_list<const char*> names) {
        table->setColumnCount(static_cast<int>(names.size()));
        table->setHorizontalHeaderLabels(labels(names));
        configureTable(table);
    }

    void setRow(QTableWidget* table, int row, std::initializer_list<QString> values) {
        int column = 0;
        for (std::initializer_list<QString>::const_iterator it = values.begin(); it != values.end(); ++it) {
            table->setItem(row, column++, item(*it));
        }
    }

    QPushButton* button(const char* text) {
        return new QPushButton(q(text));
    }

    void configureCombo(QComboBox* combo, int visibleItems) {
        combo->setEditable(false);
        combo->setInsertPolicy(QComboBox::NoInsert);
        combo->setMaxVisibleItems(visibleItems);
        combo->setView(new QListView(combo));
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
        resize(1120, 720);
        setMinimumSize(960, 640);
        setWindowTitle(QString::fromUtf8("中小饭店点餐管理系统"));
    }

private:
    Backend::RestaurantBackend backend_;

    QStackedWidget* stack_ = 0;
    QWidget* loginPage_ = 0;
    QWidget* workPage_ = 0;

    QButtonGroup* loginRoleGroup_ = 0;
    QLineEdit* usernameEdit_ = 0;
    QLineEdit* passwordEdit_ = 0;

    QLabel* currentUserLabel_ = 0;
    QTabWidget* tabs_ = 0;

    QTableWidget* dishesTable_ = 0;
    QTableWidget* tablesTable_ = 0;
    QTableWidget* ordersTable_ = 0;
    QTableWidget* orderItemsTable_ = 0;
    QTableWidget* usersTable_ = 0;
    QTableWidget* dataSummaryTable_ = 0;
    QTableWidget* dishStatsTable_ = 0;
    QComboBox* dayCombo_ = 0;
    QComboBox* monthCombo_ = 0;
    QPushButton* clearHistoryButton_ = 0;

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

    // 界面接口 构建登录页
    void buildLoginPage() {
        loginPage_ = new QWidget;
        QVBoxLayout* outer = new QVBoxLayout(loginPage_);
        outer->setContentsMargins(24, 24, 24, 24);
        outer->setAlignment(Qt::AlignCenter);

        QGroupBox* box = new QGroupBox(QString::fromUtf8("系统登录"));
        box->setFixedWidth(520);
        box->setMaximumHeight(420);
        QVBoxLayout* boxLayout = new QVBoxLayout(box);
        boxLayout->setContentsMargins(28, 30, 28, 28);
        boxLayout->setSpacing(18);
        QLabel* title = new QLabel(QString::fromUtf8("中小饭店点餐管理系统"));
        QFont titleFont = title->font();
        titleFont.setPointSize(22);
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setAlignment(Qt::AlignCenter);

        QWidget* rolePanel = new QWidget;
        QHBoxLayout* roleLayout = new QHBoxLayout(rolePanel);
        roleLayout->setContentsMargins(0, 0, 0, 0);
        roleLayout->setSpacing(10);
        QRadioButton* adminRole = new QRadioButton(QString::fromUtf8("管理员"));
        QRadioButton* waiterRole = new QRadioButton(QString::fromUtf8("服务员"));
        loginRoleGroup_ = new QButtonGroup(this);
        loginRoleGroup_->addButton(adminRole, 0);
        loginRoleGroup_->addButton(waiterRole, 1);
        adminRole->setChecked(true);
        roleLayout->addWidget(adminRole);
        roleLayout->addWidget(waiterRole);
        usernameEdit_ = new QLineEdit("admin");
        passwordEdit_ = new QLineEdit("123456");
        passwordEdit_->setEchoMode(QLineEdit::Password);

        QFormLayout* form = new QFormLayout;
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        form->setFormAlignment(Qt::AlignHCenter);
        form->setHorizontalSpacing(12);
        form->setVerticalSpacing(12);
        form->addRow(QString::fromUtf8("角色"), rolePanel);
        form->addRow(QString::fromUtf8("账号"), usernameEdit_);
        form->addRow(QString::fromUtf8("密码"), passwordEdit_);

        QPushButton* loginButton = new QPushButton(QString::fromUtf8("登录"));
        loginButton->setMinimumHeight(42);
        connect(loginButton, &QPushButton::clicked, this, [this]() { login(); });
        connect(passwordEdit_, &QLineEdit::returnPressed, this, [this]() { login(); });

        boxLayout->addWidget(title);
        boxLayout->addSpacing(12);
        boxLayout->addLayout(form);
        boxLayout->addSpacing(8);
        boxLayout->addWidget(loginButton);
        outer->addWidget(box, 0, Qt::AlignCenter);
        stack_->addWidget(loginPage_);
    }

    // 界面接口 构建工作页和角色菜单
    void buildWorkPage() {
        workPage_ = new QWidget;
        QVBoxLayout* root = new QVBoxLayout(workPage_);
        root->setContentsMargins(16, 14, 16, 16);
        root->setSpacing(12);

        QHBoxLayout* top = new QHBoxLayout;
        top->setSpacing(12);
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
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(12);
        dishesTable_ = new QTableWidget;
        setupTable(dishesTable_, {"编号", "名称", "分类", "价格", "库存", "已售"});

        QHBoxLayout* buttons = new QHBoxLayout;
        buttons->setSpacing(10);
        addDishButton_ = button("新增菜品");
        editDishButton_ = button("修改菜品");
        deleteDishButton_ = button("删除菜品");
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
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(12);
        tablesTable_ = new QTableWidget;
        setupTable(tablesTable_, {"桌号", "容量", "状态", "当前金额", "关联订单"});

        QHBoxLayout* buttons = new QHBoxLayout;
        buttons->setSpacing(10);
        addTableButton_ = button("新增餐桌");
        editTableButton_ = button("修改容量");
        deleteTableButton_ = button("删除餐桌");
        QPushButton* openTable = button("开台/进入订单");
        QPushButton* checkout = button("本桌结账");
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
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(12);

        ordersTable_ = new QTableWidget;
        setupTable(ordersTable_, {"订单号", "桌号", "服务员", "状态", "金额", "创建时间"});

        orderItemsTable_ = new QTableWidget;
        setupTable(orderItemsTable_, {"菜品编号", "菜品名称", "数量", "单价", "小计"});

        connect(ordersTable_, &QTableWidget::itemSelectionChanged, this, [this]() { refreshOrderItems(); });

        QHBoxLayout* buttons = new QHBoxLayout;
        buttons->setSpacing(10);
        QPushButton* addDish = button("加菜");
        QPushButton* returnDish = button("退菜");
        QPushButton* waitPay = button("设为待结账");
        QPushButton* checkout = button("结账");
        QPushButton* cancelOrder = button("取消订单");
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
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(12);

        QLabel* summaryTitle = new QLabel(QString::fromUtf8("经营数据总览"));
        QFont titleFont = summaryTitle->font();
        titleFont.setBold(true);
        titleFont.setPointSize(12);
        summaryTitle->setFont(titleFont);

        dataSummaryTable_ = new QTableWidget;
        setupTable(dataSummaryTable_, {"指标", "数值", "指标", "数值"});
        dataSummaryTable_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        dataSummaryTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        dataSummaryTable_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        dataSummaryTable_->setFixedHeight(222);
        dataSummaryTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        dataSummaryTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        dataSummaryTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        dataSummaryTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

        QHBoxLayout* dataTools = new QHBoxLayout;
        dataTools->setSpacing(10);
        dayCombo_ = new QComboBox;
        dayCombo_->setMinimumWidth(150);
        configureCombo(dayCombo_, 14);
        monthCombo_ = new QComboBox;
        monthCombo_->setMinimumWidth(120);
        configureCombo(monthCombo_, 12);
        clearHistoryButton_ = button("清除历史数据");
        clearHistoryButton_->setMinimumWidth(138);
        connect(dayCombo_, &QComboBox::currentTextChanged, this, [this](const QString&) { refreshDataView(); });
        connect(monthCombo_, &QComboBox::currentTextChanged, this, [this](const QString&) { refreshDataView(); });
        connect(clearHistoryButton_, &QPushButton::clicked, this, [this]() { clearHistoricalData(); });
        dataTools->addWidget(new QLabel(QString::fromUtf8("日期")));
        dataTools->addWidget(dayCombo_);
        dataTools->addWidget(new QLabel(QString::fromUtf8("月份")));
        dataTools->addWidget(monthCombo_);
        dataTools->addWidget(clearHistoryButton_);
        dataTools->addStretch();

        QLabel* dishTitle = new QLabel(QString::fromUtf8("菜品销量与库存"));
        dishTitle->setFont(titleFont);

        dishStatsTable_ = new QTableWidget;
        setupTable(dishStatsTable_, {"编号", "名称", "分类", "已售", "库存"});

        layout->addWidget(summaryTitle);
        layout->addLayout(dataTools);
        layout->addWidget(dataSummaryTable_);
        layout->addWidget(dishTitle);
        layout->addWidget(dishStatsTable_, 2);
        return page;
    }

    QWidget* buildUsersTab() {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(12);
        usersTable_ = new QTableWidget;
        setupTable(usersTable_, {"账号", "密码", "角色", "姓名"});

        QHBoxLayout* buttons = new QHBoxLayout;
        buttons->setSpacing(10);
        addUserButton_ = button("新增用户");
        editUserButton_ = button("修改用户");
        deleteUserButton_ = button("删除用户");
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

    // 操作接口 登录系统并进入对应角色界面
    void login() {
        std::string display;
        std::string role = loginRoleGroup_->checkedId() == 1 ? "waiter" : "admin";
        bool ok = backend_.login(role, ss(usernameEdit_->text()), ss(passwordEdit_->text()), &display);
        if (!ok) {
            QMessageBox::warning(this, QString::fromUtf8("登录失败"), QString::fromUtf8("账号、密码或角色不匹配。"));
            return;
        }

        currentRole_ = qs(role);
        currentUsername_ = usernameEdit_->text();
        currentDisplayName_ = qs(display);
        currentUserLabel_->setText(QString::fromUtf8("当前用户：%1（%2）")
            .arg(currentDisplayName_, qs(Backend::RestaurantBackend::roleName(role))));
        applyPermissions();
        refreshAll();
        stack_->setCurrentWidget(workPage_);
    }

    // 根据管理员和服务员身份显示不同菜单
    void applyPermissions() {
        bool admin = currentRole_ == "admin";
        addUserButton_->setVisible(admin);
        editUserButton_->setVisible(admin);
        deleteUserButton_->setVisible(admin);
        addDishButton_->setVisible(admin);
        editDishButton_->setVisible(admin);
        deleteDishButton_->setVisible(admin);
        addTableButton_->setVisible(admin);
        editTableButton_->setVisible(admin);
        deleteTableButton_->setVisible(admin);
        clearHistoryButton_->setVisible(admin);

        tabs_->setTabVisible(3, admin);
        tabs_->setTabVisible(4, admin);
        tabs_->setCurrentIndex(admin ? 0 : 1);
    }

    void refreshAll() {
        refreshDishes();
        refreshTables();
        refreshOrders();
        refreshUsers();
        refreshDataView();
        refreshOrderItems();
    }

    void refreshDishes() {
        const Backend::LinkedList<Backend::Dish>& dishes = backend_.dishes();
        dishesTable_->setRowCount(static_cast<int>(dishes.size()));
        int row = 0;
        for (const Backend::Dish& d : dishes) {
            setRow(dishesTable_, row, {QString::number(d.id), qs(d.name), qs(d.category),
                qs(Backend::RestaurantBackend::money(d.price)), QString::number(d.stock), QString::number(d.sold)});
            ++row;
        }
    }

    void refreshTables() {
        const Backend::LinkedList<Backend::Table>& tables = backend_.tables();
        tablesTable_->setRowCount(static_cast<int>(tables.size()));
        int row = 0;
        for (const Backend::Table& t : tables) {
            setRow(tablesTable_, row, {QString::number(t.id), QString::number(t.capacity),
                qs(Backend::RestaurantBackend::tableStatusName(t.status)),
                qs(Backend::RestaurantBackend::money(t.currentAmount)), QString::number(t.activeOrderId)});
            ++row;
        }
    }

    void refreshOrders() {
        const Backend::LinkedList<Backend::Order>& orders = backend_.orders();
        ordersTable_->setRowCount(static_cast<int>(orders.size()));
        int row = 0;
        for (const Backend::Order& o : orders) {
            setRow(ordersTable_, row, {QString::number(o.orderId), QString::number(o.tableId), qs(o.waiter),
                qs(Backend::RestaurantBackend::orderStatusName(o.status)),
                qs(Backend::RestaurantBackend::money(o.totalAmount)), qs(o.createdAt)});
            ++row;
        }
    }

    void refreshUsers() {
        const Backend::LinkedList<Backend::User>& users = backend_.users();
        usersTable_->setRowCount(static_cast<int>(users.size()));
        int row = 0;
        for (const Backend::User& u : users) {
            setRow(usersTable_, row, {qs(u.username), qs(u.password),
                qs(Backend::RestaurantBackend::roleName(u.role)), qs(u.name)});
            ++row;
        }
    }

    void refreshDateChoices(const Backend::LinkedList<Backend::Order>& orders) {
        if (dayCombo_ == 0 || monthCombo_ == 0) {
            return;
        }
        const QString today = QDate::currentDate().toString("yyyy-MM-dd");
        const QString thisMonth = QDate::currentDate().toString("yyyy-MM");
        const QString oldDay = dayCombo_->currentText().isEmpty() ? today : dayCombo_->currentText();
        const QString oldMonth = monthCombo_->currentText().isEmpty() ? thisMonth : monthCombo_->currentText();
        QStringList days;
        QStringList months;
        QDate todayDate = QDate::currentDate();
        for (int i = 0; i < 31; ++i) {
            QString day = todayDate.addDays(-i).toString("yyyy-MM-dd");
            if (!days.contains(day)) {
                days << day;
            }
        }
        for (int i = 0; i < 12; ++i) {
            QString month = todayDate.addMonths(-i).toString("yyyy-MM");
            if (!months.contains(month)) {
                months << month;
            }
        }
        for (const Backend::Order& order : orders) {
            if (order.status != Backend::OrderPaid) {
                continue;
            }
            QString paidTime = order.paidAt.empty() ? qs(order.createdAt) : qs(order.paidAt);
            QString day = paidTime.left(10);
            QString month = paidTime.left(7);
            if (day.size() == 10 && !days.contains(day)) {
                days << day;
            }
            if (month.size() == 7 && !months.contains(month)) {
                months << month;
            }
        }
        days.sort();
        months.sort();
        for (int i = 0, j = days.size() - 1; i < j; ++i, --j) {
            days.swapItemsAt(i, j);
        }
        for (int i = 0, j = months.size() - 1; i < j; ++i, --j) {
            months.swapItemsAt(i, j);
        }
        QSignalBlocker dayBlocker(dayCombo_);
        QSignalBlocker monthBlocker(monthCombo_);
        dayCombo_->clear();
        monthCombo_->clear();
        dayCombo_->addItems(days);
        monthCombo_->addItems(months);
        int dayIndex = dayCombo_->findText(oldDay);
        int monthIndex = monthCombo_->findText(oldMonth);
        dayCombo_->setCurrentIndex(dayIndex < 0 ? dayCombo_->findText(today) : dayIndex);
        monthCombo_->setCurrentIndex(monthIndex < 0 ? monthCombo_->findText(thisMonth) : monthIndex);
    }

    // 刷新经营数据页时顺便统计营业额和销量
    void refreshDataView() {
        if (dataSummaryTable_ == 0 || dishStatsTable_ == 0) {
            return;
        }

        const Backend::LinkedList<Backend::Order>& orders = backend_.orders();
        const Backend::LinkedList<Backend::Table>& tables = backend_.tables();
        const Backend::LinkedList<Backend::Dish>& dishes = backend_.dishes();
        refreshDateChoices(orders);

        int paidOrders = 0;
        int activeOrders = 0;
        int waitPayOrders = 0;
        int cancelledOrders = 0;
        const QString dayText = dayCombo_ == 0 || dayCombo_->currentText().isEmpty() ? QDate::currentDate().toString("yyyy-MM-dd") : dayCombo_->currentText();
        const QString monthText = monthCombo_ == 0 || monthCombo_->currentText().isEmpty() ? QDate::currentDate().toString("yyyy-MM") : monthCombo_->currentText();
        double revenue = 0.0;
        double dayRevenue = 0.0;
        double monthRevenue = 0.0;
        int dayPaidOrders = 0;
        int monthPaidOrders = 0;
        for (const Backend::Order& order : orders) {
            if (order.status == Backend::OrderPaid) {
                ++paidOrders;
                revenue += order.totalAmount;
                QString paidTime = order.paidAt.empty() ? qs(order.createdAt) : qs(order.paidAt);
                if (paidTime.left(10) == dayText) {
                    dayRevenue += order.totalAmount;
                    ++dayPaidOrders;
                }
                if (paidTime.left(7) == monthText) {
                    monthRevenue += order.totalAmount;
                    ++monthPaidOrders;
                }
            } else if (order.status == Backend::OrderActive) {
                ++activeOrders;
            } else if (order.status == Backend::OrderWaitPay) {
                ++waitPayOrders;
            } else {
                ++cancelledOrders;
            }
        }

        int occupiedTables = 0;
        for (const Backend::Table& table : tables) {
            if (table.status != Backend::TableIdle) {
                ++occupiedTables;
            }
        }

        int totalSold = 0;
        QString topDish = QString::fromUtf8("暂无");
        int topSold = 0;
        for (const Backend::Dish& dish : dishes) {
            totalSold += dish.sold;
            if (dish.sold > topSold) {
                topSold = dish.sold;
                topDish = qs(dish.name);
            }
        }

        dataSummaryTable_->setRowCount(5);
        setRow(dataSummaryTable_, 0, {q("累计营业额"), qs(Backend::RestaurantBackend::money(revenue)), q("已结账订单"), QString::number(paidOrders)});
        setRow(dataSummaryTable_, 1, {QString::fromUtf8("单日流水(%1)").arg(dayText), qs(Backend::RestaurantBackend::money(dayRevenue)), q("单日订单"), QString::number(dayPaidOrders)});
        setRow(dataSummaryTable_, 2, {QString::fromUtf8("月流水(%1)").arg(monthText), qs(Backend::RestaurantBackend::money(monthRevenue)), q("月订单"), QString::number(monthPaidOrders)});
        setRow(dataSummaryTable_, 3, {q("进行中 / 待结账"), QString::number(activeOrders) + " / " + QString::number(waitPayOrders), q("已取消订单"), QString::number(cancelledOrders)});
        setRow(dataSummaryTable_, 4, {q("占用餐桌"), QString::number(occupiedTables) + "/" + QString::number(tables.size()), q("总销量 / 热销菜"), QString::number(totalSold) + " / " + topDish});

        dishStatsTable_->setRowCount(static_cast<int>(dishes.size()));
        int row = 0;
        for (const Backend::Dish& d : dishes) {
            setRow(dishStatsTable_, row, {QString::number(d.id), qs(d.name), qs(d.category),
                QString::number(d.sold), QString::number(d.stock)});
            ++row;
        }
    }

    void refreshOrderItems() {
        int row = selectedRow(ordersTable_);
        if (row < 0 || ordersTable_->item(row, 0) == 0) {
            orderItemsTable_->setRowCount(0);
            return;
        }
        int orderId = ordersTable_->item(row, 0)->text().toInt();
        const Backend::LinkedList<Backend::Order>& orders = backend_.orders();
        for (const Backend::Order& order : orders) {
            if (order.orderId == orderId) {
                orderItemsTable_->setRowCount(static_cast<int>(order.items.size()));
                int r = 0;
                for (const Backend::OrderItem& it : order.items) {
                    setRow(orderItemsTable_, r, {QString::number(it.dishId), qs(it.dishName),
                        QString::number(it.quantity), qs(Backend::RestaurantBackend::money(it.price)),
                        qs(Backend::RestaurantBackend::money(it.subtotal))});
                    ++r;
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

    const Backend::Order* findOrderById(int orderId) const {
        const Backend::LinkedList<Backend::Order>& orders = backend_.orders();
        for (const Backend::Order& order : orders) {
            if (order.orderId == orderId) {
                return &order;
            }
        }
        return 0;
    }

    int activeOrderIdForTable(int tableId) const {
        const Backend::LinkedList<Backend::Table>& tables = backend_.tables();
        for (const Backend::Table& table : tables) {
            if (table.id == tableId) {
                return table.activeOrderId;
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
        QMessageBox::warning(this, QString::fromUtf8("操作失败"), qs(error));
    }

    // 操作接口 管理员清除历史经营数据
    void clearHistoricalData() {
        if (currentRole_ != "admin") {
            QMessageBox::warning(this, QString::fromUtf8("权限不足"), QString::fromUtf8("只有管理员可以清除历史数据。"));
            return;
        }

        const QString message = QString::fromUtf8(
            "确认清除所有历史订单、订单明细和菜品销量统计吗？\n"
            "该操作会保留用户、菜品、库存和餐桌基础资料。\n"
            "如果仍有进行中或待结账订单，系统会自动拒绝清除。");
        QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            QString::fromUtf8("清除历史数据"),
            message,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice != QMessageBox::Yes) {
            return;
        }

        std::string error;
        if (!backend_.clearHistoricalData(error)) {
            showBackendError(error);
            return;
        }

        refreshAll();
        QMessageBox::information(this, QString::fromUtf8("清除完成"), QString::fromUtf8("历史订单和统计数据已清除。"));
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
        QWidget rolePanel;
        QHBoxLayout roleLayout(&rolePanel);
        roleLayout.setContentsMargins(0, 0, 0, 0);
        roleLayout.setSpacing(10);
        QRadioButton adminRole(QString::fromUtf8("管理员"));
        QRadioButton waiterRole(QString::fromUtf8("服务员"));
        QButtonGroup roleGroup;
        roleGroup.addButton(&adminRole, 0);
        roleGroup.addButton(&waiterRole, 1);
        if (user.role == "waiter") {
            waiterRole.setChecked(true);
        } else {
            adminRole.setChecked(true);
        }
        roleLayout.addWidget(&adminRole);
        roleLayout.addWidget(&waiterRole);
        QLineEdit name(qs(user.name));
        QPushButton ok(QString::fromUtf8("确定"));
        form.addRow(QString::fromUtf8("账号"), &username);
        form.addRow(QString::fromUtf8("密码"), &password);
        form.addRow(QString::fromUtf8("角色"), &rolePanel);
        form.addRow(QString::fromUtf8("姓名"), &name);
        form.addRow(&ok);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() != QDialog::Accepted) return;

        user.username = ss(username.text());
        user.password = ss(password.text());
        user.role = roleGroup.checkedId() == 1 ? "waiter" : "admin";
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

    // 操作接口 餐桌空闲时开台否则进入已有订单
    void openSelectedTable() {
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
        return selectedInt(ordersTable_, 0, QString::fromUtf8("订单操作"));
    }

    // 操作接口 弹出菜品选择表并加入当前订单
    void addDishToSelectedOrder() {
        int orderId = selectedOrderId();
        if (orderId < 0) return;

        const Backend::LinkedList<Backend::Dish>& dishes = backend_.dishes();
        if (dishes.empty()) {
            QMessageBox::information(this, QString::fromUtf8("加菜"), QString::fromUtf8("当前没有可选菜品。"));
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle(QString::fromUtf8("加菜"));
        QVBoxLayout layout(&dialog);
        layout.setContentsMargins(14, 14, 14, 14);
        layout.setSpacing(12);

        QTableWidget dishPicker;
        setupTable(&dishPicker, {"编号", "菜品", "库存", "单价"});
        dishPicker.setMinimumSize(560, 360);

        int row = 0;
        for (const Backend::Dish& dish : dishes) {
            if (dish.stock <= 0) {
                continue;
            }
            dishPicker.insertRow(row);
            setRow(&dishPicker, row, {QString::number(dish.id), qs(dish.name),
                QString::number(dish.stock), qs(Backend::RestaurantBackend::money(dish.price))});
            ++row;
        }
        if (dishPicker.rowCount() == 0) {
            QMessageBox::information(this, QString::fromUtf8("加菜"), QString::fromUtf8("当前没有库存充足的菜品。"));
            return;
        }
        dishPicker.selectRow(0);

        QSpinBox quantity;
        quantity.setRange(1, 999999);
        QPushButton ok(QString::fromUtf8("加入订单"));
        QHBoxLayout quantityRow;
        quantityRow.setSpacing(10);
        quantityRow.addWidget(new QLabel(QString::fromUtf8("数量")));
        quantityRow.addWidget(&quantity);
        quantityRow.addStretch();
        quantityRow.addWidget(&ok);
        layout.addWidget(new QLabel(QString::fromUtf8("请选择菜品")));
        layout.addWidget(&dishPicker);
        layout.addLayout(&quantityRow);
        connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        connect(&dishPicker, &QTableWidget::cellDoubleClicked, &dialog, [&dialog](int, int) { dialog.accept(); });
        if (dialog.exec() != QDialog::Accepted) return;

        int selectedDishRow = selectedRow(&dishPicker);
        if (selectedDishRow < 0 || dishPicker.item(selectedDishRow, 0) == 0) {
            QMessageBox::information(this, QString::fromUtf8("加菜"), QString::fromUtf8("请先选择菜品。"));
            return;
        }
        int dishId = dishPicker.item(selectedDishRow, 0)->text().toInt();
        std::string error;
        if (!backend_.addDishToOrder(orderId, dishId, quantity.value(), error)) showBackendError(error);
        refreshAll();
        selectOrderById(orderId);
    }

    void returnDishFromOrder() {
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
    const QString appDir = QApplication::applicationDirPath();
    const QString dataDir = dataDirectory(appDir);
    QApplication::setStyle("Fusion");
    app.setStyleSheet(appStyleSheet(dataDir));
    Backend::RestaurantBackend::setDataDirectory(dataDir.toStdString());
    MainWindow window;
    window.show();
    return app.exec();
}
