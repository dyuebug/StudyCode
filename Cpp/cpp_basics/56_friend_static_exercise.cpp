// 56_friend_static_exercise.cpp
// friend + static 综合练习：账户类中的特权访问与共享数据

#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 第一部分：题目背景
// ============================================================
// 设计一个 BankAccount 类：
// 1. 每个账户有自己的 owner 和 balance（普通成员）
// 2. 整个类共享 totalAccounts 和 annualInterestRate（static 成员）
// 3. 让友元函数和友元类在合理场景下访问这些 private 数据

class BankAccount;
class AuditTool;

// 友元函数前置声明
void printAccountReport(const BankAccount& account);

class BankAccount {
private:
    string owner;               // 每个对象自己的数据
    double balance;             // 每个对象自己的数据
    static int totalAccounts;   // 整个类共享：账户总数
    static double annualInterestRate; // 整个类共享：年利率

public:
    BankAccount(const string& owner, double balance)
        : owner(owner), balance(balance) {
        totalAccounts++;
    }

    void deposit(double amount) {
        if (amount > 0) {
            balance += amount;
        }
    }

    void showBasicInfo() const {
        cout << "账户持有人：" << owner << "，当前余额：" << balance << endl;
    }

    static void showClassInfo() {
        cout << "账户总数：" << totalAccounts << endl;
        cout << "当前统一年利率：" << annualInterestRate << endl;
    }

    // 友元函数：允许类外函数访问对象私有数据和类私有静态数据
    friend void printAccountReport(const BankAccount& account);

    // 友元类：允许整个审计工具类访问私有成员
    friend class AuditTool;
};

int BankAccount::totalAccounts = 0;
double BankAccount::annualInterestRate = 0.035;

// ============================================================
// 第二部分：friend 函数访问普通成员与 static 成员
// ============================================================
// 这个函数不是成员函数，但因为被授权为 friend，
// 所以它可以同时访问 private 的 balance 和 private static 的 annualInterestRate

void printAccountReport(const BankAccount& account) {
    double estimatedInterest = account.balance * BankAccount::annualInterestRate;

    cout << "[账户报告]" << endl;
    cout << "持有人：" << account.owner << endl;
    cout << "余额：" << account.balance << endl;
    cout << "年利率：" << BankAccount::annualInterestRate << endl;
    cout << "预计一年利息：" << estimatedInterest << endl;
}

void demoFriendFunction() {
    BankAccount account("Alice", 10000.0);
    account.deposit(2000.0);
    account.showBasicInfo();

    cout << "通过友元函数生成账户报告：" << endl;
    printAccountReport(account);
}

// ============================================================
// 第三部分：friend 类访问普通成员与 static 成员
// ============================================================
// AuditTool 被声明为 friend class，
// 所以它的成员函数可以查看账户对象的私有成员，也可以访问类的私有 static 成员

class AuditTool {
public:
    void inspect(const BankAccount& account) const {
        cout << "[审计工具]" << endl;
        cout << "持有人：" << account.owner << endl;
        cout << "余额：" << account.balance << endl;
        cout << "全系统账户总数：" << BankAccount::totalAccounts << endl;
        cout << "当前年利率：" << BankAccount::annualInterestRate << endl;
    }

    void explainDesign() const {
        cout << "1. owner 和 balance 属于具体账户对象" << endl;
        cout << "2. totalAccounts 和 annualInterestRate 属于整个 BankAccount 类" << endl;
        cout << "3. friend 让审计工具在特定场景下突破封装" << endl;
        cout << "4. 但这种权限很大，所以实际设计中要慎用" << endl;
    }
};

void demoFriendClass() {
    BankAccount account1("Bob", 5000.0);
    BankAccount account2("Cindy", 8000.0);
    AuditTool auditTool;

    auditTool.inspect(account1);
    cout << endl;
    auditTool.inspect(account2);
    cout << endl;
    auditTool.explainDesign();
}

// ============================================================
// 第四部分：扩展练习题
// ============================================================

void printExercises() {
    cout << "\n=== 练习题 ===" << endl;
    cout << "1. 为 BankAccount 增加 withdraw()，并考虑余额不足时如何处理" << endl;
    cout << "2. 增加一个 static 成员表示银行名称，并输出到报告中" << endl;
    cout << "3. 思考：AuditTool 一定要做成 friend class 吗？能否改成 getter + 普通类？" << endl;
    cout << "4. 如果只允许某个函数查余额，而不允许查看 owner，应该如何缩小授权范围？" << endl;
    cout << "5. 思考：annualInterestRate 为什么适合做成 static，而 balance 不适合？" << endl;
}

// ============================================================
// 第五部分：学习提示
// ============================================================

void printHints() {
    cout << "1. 普通成员描述对象自己的状态，例如 owner、balance" << endl;
    cout << "2. static 成员描述全类共享的状态，例如 totalAccounts、年利率" << endl;
    cout << "3. friend 可以访问 private 普通成员，也可以访问 private static 成员" << endl;
    cout << "4. 友元权限越大，越要谨慎，避免破坏封装边界" << endl;
    cout << "5. 设计类时要先分清：谁是对象数据，谁是共享数据" << endl;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    cout << "========================================" << endl;
    cout << "   friend + static 综合练习" << endl;
    cout << "========================================" << endl;

    cout << "\n=== 第一部分：类共享信息 ===" << endl;
    BankAccount::showClassInfo();

    cout << "\n=== 第二部分：friend 函数生成报告 ===" << endl;
    demoFriendFunction();

    cout << "\n=== 第三部分：friend 类进行审计 ===" << endl;
    demoFriendClass();

    cout << "\n=== 第四部分：再次查看类共享信息 ===" << endl;
    BankAccount::showClassInfo();

    cout << "\n=== 第五部分：学习提示 ===" << endl;
    printHints();

    printExercises();

    cout << "\n=== 核心总结 ===" << endl;
    cout << "1. 对象自己的数据适合做普通成员，共享数据适合做 static 成员" << endl;
    cout << "2. friend 函数和 friend 类都可以访问 private 数据" << endl;
    cout << "3. friend 不只是能访问普通私有成员，也能访问私有 static 成员" << endl;
    cout << "4. static 体现的是“全类共享”，friend 体现的是“特权访问”" << endl;
    cout << "5. 真正重要的不是语法，而是类设计边界是否合理" << endl;

    return 0;
}
