#include "app.hpp"
#include "ui.hpp"

int main() {
    // 程序入口：先初始化 Windows 控制台，再进入系统主菜单。
    // 后续所有页面跳转都从 showMainMenu() 开始统一调度。
    UI::init();
    showMainMenu();

    // 退出前恢复默认颜色并清屏，避免控制台保留高亮颜色。
    UI::resetColor();
    UI::clear();
    return 0;
}
