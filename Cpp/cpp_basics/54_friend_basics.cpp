// 54_friend_basics.cpp
// friend 基础：友元函数、友元类与使用边界

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：friend 函数的基本概念
// ============================================================
// friend 不是成员函数，也不是继承关系
// 它的本质是：类主动授权某个外部函数访问自己的 private / protected 成员

class Box {
private:
    double length;
    double width;

public:
    Box(double length, double width) : length(length), width(width) {}

    void show() const {
        cout << "Box 的长=" << length << "，宽=" << width << endl;
    }

    // 友元函数声明：允许这个外部函数访问私有成员
    friend double calculateArea(const Box& box);
};

// 这是普通的类外函数，但因为被声明为 friend，所以可以访问私有成员
// 注意：它不是 Box 的成员，因此没有 this 指针
// 这说明 friend 的本质是“特权访问”，不是“类的一部分”
double calculateArea(const Box& box) {
    return box.length * box.width;
}

void demoFriendFunction() {
    Box box(5.0, 3.0);
    box.show();
    cout << "通过友元函数计算面积：" << calculateArea(box) << endl;
}

// ============================================================
// 第二部分：friend 类的基本概念
// ============================================================
// 如果一个类需要频繁访问另一个类的内部数据，
// 可以把整个类声明为 friend class

class Secret {
private:
    string message;
    int level;

public:
    Secret(const string& message, int level) : message(message), level(level) {}

    // 授权 Inspector 类访问自己的私有成员
    friend class Inspector;
};

class Inspector {
public:
    void inspect(const Secret& secret) const {
        cout << "Inspector 正在查看机密信息..." << endl;
        cout << "message = " << secret.message << endl;
        cout << "level = " << secret.level << endl;
    }
};

void demoFriendClass() {
    Secret secret("项目代号：Alpha", 3);
    Inspector inspector;
    inspector.inspect(secret);
}

// ============================================================
// 第三部分：friend 的边界与规则
// ============================================================
// friend 很强大，但它有几个重要边界：
// 1. 不对称：A 把 B 声明为 friend，不代表 B 自动把 A 也声明为 friend
// 2. 不传递：A 的 friend 是 B，B 的 friend 是 C，不代表 C 就是 A 的 friend
// 3. 不继承：父类的 friend，不会自动成为子类的 friend

class Vault {
private:
    int password;

public:
    Vault(int password) : password(password) {}

    friend class Key;
};

class Key {
public:
    void open(const Vault& vault) const {
        cout << "Key 可以访问 Vault 的密码：" << vault.password << endl;
    }
};

class CopyKey {
public:
    void tryOpen() const {
        cout << "CopyKey 不是 Vault 的 friend，不能直接访问其私有成员" << endl;
        // cout << vault.password; // 错误示例：不能编译
    }
};

void explainFriendRules() {
    Vault vault(123456);
    Key key;
    CopyKey copyKey;

    key.open(vault);
    copyKey.tryOpen();

    cout << "1. friend 是单向授权，不是双向关系" << endl;
    cout << "2. friend 不能层层传递" << endl;
    cout << "3. friend 也不会随着继承自动扩散" << endl;
    cout << "4. 所以 friend 应遵循最小授权原则" << endl;
}

// ============================================================
// 第四部分：什么时候不用 friend
// ============================================================
// 如果只是读取数据，很多时候 getter 或普通成员函数就够了
// 只有在确实需要突破封装、并且设计上合理时，才考虑 friend

class StudentScore {
private:
    string name;
    int score;

public:
    StudentScore(const string& name, int score) : name(name), score(score) {}

    string getName() const {
        return name;
    }

    int getScore() const {
        return score;
    }

    void printInfo() const {
        cout << "学生：" << name << "，分数：" << score << endl;
    }
};

void compareFriendAndGetter() {
    StudentScore student("Alice", 92);

    cout << "使用成员函数输出：" << endl;
    student.printInfo();

    cout << "使用 getter 读取数据：" << endl;
    cout << "姓名：" << student.getName() << "，分数：" << student.getScore() << endl;

    cout << "结论：不是所有访问 private 的需求都必须靠 friend" << endl;
    cout << "如果已有更安全、更清晰的接口，优先使用普通成员函数或 getter" << endl;
}

// ============================================================
// 第五部分：学习提示
// ============================================================

void printHints() {
    cout << "1. friend 是授权访问，不是成员函数，也不是继承" << endl;
    cout << "2. friend function 适合某些类外辅助逻辑" << endl;
    cout << "3. friend class 适合两个类关系非常紧密的场景" << endl;
    cout << "4. 能不用 friend 时，尽量用普通接口保持封装" << endl;
    cout << "5. 使用 friend 时要问自己：为什么一定要开这个后门？" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "        friend 基础学习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：friend 函数 ===" << endl;
    demoFriendFunction();

    cout << "\n=== 第二部分：friend 类 ===" << endl;
    demoFriendClass();

    cout << "\n=== 第三部分：friend 的边界 ===" << endl;
    explainFriendRules();

    cout << "\n=== 第四部分：什么时候不用 friend ===" << endl;
    compareFriendAndGetter();

    cout << "\n=== 第五部分：学习提示 ===" << endl;
    printHints();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. friend 的本质是特权访问，不是成员，也不是继承" << endl;
    cout << "2. friend function 适合少量类外函数访问对象内部数据" << endl;
    cout << "3. friend class 是对整个类授权，权限更大，要更谨慎" << endl;
    cout << "4. friend 不对称、不传递、不继承" << endl;
    cout << "5. 设计类时应优先考虑封装，friend 只在必要时使用" << endl;

    return 0;
}
