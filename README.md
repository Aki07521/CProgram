# 中小饭店点餐管理系统

## 一、项目说明

本项目是一个基于 Qt Widgets 的中小饭店点餐管理系统，主要用于完成餐桌开台、点餐加菜、退菜、待结账、结账、订单查询、菜品管理、用户管理和经营数据统计等功能。

提交版已经包含可直接运行的程序和源码。普通运行时只需要双击根目录下的 `RestaurantOrderingSystem.exe`，不要求电脑提前安装 Qt、MinGW 或其他开发环境。

默认账号如下。

| 身份 | 账号 | 密码 | 说明 |
| --- | --- | --- | --- |
| 管理员 | admin | 123456 | 可以管理菜品、餐桌、用户和查看经营数据 |
| 服务员 | waiter | 123456 | 可以开台、点餐、退菜、结账和查看订单 |

## 二、文件目录结构

```text
RestaurantOrderingSystem_Submit/
  RestaurantOrderingSystem.exe     程序入口，双击运行
  README.md                        项目说明文档
  src/                             源码目录
    qt_main.cpp                    Qt 图形界面和界面事件处理
    restaurant_backend.hpp         后端数据结构和业务接口声明
    restaurant_backend.cpp         后端业务逻辑和文件读写实现
  data/                            数据和界面样式目录
    users.txt                      用户数据
    dishes.txt                     菜品数据
    tables.txt                     餐桌数据
    orders.txt                     订单主表数据
    order_items.txt                订单明细数据
    style.qss                      Qt 界面样式表
  _runtime/                        Qt 运行依赖目录，普通使用时不需要打开
```

## 三、系统总体框架

系统分为前端界面、后端业务和文件数据三部分。

| 层次 | 对应文件 | 主要职责 |
| --- | --- | --- |
| 前端界面层 | `src/qt_main.cpp` | 创建登录页和主操作界面，响应按钮点击、表格选择、下拉选择等用户操作 |
| 后端业务层 | `src/restaurant_backend.hpp` 和 `src/restaurant_backend.cpp` | 维护用户、菜品、餐桌、订单链表，完成开台、加菜、退菜、结账等业务规则 |
| 数据存储层 | `data/*.txt` | 使用文本文件保存系统数据，程序启动时读取，业务变更后写回 |
| 样式配置 | `data/style.qss` | 设置黑白灰主题、按钮、表格、输入框和下拉框样式 |
| 运行依赖 | `_runtime/` | 保存 Qt 动态库和插件，保证未知环境电脑可以运行 |

数据流向如下。

```text
用户操作界面
  调用 MainWindow 中的槽函数
    调用 RestaurantBackend 业务接口
      修改链表中的用户、菜品、餐桌、订单数据
        save() 写回 data 目录下的 txt 文件
          refreshAll() 刷新界面表格和统计数据
```

## 四、主要数据结构

项目使用 C 风格单向链表保存运行中的数据。

| 数据结构 | 所在文件 | 作用 |
| --- | --- | --- |
| `ListNode<T>` | `restaurant_backend.hpp` | 链表节点，包含 `data` 数据域和 `next` 指针 |
| `LinkedList<T>` | `restaurant_backend.hpp` | 单向链表封装，提供遍历、尾插、删除和清空功能 |
| `User` | `restaurant_backend.hpp` | 保存用户名、密码、角色和姓名 |
| `Dish` | `restaurant_backend.hpp` | 保存菜品编号、名称、分类、价格、库存和销量 |
| `Table` | `restaurant_backend.hpp` | 保存餐桌编号、容量、状态、当前金额和关联订单号 |
| `OrderItem` | `restaurant_backend.hpp` | 保存订单明细中的菜品编号、名称、数量、单价和小计 |
| `Order` | `restaurant_backend.hpp` | 保存订单编号、餐桌号、服务员、状态、金额、时间和明细链表 |

餐桌状态如下。

| 枚举值 | 含义 |
| --- | --- |
| `TableIdle` | 空闲 |
| `TableUsing` | 使用中 |
| `TableWaitPay` | 待结账 |

订单状态如下。

| 枚举值 | 含义 |
| --- | --- |
| `OrderActive` | 进行中 |
| `OrderWaitPay` | 待结账 |
| `OrderPaid` | 已结账 |
| `OrderCancelled` | 已取消 |

## 五、后端模块接口说明

后端核心类为 `Backend::RestaurantBackend`，声明在 `src/restaurant_backend.hpp`，实现在 `src/restaurant_backend.cpp`。

### 1. 文件与初始化接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `setDataDirectory(const std::string& directory)` | `directory` 数据文件目录 | 无 | 设置 `users.txt`、`dishes.txt` 等文件所在目录 |
| `load()` | 无 | `bool`，读取成功返回 `true` | 从 `data` 目录读取文本文件，加载到链表中 |
| `save() const` | 无 | 无 | 将链表中的用户、菜品、餐桌、订单和明细写回文本文件 |
| `clearHistoricalData(std::string& error)` | `error` 错误信息引用 | `bool`，成功返回 `true` | 清除已结账和已取消订单，重置销量、餐桌状态和订单编号 |

### 2. 登录与查询接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `login(role, username, password, displayName)` | `role` 角色，`username` 账号，`password` 密码 | `bool`，`displayName` 返回显示名称 | 校验登录身份，成功后返回用户姓名 |
| `users() const` | 无 | `const LinkedList<User>&` | 返回用户链表，只供界面读取显示 |
| `dishes() const` | 无 | `const LinkedList<Dish>&` | 返回菜品链表，只供界面读取显示 |
| `tables() const` | 无 | `const LinkedList<Table>&` | 返回餐桌链表，只供界面读取显示 |
| `orders() const` | 无 | `const LinkedList<Order>&` | 返回订单链表，只供界面读取显示 |

### 3. 用户管理接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `addUser(const User& user, std::string& error)` | `user` 新用户数据 | `bool`，`error` 返回失败原因 | 新增用户，检查账号是否为空或重复 |
| `updateUser(const User& user, std::string& error)` | `user` 修改后的用户数据 | `bool`，`error` 返回失败原因 | 修改用户密码、角色和姓名 |
| `deleteUser(const std::string& username, std::string& error)` | `username` 要删除的账号 | `bool`，`error` 返回失败原因 | 删除用户，同时保证至少保留一个管理员 |

### 4. 菜品管理接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `addDish(const Dish& dish, std::string& error)` | `dish` 新菜品数据 | `bool`，`error` 返回失败原因 | 新增菜品，检查编号、价格、库存和重复编号 |
| `updateDish(const Dish& dish, std::string& error)` | `dish` 修改后的菜品数据 | `bool`，`error` 返回失败原因 | 修改菜品名称、分类、价格和库存 |
| `deleteDish(int id, std::string& error)` | `id` 菜品编号 | `bool`，`error` 返回失败原因 | 删除菜品，如果菜品存在于未完成订单中则不允许删除 |

### 5. 餐桌管理接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `addTable(const Table& table, std::string& error)` | `table` 新餐桌数据 | `bool`，`error` 返回失败原因 | 新增餐桌，检查桌号和容量是否合法 |
| `updateTable(const Table& table, std::string& error)` | `table` 修改后的餐桌数据 | `bool`，`error` 返回失败原因 | 修改餐桌容量 |
| `deleteTable(int id, std::string& error)` | `id` 餐桌编号 | `bool`，`error` 返回失败原因 | 删除餐桌，使用中或待结账餐桌不允许删除 |

### 6. 订单业务接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `openTable(int tableId, const std::string& waiter, int& orderId, std::string& error)` | `tableId` 餐桌号，`waiter` 服务员账号 | `bool`，`orderId` 返回新订单号，`error` 返回失败原因 | 开台并创建进行中订单 |
| `addDishToOrder(int orderId, int dishId, int quantity, std::string& error)` | `orderId` 订单号，`dishId` 菜品编号，`quantity` 数量 | `bool`，`error` 返回失败原因 | 向订单添加菜品，扣减库存，更新订单金额 |
| `returnDish(int orderId, int dishId, int quantity, std::string& error)` | `orderId` 订单号，`dishId` 菜品编号，`quantity` 数量 | `bool`，`error` 返回失败原因 | 从订单退菜，恢复库存，更新订单金额 |
| `markWaitPay(int tableId, std::string& error)` | `tableId` 餐桌号 | `bool`，`error` 返回失败原因 | 将餐桌和订单设置为待结账 |
| `checkout(int tableId, std::string& error)` | `tableId` 餐桌号 | `bool`，`error` 返回失败原因 | 完成结账，更新销量，释放餐桌，记录结账时间 |
| `cancelOrder(int orderId, std::string& error)` | `orderId` 订单号 | `bool`，`error` 返回失败原因 | 取消未结账订单，恢复库存并释放餐桌 |

### 7. 格式化和辅助接口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `money(double value)` | `value` 金额 | `std::string` | 将金额格式化为两位小数 |
| `tableStatusName(int status)` | `status` 餐桌状态枚举 | `std::string` | 返回餐桌状态中文文本 |
| `orderStatusName(int status)` | `status` 订单状态枚举 | `std::string` | 返回订单状态中文文本 |
| `roleName(const std::string& role)` | `role` 角色字符串 | `std::string` | 返回管理员或服务员中文文本 |

## 六、前端模块函数说明

前端核心类为 `MainWindow`，定义在 `src/qt_main.cpp`。该类负责构建页面、绑定按钮事件、调用后端接口并刷新表格。

### 1. 程序入口

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `main(int argc, char* argv[])` | 命令行参数 | `int`，返回 Qt 事件循环退出码 | 创建 `QApplication`，读取 `data/style.qss`，设置数据目录，创建并显示主窗口 |
| `MainWindow()` | 无 | 无 | 构造主窗口，调用后端 `load()`，创建登录页和工作页 |

### 2. 页面构建函数

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `buildLoginPage()` | 无 | 无 | 创建登录界面，包含角色、账号、密码和登录按钮 |
| `buildWorkPage()` | 无 | 无 | 创建登录后的主界面，包含顶部用户信息、退出按钮和分页标签 |
| `buildDishesTab()` | 无 | `QWidget*` | 创建菜品管理页 |
| `buildTablesTab()` | 无 | `QWidget*` | 创建餐桌管理页 |
| `buildOrdersTab()` | 无 | `QWidget*` | 创建订单管理页 |
| `buildDataTab()` | 无 | `QWidget*` | 创建经营数据页，包含单日流水、月流水、销量库存统计 |
| `buildUsersTab()` | 无 | `QWidget*` | 创建用户管理页 |

### 3. 登录和权限函数

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `login()` | 无 | 无 | 读取界面账号密码，调用后端 `login()`，成功后进入主界面 |
| `applyPermissions()` | 无 | 无 | 根据管理员或服务员身份显示不同菜单和按钮 |

### 4. 数据刷新函数

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `refreshAll()` | 无 | 无 | 统一刷新菜品、餐桌、订单、用户、经营数据和订单明细 |
| `refreshDishes()` | 无 | 无 | 将菜品链表显示到菜品表格 |
| `refreshTables()` | 无 | 无 | 将餐桌链表显示到餐桌表格 |
| `refreshOrders()` | 无 | 无 | 将订单链表显示到订单表格 |
| `refreshUsers()` | 无 | 无 | 将用户链表显示到用户表格 |
| `refreshDateChoices(const LinkedList<Order>& orders)` | `orders` 订单链表 | 无 | 生成单日流水和月流水的日期下拉选项 |
| `refreshDataView()` | 无 | 无 | 统计累计营业额、单日流水、月流水、订单数量、餐桌占用和热销菜 |
| `refreshOrderItems()` | 无 | 无 | 根据当前选中订单刷新订单明细表格 |

### 5. 前端业务操作函数

| 函数 | 入口参数 | 出口参数或返回值 | 功能说明 |
| --- | --- | --- | --- |
| `editDish(bool existing)` | `existing` 为 `true` 表示修改，为 `false` 表示新增 | 无 | 弹窗新增或修改菜品，调用 `addDish()` 或 `updateDish()` |
| `deleteDish()` | 无 | 无 | 删除选中菜品，调用后端 `deleteDish()` |
| `editTable(bool existing)` | `existing` 为 `true` 表示修改，为 `false` 表示新增 | 无 | 弹窗新增或修改餐桌，调用 `addTable()` 或 `updateTable()` |
| `deleteTable()` | 无 | 无 | 删除选中餐桌，调用后端 `deleteTable()` |
| `editUser(bool existing)` | `existing` 为 `true` 表示修改，为 `false` 表示新增 | 无 | 弹窗新增或修改用户，调用 `addUser()` 或 `updateUser()` |
| `deleteUser()` | 无 | 无 | 删除选中用户，调用后端 `deleteUser()` |
| `openSelectedTable()` | 无 | 无 | 对选中餐桌开台，调用后端 `openTable()` |
| `addDishToSelectedOrder()` | 无 | 无 | 选择菜品和数量后加菜，调用后端 `addDishToOrder()` |
| `returnDishFromOrder()` | 无 | 无 | 对选中订单明细退菜，调用后端 `returnDish()` |
| `markSelectedOrderWaitPay()` | 无 | 无 | 将选中订单设置为待结账 |
| `checkoutSelectedOrder()` | 无 | 无 | 对选中订单对应餐桌结账 |
| `markSelectedTableWaitPay()` | 无 | 无 | 将选中餐桌设置为待结账 |
| `checkoutSelectedTable()` | 无 | 无 | 对选中餐桌直接结账 |
| `cancelSelectedOrder()` | 无 | 无 | 取消选中订单 |
| `clearHistoricalData()` | 无 | 无 | 管理员清除历史订单和经营统计数据 |

## 七、主要函数调用关系

### 1. 启动流程

```text
main()
  QApplication app
  appStyleSheet(dataDir)
  RestaurantBackend::setDataDirectory(dataDir)
  MainWindow window
    backend_.load()
    buildLoginPage()
    buildWorkPage()
  window.show()
  app.exec()
```

### 2. 登录流程

```text
登录按钮 clicked
  MainWindow::login()
    RestaurantBackend::login()
    MainWindow::applyPermissions()
    MainWindow::refreshAll()
    切换到主界面
```

### 3. 开台和加菜流程

```text
选择餐桌并点击开台/进入订单
  MainWindow::openSelectedTable()
    RestaurantBackend::openTable()
      创建 Order
      设置 TableUsing
      save()
    MainWindow::refreshAll()

点击加菜
  MainWindow::addDishToSelectedOrder()
    RestaurantBackend::addDishToOrder()
      检查库存和订单状态
      修改 OrderItem 链表
      扣减 Dish.stock
      recalcOrder()
      save()
    MainWindow::refreshAll()
```

### 4. 退菜、待结账和结账流程

```text
点击退菜
  MainWindow::returnDishFromOrder()
    RestaurantBackend::returnDish()
      减少订单明细数量
      恢复库存
      recalcOrder()
      save()
    MainWindow::refreshAll()

点击设为待结账
  MainWindow::markSelectedOrderWaitPay()
    RestaurantBackend::markWaitPay()
      设置订单状态 OrderWaitPay
      设置餐桌状态 TableWaitPay
      save()
    MainWindow::refreshAll()

点击结账
  MainWindow::checkoutSelectedOrder()
    RestaurantBackend::checkout()
      设置订单状态 OrderPaid
      写入 paidAt
      增加菜品 sold
      释放餐桌
      save()
    MainWindow::refreshAll()
```

### 5. 经营数据统计流程

```text
进入数据查看页或切换日期月份
  MainWindow::refreshDataView()
    RestaurantBackend::orders()
    RestaurantBackend::tables()
    RestaurantBackend::dishes()
    refreshDateChoices()
    统计累计营业额、单日流水、月流水、订单数和热销菜
    写入 dataSummaryTable_ 和 dishStatsTable_
```

### 6. 数据保存流程

```text
任意业务接口执行成功
  RestaurantBackend::save()
    写 users.txt
    写 dishes.txt
    写 tables.txt
    写 orders.txt
    写 order_items.txt
```

## 八、数据文件格式说明

所有数据文件都放在 `data/` 目录下，字段之间使用英文竖线 `|` 分隔。

| 文件 | 字段说明 |
| --- | --- |
| `users.txt` | 账号、密码、角色、姓名 |
| `dishes.txt` | 菜品编号、名称、分类、价格、库存、销量 |
| `tables.txt` | 桌号、容量、状态、当前金额、关联订单号 |
| `orders.txt` | 订单号、桌号、服务员、状态、总金额、创建时间、结账时间 |
| `order_items.txt` | 订单号、菜品编号、菜品名、数量、单价、小计 |

程序启动时调用 `load()` 读取这些文件，操作完成后调用 `save()` 写回这些文件。

## 九、主要功能

| 功能 | 管理员 | 服务员 |
| --- | --- | --- |
| 登录系统 | 支持 | 支持 |
| 菜品查看 | 支持 | 支持 |
| 新增、修改、删除菜品 | 支持 | 不显示 |
| 餐桌查看 | 支持 | 支持 |
| 新增、修改、删除餐桌 | 支持 | 不显示 |
| 开台、加菜、退菜、待结账、结账 | 支持 | 支持 |
| 订单查看和取消订单 | 支持 | 支持 |
| 经营数据查看 | 支持 | 不显示 |
| 用户管理 | 支持 | 不显示 |
| 清除历史数据 | 支持 | 不显示 |

## 十、运行说明

1. 打开 `RestaurantOrderingSystem_Submit` 文件夹
2. 双击 `RestaurantOrderingSystem.exe`
3. 使用默认账号登录
4. 管理员可维护基础数据并查看经营数据
5. 服务员可进行餐桌开台、点餐和结账操作

如果程序无法启动，请确认 `_runtime/` 文件夹与 `RestaurantOrderingSystem.exe` 位于同一目录下，不能只单独复制 exe 文件。
