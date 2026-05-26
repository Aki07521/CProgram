#include "app.hpp"

/*
    app.cpp 现在只保留 app 模块的总入口说明。

    第一版项目把所有业务函数都写在这个文件里，虽然能运行，但阅读时很难分清：
    哪些函数负责数据结构，哪些函数负责文件读写，哪些函数负责页面流程。

    学习版已经按职责拆成以下文件：
    - app_internal.hpp：内部数据结构、全局链表和函数声明。
    - app_core.cpp：链表操作、订单金额/库存同步、输入校验等核心规则。
    - app_persistence.cpp：users.txt、dishes.txt、tables.txt、orders.txt、order_items.txt 的读写。
    - app_display.cpp：列表、小票等只读展示页面。
    - app_menus.cpp：系统入口、登录、管理员菜单、服务员菜单。
    - app_admin.cpp：管理员维护和统计功能。
    - app_waiter.cpp：服务员点餐、退菜、结账功能。

    这个文件不再定义函数，保留下来是为了让旧项目结构中仍然能看到 app 模块的学习路线。
*/
