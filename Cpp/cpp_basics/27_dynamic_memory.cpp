/*
 * ============================================================================
 * 文件名：27_dynamic_memory.cpp
 * 主题：动态内存基础详解
 * 学习目标：
 *   1. 理解栈内存和堆内存的区别
 *   2. 掌握new运算符的使用（动态分配内存）
 *   3. 掌握delete运算符的使用（释放内存）
 *   4. 理解内存泄漏的危害
 *   5. 认识悬空指针的危险
 *
 * 编译命令：g++ -std=c++11 -o 27_dynamic_memory 27_dynamic_memory.cpp
 * 运行命令：./27_dynamic_memory (Linux/Mac) 或 27_dynamic_memory.exe (Windows)
 * ============================================================================
 */

#include <iostream>
using namespace std;

int main() {
    cout << "=== 动态内存基础详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是动态内存？
    // ========================================
    cout << "【1. 什么是动态内存？】" << endl;
    cout << "程序的内存分为两大区域：" << endl;
    cout << "  栈（Stack）：自动管理，生命周期由作用域决定" << endl;
    cout << "  堆（Heap）：手动管理，生命周期由程序员控制" << endl;
    cout << endl;

    cout << "为什么需要动态内存？" << endl;
    cout << "  ✓ 运行时才知道需要多少内存" << endl;
    cout << "  ✓ 需要大块内存（栈空间有限）" << endl;
    cout << "  ✓ 需要跨函数使用的数据" << endl;
    cout << "  ✓ 需要灵活控制生命周期" << endl;
    cout << endl;

    // ========================================
    // 2. 栈内存 vs 堆内存
    // ========================================
    cout << "【2. 栈内存 vs 堆内存】" << endl;

    // 栈内存：自动分配和释放
    int stackVar = 10;
    cout << "栈变量：int stackVar = 10;" << endl;
    cout << "stackVar = " << stackVar << endl;
    cout << "地址：" << &stackVar << endl;
    cout << "特点：函数结束时自动释放" << endl;
    cout << endl;

    // 堆内存：手动分配和释放
    int* heapVar = new int(20);
    cout << "堆变量：int* heapVar = new int(20);" << endl;
    cout << "*heapVar = " << *heapVar << endl;
    cout << "地址：" << heapVar << endl;
    cout << "特点：必须手动释放（delete）" << endl;
    delete heapVar;  // 释放内存
    cout << "已释放：delete heapVar;" << endl;
    cout << endl;

    cout << "┌──────────────┬────────────┬────────────┐" << endl;
    cout << "│   特性       │   栈内存   │   堆内存   │" << endl;
    cout << "├──────────────┼────────────┼────────────┤" << endl;
    cout << "│ 分配方式     │   自动     │   手动     │" << endl;
    cout << "│ 释放方式     │   自动     │   手动     │" << endl;
    cout << "│ 大小         │   有限     │   较大     │" << endl;
    cout << "│ 速度         │   快       │   慢       │" << endl;
    cout << "│ 生命周期     │   作用域   │   程序员   │" << endl;
    cout << "└──────────────┴────────────┴────────────┘" << endl;
    cout << endl;

    // ========================================
    // 3. new运算符：分配内存
    // ========================================
    cout << "【3. new运算符：分配内存】" << endl;

    // 分配单个变量
    int* p1 = new int;  // 分配但不初始化
    cout << "int* p1 = new int; → *p1 = " << *p1 << " (未初始化，随机值)" << endl;

    int* p2 = new int(42);  // 分配并初始化
    cout << "int* p2 = new int(42); → *p2 = " << *p2 << endl;

    int* p3 = new int{99};  // C++11初始化方式
    cout << "int* p3 = new int{99}; → *p3 = " << *p3 << endl;
    cout << endl;

    // 分配其他类型
    double* pd = new double(3.14);
    cout << "double* pd = new double(3.14); → *pd = " << *pd << endl;

    char* pc = new char('A');
    cout << "char* pc = new char('A'); → *pc = " << *pc << endl;
    cout << endl;

    // 清理
    delete p1;
    delete p2;
    delete p3;
    delete pd;
    delete pc;

    // ========================================
    // 4. delete运算符：释放内存
    // ========================================
    cout << "【4. delete运算符：释放内存】" << endl;

    int* ptr = new int(100);
    cout << "分配内存：int* ptr = new int(100);" << endl;
    cout << "*ptr = " << *ptr << endl;

    delete ptr;  // 释放内存
    cout << "释放内存：delete ptr;" << endl;
    cout << "内存已归还给系统" << endl;
    cout << endl;

    cout << "重要规则：" << endl;
    cout << "  ✓ 每个new必须对应一个delete" << endl;
    cout << "  ✓ 不要delete同一个指针两次" << endl;
    cout << "  ✓ 不要delete栈变量" << endl;
    cout << "  ✓ delete后建议将指针设为nullptr" << endl;
    cout << endl;

    // ========================================
    // 5. 内存泄漏（Memory Leak）
    // ========================================
    cout << "【5. 内存泄漏（Memory Leak）】" << endl;

    cout << "什么是内存泄漏？" << endl;
    cout << "  分配的内存没有释放，导致内存无法回收" << endl;
    cout << endl;

    cout << "示例1：忘记delete" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  // 忘记 delete p;" << endl;
    cout << "  结果：内存泄漏！" << endl;
    cout << endl;

    cout << "示例2：指针被覆盖" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  p = new int(20);  // 第一块内存丢失！" << endl;
    cout << "  结果：第一块内存泄漏！" << endl;
    cout << endl;

    cout << "示例3：函数返回前未释放" << endl;
    cout << "  void func() {" << endl;
    cout << "      int* p = new int(10);" << endl;
    cout << "      return;  // 忘记delete！" << endl;
    cout << "  }" << endl;
    cout << "  结果：每次调用都泄漏内存！" << endl;
    cout << endl;

    cout << "内存泄漏的危害：" << endl;
    cout << "  ⚠️ 程序占用内存越来越多" << endl;
    cout << "  ⚠️ 系统变慢" << endl;
    cout << "  ⚠️ 最终可能崩溃" << endl;
    cout << endl;

    // ========================================
    // 6. 悬空指针（Dangling Pointer）
    // ========================================
    cout << "【6. 悬空指针（Dangling Pointer）】" << endl;

    int* dangling = new int(50);
    cout << "分配内存：int* dangling = new int(50);" << endl;
    cout << "*dangling = " << *dangling << endl;

    delete dangling;
    cout << "释放内存：delete dangling;" << endl;
    cout << "此时dangling成为悬空指针（指向已释放的内存）" << endl;
    cout << endl;

    cout << "⚠️ 危险操作：" << endl;
    cout << "  *dangling = 60;  // 访问已释放的内存，未定义行为！" << endl;
    cout << "  可能崩溃、可能覆盖其他数据、可能看起来正常" << endl;
    cout << endl;

    cout << "✅ 安全做法：" << endl;
    dangling = nullptr;
    cout << "  delete dangling;" << endl;
    cout << "  dangling = nullptr;  // 设为空指针" << endl;
    cout << "  if (dangling != nullptr) { ... }  // 使用前检查" << endl;
    cout << endl;

    // ========================================
    // 7. 正确的内存管理流程
    // ========================================
    cout << "【7. 正确的内存管理流程】" << endl;

    cout << "步骤1：分配内存" << endl;
    int* safe = new int(77);
    cout << "  int* safe = new int(77);" << endl;
    cout << "  *safe = " << *safe << endl;
    cout << endl;

    cout << "步骤2：使用内存" << endl;
    *safe = 88;
    cout << "  *safe = 88;" << endl;
    cout << "  *safe = " << *safe << endl;
    cout << endl;

    cout << "步骤3：释放内存" << endl;
    delete safe;
    cout << "  delete safe;" << endl;
    cout << endl;

    cout << "步骤4：指针置空" << endl;
    safe = nullptr;
    cout << "  safe = nullptr;" << endl;
    cout << endl;

    cout << "步骤5：使用前检查" << endl;
    if (safe != nullptr) {
        cout << "  指针有效，可以使用" << endl;
    } else {
        cout << "  指针为空，不能使用" << endl;
    }
    cout << endl;

    // ========================================
    // 8. 常见错误
    // ========================================
    cout << "【8. 常见错误】" << endl;

    cout << "❌ 错误1：忘记delete" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  // 程序结束，内存泄漏" << endl;
    cout << endl;

    cout << "❌ 错误2：重复delete" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete p;" << endl;
    cout << "  delete p;  // 崩溃！" << endl;
    cout << endl;

    cout << "❌ 错误3：delete栈变量" << endl;
    cout << "  int x = 10;" << endl;
    cout << "  delete &x;  // 崩溃！" << endl;
    cout << endl;

    cout << "❌ 错误4：使用已释放的内存" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete p;" << endl;
    cout << "  *p = 20;  // 未定义行为！" << endl;
    cout << endl;

    // ========================================
    // 9. 内存管理最佳实践
    // ========================================
    cout << "【9. 内存管理最佳实践】" << endl;

    cout << "✅ 原则1：谁分配谁释放" << endl;
    cout << "  在同一个函数/类中完成new和delete" << endl;
    cout << endl;

    cout << "✅ 原则2：立即初始化" << endl;
    cout << "  int* p = new int(0);  // 不要留未初始化的内存" << endl;
    cout << endl;

    cout << "✅ 原则3：delete后置空" << endl;
    cout << "  delete p;" << endl;
    cout << "  p = nullptr;" << endl;
    cout << endl;

    cout << "✅ 原则4：使用前检查" << endl;
    cout << "  if (p != nullptr) { ... }" << endl;
    cout << endl;

    cout << "✅ 原则5：优先使用栈" << endl;
    cout << "  能用栈就不用堆，栈更安全" << endl;
    cout << endl;

    cout << "✅ 原则6：考虑智能指针" << endl;
    cout << "  C++11后，优先使用智能指针（后续学习）" << endl;
    cout << endl;

    // ========================================
    // 10. 总结
    // ========================================
    cout << "【10. 总结】" << endl;

    cout << "动态内存的核心：" << endl;
    cout << "  new  → 分配内存（堆）" << endl;
    cout << "  delete → 释放内存" << endl;
    cout << "  nullptr → 安全的空指针" << endl;
    cout << endl;

    cout << "两大危险：" << endl;
    cout << "  ⚠️ 内存泄漏：忘记释放" << endl;
    cout << "  ⚠️ 悬空指针：释放后使用" << endl;
    cout << endl;

    cout << "记住：" << endl;
    cout << "  动态内存是强大的工具，但需要谨慎使用！" << endl;
    cout << "  每个new必须对应一个delete！" << endl;
    cout << endl;

    return 0;
}
