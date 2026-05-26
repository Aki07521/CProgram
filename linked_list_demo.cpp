#include <iostream>
#include <string>

/*
    这是一个“单链表”教学示例。

    适合你现在这种状态：
    - 还没有系统学过链表
    - 想先看一个最小、最直观、能跑起来的例子
    - 想把它和当前点餐系统里的 g_users / g_dishes 这种写法对应起来

    你可以先把链表理解成：
    1. 每个数据单独放在一个“节点”里
    2. 每个节点都知道“下一个节点是谁”
    3. 最前面的那个节点地址，叫“头指针”

    头指针就像一串珠子的起点。
    只要拿到起点，就能顺着 next 一颗一颗找下去。
*/

// 节点结构体：每个节点只存一条数据，以及指向下一个节点的指针。
struct StudentNode {
    int id;                 // 学号
    std::string name;       // 姓名
    StudentNode* next;      // 指向下一个节点
};

/*
    头指针：
    它不是“第一个节点本身”，而是“指向第一个节点的指针”。
    如果链表为空，head = nullptr。
*/
StudentNode* head = nullptr;

// 创建一个新节点。
StudentNode* createNode(int id, const std::string& name) {
    StudentNode* node = new StudentNode;
    node->id = id;
    node->name = name;
    node->next = nullptr;   // 先不连任何后继节点
    return node;
}

/*
    尾插法：把新节点加到链表最后面。

    为什么要走到最后？
    因为单链表只有“向后”的指针，没有“向前”的指针。
    如果你想把节点加到尾部，就必须从头开始一路找到最后一个节点。
*/
void appendNode(int id, const std::string& name) {
    StudentNode* node = createNode(id, name);

    // 1. 如果链表为空，头指针直接指向新节点。
    if (head == nullptr) {
        head = node;
        return;
    }

    // 2. 如果链表不为空，就找到最后一个节点。
    StudentNode* p = head;
    while (p->next != nullptr) {
        p = p->next;
    }

    // 3. 让最后一个节点的 next 指向新节点。
    p->next = node;
}

/*
    遍历链表：
    从头节点开始，顺着 next 一直往后走，直到 nullptr。

    这就是链表最常见的使用方式。
*/
void printList() {
    if (head == nullptr) {
        std::cout << "链表为空。\n";
        return;
    }

    StudentNode* p = head;
    while (p != nullptr) {
        std::cout << "学号: " << p->id << ", 姓名: " << p->name << '\n';
        p = p->next;
    }
}

/*
    查找节点：
    链表没有下标，所以只能从头开始一个一个比对。

    这和数组不一样：
    - 数组可以直接 arr[3]
    - 链表必须从 head 开始找
*/
StudentNode* findById(int id) {
    StudentNode* p = head;
    while (p != nullptr) {
        if (p->id == id) {
            return p; // 找到就直接返回
        }
        p = p->next;
    }
    return nullptr; // 没找到
}

/*
    删除节点：
    这是链表里最容易卡住的地方。

    因为你不能“直接把某个节点删掉”就完事，
    你还要让“前一个节点”跳过它，连到它后面去。

    例如：
    A -> B -> C

    想删除 B，就要让 A->next 指向 C。
*/
bool removeById(int id) {
    if (head == nullptr) {
        return false;
    }

    // 如果要删的是头节点，要单独处理。
    if (head->id == id) {
        StudentNode* oldHead = head;
        head = head->next;
        delete oldHead;
        return true;
    }

    // prev 指向前一个节点，cur 指向当前节点。
    StudentNode* prev = head;
    StudentNode* cur = head->next;

    while (cur != nullptr) {
        if (cur->id == id) {
            prev->next = cur->next; // 前一个节点绕过当前节点
            delete cur;             // 释放被删除的节点
            return true;
        }
        prev = cur;
        cur = cur->next;
    }

    return false;
}

// 释放整个链表，防止内存泄漏。
void clearList() {
    while (head != nullptr) {
        StudentNode* next = head->next;
        delete head;
        head = next;
    }
}

/*
    这个 main 函数就是教学演示入口。
    你可以单独编译这个文件来运行，不会影响你的点餐系统主程序。
*/
int main() {
    std::cout << "1. 先插入 3 个学生节点\n";
    appendNode(1001, "张三");
    appendNode(1002, "李四");
    appendNode(1003, "王五");

    std::cout << "\n2. 输出整个链表\n";
    printList();

    std::cout << "\n3. 查找学号 1002\n";
    StudentNode* found = findById(1002);
    if (found != nullptr) {
        std::cout << "找到了: " << found->id << " " << found->name << '\n';
    } else {
        std::cout << "没找到\n";
    }

    std::cout << "\n4. 删除学号 1002\n";
    if (removeById(1002)) {
        std::cout << "删除成功\n";
    } else {
        std::cout << "删除失败\n";
    }

    std::cout << "\n5. 再次输出整个链表\n";
    printList();

    clearList();
    return 0;
}
