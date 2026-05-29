#include "ui.hpp"

#include <conio.h>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <windows.h>

namespace {
    // 页面采用固定宽度布局，LEFT/WIDTH 控制所有边框和文本的对齐范围。
    const int LEFT = 10;
    const int WIDTH = 80;
    const int FOOTER_Y = 25;

    // Windows 控制台颜色值：普通白色、标题青色、错误红色等。
    // COLOR_SELECTED=240 表示黑字白底，用于菜单当前选中项。
    const int COLOR_DEFAULT = 7;
    const int COLOR_TITLE = 11;
    const int COLOR_SUCCESS = 10;
    const int COLOR_ERROR = 12;
    const int COLOR_WARNING = 14;
    const int COLOR_SELECTED = 240;
    const int COLOR_TIP = 8;

    // UTF-8 中文字符在控制台里通常占两个英文字符宽度。
    // 这里做一个简单估算，用来让标题和菜单项尽量居中。
    /*
        函数用途：
        估算字符串在控制台中占用的显示宽度。

        接口说明：
        - text：要估算的字符串。
        - 返回值：估算后的显示宽度。

        语法说明：
        UTF-8 中文字符通常由多个字节组成，所以不能直接用 text.size()
        当作屏幕宽度。这里通过判断字节开头大致跳过一个完整字符。
    */
    int displayWidth(const std::string& text) {
        int width = 0;
        for (size_t i = 0; i < text.size();) {
            unsigned char ch = static_cast<unsigned char>(text[i]);
            if (ch < 0x80) {
                ++width;
                ++i;
            } else {
                width += 2;
                if ((ch & 0xE0) == 0xC0) {
                    i += 2;
                } else if ((ch & 0xF0) == 0xE0) {
                    i += 3;
                } else if ((ch & 0xF8) == 0xF0) {
                    i += 4;
                } else {
                    ++i;
                }
            }
        }
        return width;
    }

    /*
        函数用途：
        在指定坐标输出文字，是 ui.cpp 内部使用的小工具。
    */
    void printAt(int x, int y, const std::string& text) {
        UI::gotoXY(x, y);
        std::cout << text;
    }
}

namespace UI {
    /*
        函数用途：
        初始化 Windows 控制台。

        主要工作：
        - 设置输入输出编码为 UTF-8。
        - 设置控制台标题。
        - 设置窗口大小。
        - 恢复默认颜色并清屏。

        语法说明：
        SetConsoleOutputCP、SetConsoleCP、SetConsoleTitleW 都是 Windows API。
    */
    void init() {
        // 使用 UTF-8 编码，配合 g++ 的 -finput-charset/-fexec-charset 参数，
        // 可以让中文菜单在新版 Windows Terminal 中正常显示。
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        SetConsoleTitleW(L"中小饭店点餐管理系统");
        std::system("mode con cols=100 lines=30 > nul");
        resetColor();
        clear();
    }

    /*
        函数用途：
        清空控制台屏幕。

        语法说明：
        std::system("cls") 会调用 Windows 的 cls 命令。
    */
    void clear() {
        std::system("cls");
    }

    /*
        函数用途：
        在底部显示提示，并等待用户按任意键。
    */
    void pause() {
        drawFooter("按任意键继续...");
        _getch();
    }

    /*
        函数用途：
        设置控制台文字颜色。

        接口说明：
        - color：Windows 控制台颜色值。
    */
    void setColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(color));
    }

    /*
        函数用途：
        恢复默认文字颜色。
    */
    void resetColor() {
        setColor(COLOR_DEFAULT);
    }

    /*
        函数用途：
        移动控制台光标到指定坐标。

        接口说明：
        - x：列号。
        - y：行号。

        语法说明：
        COORD 是 Windows 控制台坐标结构体。
    */
    void gotoXY(int x, int y) {
        COORD pos;
        pos.X = static_cast<SHORT>(x);
        pos.Y = static_cast<SHORT>(y);
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    }

    /*
        函数用途：
        绘制一个 ASCII 边框。

        接口说明：
        - x、y：左上角坐标。
        - width：宽度。
        - height：高度。

        注意：
        只使用 + - | 是为了避免中文环境下特殊边框字符乱码。
    */
    void drawBox(int x, int y, int width, int height) {
        if (width < 2 || height < 2) {
            return;
        }

        // 只使用 + - | 三种 ASCII 字符画边框，避免特殊边框字符乱码。
        std::string horizontal(width - 2, '-');
        printAt(x, y, "+" + horizontal + "+");
        for (int row = 1; row < height - 1; ++row) {
            printAt(x, y + row, "|" + std::string(width - 2, ' ') + "|");
        }
        printAt(x, y + height - 1, "+" + horizontal + "+");
    }

    /*
        函数用途：
        绘制页面顶部标题区域。
    */
    void drawHeader(const std::string& title) {
        clear();
        setColor(COLOR_TITLE);
        drawBox(LEFT, 1, WIDTH, 3);
        printCenter(2, title);
        resetColor();
    }

    /*
        函数用途：
        绘制页面底部提示区域。
    */
    void drawFooter(const std::string& tip) {
        setColor(COLOR_TIP);
        drawBox(LEFT, FOOTER_Y, WIDTH, 3);
        printCenter(FOOTER_Y + 1, tip);
        resetColor();
    }

    /*
        函数用途：
        在固定页面宽度内居中输出文本。

        学习重点：
        居中位置 = 左边界 + (总宽度 - 文本显示宽度) / 2。
    */
    void printCenter(int y, const std::string& text) {
        int x = LEFT + (WIDTH - displayWidth(text)) / 2;
        if (x < LEFT + 1) {
            x = LEFT + 1;
        }
        printAt(x, y, text);
    }

    /*
        函数用途：
        通用菜单选择组件。

        接口说明：
        - title：菜单标题。
        - items：菜单选项数组。
        - 返回值：用户按 Enter 选中的下标；按 Esc 返回 -1。

        语法说明：
        - std::vector<std::string> 用来保存可变数量的菜单项。
        - _getch() 可以不等待回车，直接读取按键。
        - 方向键会产生两次按键码，因此代码里要额外读取一次。
    */
    int menuSelect(const std::string& title, const std::vector<std::string>& items) {
        if (items.empty()) {
            return -1;
        }

        int selected = 0;
        while (true) {
            // 每次按键后重新绘制整个菜单，逻辑简单且页面不会残留旧高亮。
            drawHeader(title);
            drawBox(LEFT, 5, WIDTH, 18);

            int startY = 8;
            for (size_t i = 0; i < items.size(); ++i) {
                std::string line = (static_cast<int>(i) == selected ? "> " : "  ") + items[i];
                int x = LEFT + (WIDTH - displayWidth(line)) / 2;
                int y = startY + static_cast<int>(i) * 2;
                gotoXY(x, y);
                if (static_cast<int>(i) == selected) {
                    setColor(COLOR_SELECTED);
                    std::cout << "  " << line << "  ";
                    resetColor();
                } else {
                    std::cout << line;
                }
            }

            drawFooter("↑ ↓ 选择菜单    Enter 确认    Esc 返回");

            // _getch() 读取方向键时会先返回 224 或 0，再读取一次才是具体键值。
            // 上键是 72，下键是 80；Enter 是 13；Esc 是 27。
            int key = _getch();
            if (key == 224 || key == 0) {
                key = _getch();
                if (key == 72) {
                    selected = (selected - 1 + static_cast<int>(items.size())) % static_cast<int>(items.size());
                } else if (key == 80) {
                    selected = (selected + 1) % static_cast<int>(items.size());
                }
            } else if (key == 13) {
                return selected;
            } else if (key == 27) {
                return -1;
            }
        }
    }

    /*
        函数用途：
        显示普通消息框。
    */
    void showMessage(const std::string& title, const std::string& message) {
        drawHeader(title);
        drawBox(LEFT, 8, WIDTH, 8);
        printCenter(11, message);
        pause();
    }

    /*
        函数用途：
        显示成功提示框。
    */
    void showSuccess(const std::string& message) {
        drawHeader("操作成功");
        drawBox(LEFT, 8, WIDTH, 8);
        setColor(COLOR_SUCCESS);
        printCenter(11, message);
        resetColor();
        pause();
    }

    /*
        函数用途：
        显示错误提示框。
    */
    void showError(const std::string& message) {
        drawHeader("操作失败");
        drawBox(LEFT, 8, WIDTH, 8);
        setColor(COLOR_ERROR);
        printCenter(11, message);
        resetColor();
        pause();
    }

    /*
        函数用途：
        显示确认框，等待用户按 Y 或 N。

        返回值：
        - true：用户按 Y。
        - false：用户按 N 或 Esc。
    */
    bool confirm(const std::string& message) {
        drawHeader("确认操作");
        drawBox(LEFT, 8, WIDTH, 8);
        setColor(COLOR_WARNING);
        printCenter(10, message);
        resetColor();
        printCenter(12, "按 Y 确认，按 N 取消");
        drawFooter("Y 确认    N 取消");

        while (true) {
            // 只接受明确的 Y/N 操作，避免误按其他键造成危险操作。
            int key = _getch();
            if (key == 'Y' || key == 'y') {
                return true;
            }
            if (key == 'N' || key == 'n' || key == 27) {
                return false;
            }
        }
    }

    /*
        函数用途：
        读取一行字符串输入。

        接口说明：
        - label：输入提示。
        - 返回值：用户输入的整行文本。
    */
    std::string inputString(const std::string& label) {
        std::string value;
        setColor(COLOR_DEFAULT);
        std::cout << label << "：";
        resetColor();
        std::getline(std::cin, value);
        return value;
    }

    /*
        函数用途：
        读取指定范围内的整数。

        接口说明：
        - label：输入提示。
        - minValue：最小允许值。
        - maxValue：最大允许值。
        - 返回值：合法整数。

        语法说明：
        先用字符串接收，再用 stringstream 转成 int，可以避免用户输入字母导致程序崩溃。
    */
    int inputInt(const std::string& label, int minValue, int maxValue) {
        while (true) {
            std::string text = inputString(label);
            std::stringstream ss(text);
            int value = 0;
            char extra = '\0';

            // 先用字符串接收，再用 stringstream 转换，能处理输入字母等错误情况。
            // extra 用于判断是否存在 “12abc” 这种混合输入。
            if ((ss >> value) && !(ss >> extra) && value >= minValue && value <= maxValue) {
                return value;
            }

            setColor(COLOR_ERROR);
            std::cout << "输入错误，请输入 " << minValue << " 到 " << maxValue << " 之间的整数。" << std::endl;
            resetColor();
        }
    }
}
