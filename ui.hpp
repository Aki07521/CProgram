#ifndef UI_HPP
#define UI_HPP

#include <string>
#include <vector>

namespace UI {
    // 初始化控制台：设置窗口大小、标题、编码和默认颜色。
    void init();

    // 清空当前控制台屏幕。
    void clear();

    // 在底部显示“按任意键继续”，等待用户按键。
    void pause();

    // 设置/恢复控制台文字颜色。
    void setColor(int color);
    void resetColor();

    // 将光标移动到指定坐标，x 为列，y 为行。
    void gotoXY(int x, int y);

    // 绘制 ASCII 边框和统一页头、页脚。
    void drawBox(int x, int y, int width, int height);
    void drawHeader(const std::string& title);
    void drawFooter(const std::string& tip);

    // 在页面固定宽度范围内大致居中输出文本。
    void printCenter(int y, const std::string& text);

    // 通用菜单组件：上下键切换，Enter 返回下标，Esc 返回 -1。
    int menuSelect(const std::string& title, const std::vector<std::string>& items);

    // 统一消息框，分别用于普通提示、成功提示和错误提示。
    void showMessage(const std::string& title, const std::string& message);
    void showSuccess(const std::string& message);
    void showError(const std::string& message);

    // 确认框：Y 返回 true，N 或 Esc 返回 false。
    bool confirm(const std::string& message);

    // 输入函数：inputInt 会循环校验范围，避免输入字母导致程序异常。
    std::string inputString(const std::string& label);
    int inputInt(const std::string& label, int minValue, int maxValue);
}

#endif
