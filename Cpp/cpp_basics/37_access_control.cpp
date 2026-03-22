// ============================================
// 访问控制 - 超详细注释版
// 日期：2026-03-17（第11天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】访问控制与封装

是什么？
- 访问控制是限制类成员访问权限的机制
- 封装是将数据和方法包装在一起，隐藏内部实现细节

为什么需要？
- 保护数据安全，防止外部随意修改
- 隐藏实现细节，只暴露必要的接口
- 提高代码的可维护性和可扩展性

核心特点：
- public：公有成员，任何地方都可以访问
- private：私有成员，只能在类内部访问
- protected：保护成员，类内部和子类可以访问
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 概念1：访问控制关键字
    // --------------------------------------------

    cout << "\n1.1 访问控制关键字" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【public - 公有】

是什么？
- public成员可以在任何地方访问
- 包括类内部、类外部、子类

使用场景：
- 对外提供的接口（成员函数）
- 需要外部访问的成员变量（较少使用）

【private - 私有】

是什么？
- private成员只能在类内部访问
- 类外部和子类都不能直接访问

使用场景：
- 需要保护的数据（成员变量）
- 内部辅助函数（不对外公开）

【protected - 保护】

是什么？
- protected成员可以在类内部和子类中访问
- 类外部不能访问

使用场景：
- 需要被子类继承和访问的成员
- 后续学习继承时会详细讲解

【默认访问权限】

class：默认是private
struct：默认是public
)" << endl;

    // --------------------------------------------
    // 1.2 概念2：封装
    // --------------------------------------------

    cout << "\n1.2 封装的概念" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【封装（Encapsulation）】

是什么？
- 将数据（成员变量）和操作数据的方法（成员函数）包装在一起
- 隐藏内部实现细节，只暴露必要的接口

为什么需要？
- 数据安全：防止外部随意修改数据
- 降低耦合：外部只需要知道接口，不需要知道实现
- 易于维护：修改内部实现不影响外部使用

封装的原则：
1. 成员变量设为private
2. 提供public的getter和setter方法
3. 只暴露必要的接口
)" << endl;

    // --------------------------------------------
    // 1.3 重要概念：getter和setter ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.3 getter和setter方法" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【getter方法】

是什么？
- 用于获取私有成员变量的值
- 通常命名为 getXxx()

作用：
- 提供对私有数据的只读访问
- 可以在返回前进行数据处理

【setter方法】

是什么？
- 用于设置私有成员变量的值
- 通常命名为 setXxx()

作用：
- 提供对私有数据的写入访问
- 可以在设置前进行数据验证

【命名规范】

getter：getXxx() 或 xxx()（无参数）
setter：setXxx(参数)

示例：
private:
    int age;
public:
    int getAge() { return age; }
    void setAge(int a) { age = a; }
)" << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

#include <iostream>
#include <string>
using namespace std;

// --------------------------------------------
// 2.1 示例1：使用访问控制
// --------------------------------------------

// 定义一个银行账户类，演示访问控制
class BankAccount {
private:  // 私有成员：只能在类内部访问
    string accountNumber;  // 账号
    double balance;        // 余额
    string password;       // 密码

public:   // 公有成员：可以在任何地方访问
    // 构造函数（后续会详细学习）
    // 用于初始化对象
    void init(string accNum, string pwd) {
        accountNumber = accNum;
        password = pwd;
        balance = 0.0;  // 初始余额为0
    }

    // getter方法：获取账号
    string getAccountNumber() {
        return accountNumber;
    }

    // getter方法：获取余额
    double getBalance() {
        return balance;
    }

    // 存款方法
    // 参数：amount - 存款金额
    // 返回：bool - 是否成功
    bool deposit(double amount) {
        // 数据验证：金额必须大于0
        if (amount <= 0) {
            cout << "错误：存款金额必须大于0" << endl;
            return false;
        }

        balance += amount;  // 增加余额
        cout << "存款成功：" << amount << "元" << endl;
        return true;
    }

    // 取款方法
    // 参数：amount - 取款金额
    //       pwd - 密码
    // 返回：bool - 是否成功
    bool withdraw(double amount, string pwd) {
        // 验证密码
        if (pwd != password) {
            cout << "错误：密码错误" << endl;
            return false;
        }

        // 验证金额
        if (amount <= 0) {
            cout << "错误：取款金额必须大于0" << endl;
            return false;
        }

        // 验证余额
        if (amount > balance) {
            cout << "错误：余额不足" << endl;
            return false;
        }

        balance -= amount;  // 减少余额
        cout << "取款成功：" << amount << "元" << endl;
        return true;
    }

    // 显示账户信息
    void display() {
        cout << "账号：" << accountNumber << endl;
        cout << "余额：" << balance << "元" << endl;
        // 注意：不显示密码（保护隐私）
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 创建银行账户对象
    // --------------------------------------------

    cout << "\n2.1 创建银行账户" << endl;
    cout << string(60, '-') << endl;

    BankAccount account;
    account.init("6222021234567890", "123456");

    cout << "账户创建成功：" << endl;
    account.display();

    // --------------------------------------------
    // 2.2 存款操作
    // --------------------------------------------

    cout << "\n2.2 存款操作" << endl;
    cout << string(60, '-') << endl;

    account.deposit(1000.0);  // 存款1000元
    account.deposit(500.0);   // 存款500元

    cout << "\n当前账户信息：" << endl;
    account.display();

    // --------------------------------------------
    // 2.3 取款操作
    // --------------------------------------------

    cout << "\n2.3 取款操作" << endl;
    cout << string(60, '-') << endl;

    // 正确密码，取款成功
    account.withdraw(300.0, "123456");

    cout << "\n当前账户信息：" << endl;
    account.display();

    // 错误密码，取款失败
    cout << "\n尝试使用错误密码取款：" << endl;
    account.withdraw(100.0, "wrong");

    // 余额不足，取款失败
    cout << "\n尝试取款超过余额：" << endl;
    account.withdraw(2000.0, "123456");

    // --------------------------------------------
    // 2.4 验证封装性
    // --------------------------------------------

    cout << "\n2.4 验证封装性" << endl;
    cout << string(60, '-') << endl;

    // ❌ 错误：不能直接访问私有成员
    // account.balance = 10000;  // 编译错误
    // account.password = "new";  // 编译错误

    // ✅ 正确：通过公有方法访问
    cout << "账号：" << account.getAccountNumber() << endl;
    cout << "余额：" << account.getBalance() << "元" << endl;

    cout << "\n封装保护了数据安全：" << endl;
    cout << "- 不能直接修改余额" << endl;
    cout << "- 不能直接访问密码" << endl;
    cout << "- 必须通过方法操作，方法中有验证逻辑" << endl;

    return 0;
}

// ============================================
// 第三部分：getter和setter最佳实践
// ============================================

#include <iostream>
#include <string>
using namespace std;

// 定义一个学生类，演示getter和setter的最佳实践
class Student {
private:
    string name;
    int age;
    int score;

public:
    // setter方法：设置姓名
    // 包含数据验证
    void setName(string n) {
        // 验证：姓名不能为空
        if (n.empty()) {
            cout << "错误：姓名不能为空" << endl;
            return;
        }
        name = n;
    }

    // getter方法：获取姓名
    string getName() {
        return name;
    }

    // setter方法：设置年龄
    // 包含数据验证
    void setAge(int a) {
        // 验证：年龄必须在合理范围内
        if (a < 0 || a > 150) {
            cout << "错误：年龄必须在0-150之间" << endl;
            return;
        }
        age = a;
    }

    // getter方法：获取年龄
    int getAge() {
        return age;
    }

    // setter方法：设置成绩
    // 包含数据验证
    void setScore(int s) {
        // 验证：成绩必须在0-100之间
        if (s < 0 || s > 100) {
            cout << "错误：成绩必须在0-100之间" << endl;
            return;
        }
        score = s;
    }

    // getter方法：获取成绩
    int getScore() {
        return score;
    }

    // 计算等级（根据成绩）
    // 这是一个只读属性，没有setter
    string getGrade() {
        if (score >= 90) return "优秀";
        if (score >= 80) return "良好";
        if (score >= 70) return "中等";
        if (score >= 60) return "及格";
        return "不及格";
    }

    // 显示学生信息
    void display() {
        cout << "姓名：" << name << endl;
        cout << "年龄：" << age << endl;
        cout << "成绩：" << score << endl;
        cout << "等级：" << getGrade() << endl;
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第三部分：getter和setter最佳实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 使用setter设置数据
    // --------------------------------------------

    cout << "\n3.1 使用setter设置数据" << endl;
    cout << string(60, '-') << endl;

    Student stu;

    // 使用setter方法设置数据
    stu.setName("张三");
    stu.setAge(18);
    stu.setScore(85);

    cout << "学生信息：" << endl;
    stu.display();

    // --------------------------------------------
    // 3.2 数据验证
    // --------------------------------------------

    cout << "\n3.2 数据验证" << endl;
    cout << string(60, '-') << endl;

    // 尝试设置无效数据
    cout << "尝试设置无效年龄：" << endl;
    stu.setAge(200);  // 超出范围

    cout << "\n尝试设置无效成绩：" << endl;
    stu.setScore(150);  // 超出范围

    cout << "\n尝试设置空姓名：" << endl;
    stu.setName("");  // 空字符串

    cout << "\n当前学生信息（数据未被破坏）：" << endl;
    stu.display();

    // --------------------------------------------
    // 3.3 只读属性
    // --------------------------------------------

    cout << "\n3.3 只读属性" << endl;
    cout << string(60, '-') << endl;

    cout << "等级是只读属性，根据成绩自动计算：" << endl;
    cout << "当前等级：" << stu.getGrade() << endl;

    // 修改成绩，等级自动变化
    stu.setScore(95);
    cout << "\n修改成绩为95后：" << endl;
    cout << "新等级：" << stu.getGrade() << endl;

    return 0;
}

/*
==============================================
🔍 深入理解：封装的意义（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【封装的深层意义】

1. 数据安全
   - 防止外部随意修改数据
   - 通过setter进行数据验证
   - 保护敏感信息（如密码）

2. 降低耦合
   - 外部只需要知道接口，不需要知道实现
   - 内部实现可以随时修改，不影响外部使用
   - 例如：余额的存储方式改变，外部代码不需要修改

3. 易于维护
   - 修改内部实现不影响外部
   - 添加新功能更容易
   - 代码更清晰，职责更明确

【封装 vs 不封装】

不封装的问题：
class BankAccount {
public:
    double balance;  // 公有成员变量
};

BankAccount acc;
acc.balance = -1000;  // 可以设置负数余额！
acc.balance = 999999999;  // 可以随意修改余额！

封装后的优势：
class BankAccount {
private:
    double balance;
public:
    bool deposit(double amount) {
        if (amount <= 0) return false;  // 验证
        balance += amount;
        return true;
    }
};

BankAccount acc;
acc.deposit(-1000);  // 验证失败，余额不会变成负数
// acc.balance = 999999999;  // 编译错误，不能直接访问

【封装的最佳实践】

1. 成员变量设为private
2. 提供public的getter和setter
3. 在setter中进行数据验证
4. 只暴露必要的接口
5. 隐藏内部实现细节

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：忘记访问控制关键字
    // --------------------------------------------

    cout << "\n4.1 错误1：忘记访问控制关键字" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
    string name;  // 没有指定访问权限
    int age;
};

Student stu;
stu.name = "张三";  // 编译错误！

结果：编译错误，无法访问私有成员

原因：class默认是private，外部无法访问

✅ 正确代码：
class Student {
public:  // 明确指定为public
    string name;
    int age;
};

或者：
class Student {
private:
    string name;
    int age;
public:
    void setName(string n) { name = n; }
    string getName() { return name; }
};

记忆技巧：
- class默认private，struct默认public
- 养成明确指定访问权限的习惯
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：所有成员都设为public
    // --------------------------------------------

    cout << "\n4.2 错误2：所有成员都设为public" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class BankAccount {
public:
    double balance;  // 余额设为public
};

BankAccount acc;
acc.balance = -1000;  // 可以设置负数！

结果：数据不安全，可以被随意修改

原因：没有封装，没有数据验证

✅ 正确代码：
class BankAccount {
private:
    double balance;  // 余额设为private
public:
    bool deposit(double amount) {
        if (amount <= 0) return false;  // 验证
        balance += amount;
        return true;
    }
};

记忆技巧：
- 成员变量应该设为private
- 通过public方法访问，方法中进行验证
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：getter返回私有成员的引用
    // --------------------------------------------

    cout << "\n4.3 错误3：getter返回私有成员的引用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
private:
    int score;
public:
    int& getScore() { return score; }  // 返回引用
};

Student stu;
stu.getScore() = 999;  // 可以通过引用修改私有成员！

结果：破坏了封装，私有成员可以被修改

原因：返回引用允许外部修改私有成员

✅ 正确代码：
class Student {
private:
    int score;
public:
    int getScore() { return score; }  // 返回值，不是引用
    void setScore(int s) {
        if (s >= 0 && s <= 100) score = s;  // 验证
    }
};

记忆技巧：
- getter返回值，不返回引用（除非有特殊需求）
- 修改数据必须通过setter，setter中进行验证
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：setter没有数据验证
    // --------------------------------------------

    cout << "\n4.4 错误4：setter没有数据验证" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
private:
    int age;
public:
    void setAge(int a) { age = a; }  // 没有验证
};

Student stu;
stu.setAge(-10);  // 可以设置负数年龄！

结果：数据不合理，没有起到保护作用

原因：setter没有进行数据验证

✅ 正确代码：
class Student {
private:
    int age;
public:
    void setAge(int a) {
        if (a < 0 || a > 150) {  // 验证
            cout << "错误：年龄无效" << endl;
            return;
        }
        age = a;
    }
};

记忆技巧：
- setter必须进行数据验证
- 验证失败时，不修改数据，并给出错误提示
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：在类外定义成员函数时忘记类名
    // --------------------------------------------

    cout << "\n4.5 错误5：在类外定义成员函数时忘记类名" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
public:
    void display();  // 声明
};

void display() {  // 忘记类名
    // ...
}

结果：编译错误或定义了一个全局函数

原因：在类外定义成员函数时，必须加类名::

✅ 正确代码：
class Student {
public:
    void display();  // 声明
};

void Student::display() {  // 类名::函数名
    // ...
}

记忆技巧：
- 类外定义成员函数：返回类型 类名::函数名(参数)
- ::是作用域解析运算符，表示该函数属于哪个类
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

#include <iostream>
using namespace std;

int main() {
    cout << "函数卡片速查" << endl;

    cout << R"(
【访问控制关键字】

public:
- 作用：声明公有成员
- 访问：任何地方都可以访问
- 使用：对外提供的接口

private:
- 作用：声明私有成员
- 访问：只能在类内部访问
- 使用：需要保护的数据

protected:
- 作用：声明保护成员
- 访问：类内部和子类可以访问
- 使用：需要被子类继承的成员

────────────────────────────────────

【getter方法】

语法：返回类型 getXxx()
作用：获取私有成员变量的值
返回：成员变量的值（通常返回值，不返回引用）

示例：
int getAge() { return age; }

────────────────────────────────────

【setter方法】

语法：void setXxx(参数类型 参数)
作用：设置私有成员变量的值
参数：要设置的新值
返回：通常void，或bool表示是否成功

示例：
void setAge(int a) {
    if (a >= 0 && a <= 150) age = a;
}
)" << endl;

    return 0;
}

// ============================================
// 第六部分：练习题
// ============================================

#include <iostream>
using namespace std;

int main() {
    cout << "练习题" << endl;

    cout << R"(
1. 设计一个图书类（Book）
   要求：
   - 私有成员：书名、作者、价格、库存数量
   - 公有方法：getter和setter（setter要验证数据）
   - 公有方法：借书（减少库存）、还书（增加库存）
   提示：价格不能为负，库存不能为负

2. 设计一个温度类（Temperature）
   要求：
   - 私有成员：摄氏温度
   - 公有方法：设置摄氏温度、获取摄氏温度
   - 公有方法：获取华氏温度（只读属性，根据摄氏温度计算）
   提示：华氏温度 = 摄氏温度 * 1.8 + 32

3. 设计一个密码类（Password）
   要求：
   - 私有成员：密码字符串
   - 公有方法：设置密码（要求长度至少6位）
   - 公有方法：验证密码（输入密码，返回是否正确）
   - 公有方法：修改密码（需要输入旧密码验证）
   提示：不提供获取密码的方法（安全考虑）

4. 设计一个计数器类（Counter）
   要求：
   - 私有成员：计数值
   - 公有方法：增加计数、减少计数、重置计数、获取计数
   - 限制：计数值不能小于0
   提示：减少计数时要检查是否会变成负数

5. 设计一个日期类（Date）
   要求：
   - 私有成员：年、月、日
   - 公有方法：设置日期（要验证日期是否合法）
   - 公有方法：获取年、月、日
   - 公有方法：显示日期（格式：YYYY-MM-DD）
   提示：要验证月份（1-12）和日期（根据月份不同）
)" << endl;

    return 0;
}

// ============================================
// 编译和运行
// ============================================

/*
编译命令：
g++ -o 37_access_control 37_access_control.cpp

运行命令：
./37_access_control        (Linux/Mac)
37_access_control.exe      (Windows)

C++版本要求：
C++11或更高版本
*/
