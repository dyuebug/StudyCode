/*
 * ============================================================================
 * 文件名：28_dynamic_array.cpp
 * 主题：动态数组详解
 * 学习目标：
 *   1. 掌握new[]动态创建数组
 *   2. 掌握delete[]释放数组
 *   3. 理解动态数组与静态数组的区别
 *   4. 学会创建动态二维数组
 *   5. 认识常见错误（new配delete[]）
 *
 * 编译命令：g++ -std=c++11 -o 28_dynamic_array 28_dynamic_array.cpp
 * 运行命令：./28_dynamic_array (Linux/Mac) 或 28_dynamic_array.exe (Windows)
 * ============================================================================
 */

#include <iostream>
using namespace std;

int main() {
    cout << "=== 动态数组详解 ===" << endl << endl;

    // ========================================
    // 1. 静态数组的局限
    // ========================================
    cout << "【1. 静态数组的局限】" << endl;

    int staticArr[5] = {1, 2, 3, 4, 5};
    cout << "静态数组：int staticArr[5] = {1, 2, 3, 4, 5};" << endl;
    cout << "元素：";
    for (int i = 0; i < 5; i++) {
        cout << staticArr[i] << " ";
    }
    cout << endl << endl;

    cout << "静态数组的局限：" << endl;
    cout << "  ✗ 大小必须在编译时确定" << endl;
    cout << "  ✗ 不能根据用户输入动态调整" << endl;
    cout << "  ✗ 大小固定，不能扩展" << endl;
    cout << "  ✗ 占用栈空间，不能太大" << endl;
    cout << endl;

    // ========================================
    // 2. 动态数组：new[]
    // ========================================
    cout << "【2. 动态数组：new[]】" << endl;

    int size = 5;
    int* dynamicArr = new int[size];  // 动态分配数组

    cout << "动态数组：int* dynamicArr = new int[" << size << "];" << endl;

    // 初始化数组
    for (int i = 0; i < size; i++) {
        dynamicArr[i] = (i + 1) * 10;
    }

    cout << "元素：";
    for (int i = 0; i < size; i++) {
        cout << dynamicArr[i] << " ";
    }
    cout << endl << endl;

    cout << "动态数组的优势：" << endl;
    cout << "  ✓ 大小可以在运行时决定" << endl;
    cout << "  ✓ 可以根据需求分配" << endl;
    cout << "  ✓ 使用堆内存，可以很大" << endl;
    cout << "  ✓ 生命周期灵活控制" << endl;
    cout << endl;

    // 释放动态数组
    delete[] dynamicArr;
    cout << "释放数组：delete[] dynamicArr;" << endl;
    cout << "⚠️ 注意：必须用delete[]，不能用delete" << endl;
    cout << endl;

    // ========================================
    // 3. new[] 的初始化方式
    // ========================================
    cout << "【3. new[] 的初始化方式】" << endl;

    // 方式1：不初始化（随机值）
    int* arr1 = new int[3];
    cout << "方式1：int* arr1 = new int[3]; (不初始化)" << endl;
    cout << "元素：";
    for (int i = 0; i < 3; i++) {
        cout << arr1[i] << " ";
    }
    cout << " (随机值)" << endl;

    // 方式2：值初始化（全部为0）
    int* arr2 = new int[3]();
    cout << "方式2：int* arr2 = new int[3](); (值初始化)" << endl;
    cout << "元素：";
    for (int i = 0; i < 3; i++) {
        cout << arr2[i] << " ";
    }
    cout << " (全部为0)" << endl;

    // 方式3：列表初始化（C++11）
    int* arr3 = new int[3]{10, 20, 30};
    cout << "方式3：int* arr3 = new int[3]{10, 20, 30}; (列表初始化)" << endl;
    cout << "元素：";
    for (int i = 0; i < 3; i++) {
        cout << arr3[i] << " ";
    }
    cout << endl << endl;

    // 清理
    delete[] arr1;
    delete[] arr2;
    delete[] arr3;

    // ========================================
    // 4. 动态数组的使用
    // ========================================
    cout << "【4. 动态数组的使用】" << endl;

    int n = 6;
    int* scores = new int[n];

    cout << "创建动态数组：int* scores = new int[" << n << "];" << endl;

    // 赋值
    for (int i = 0; i < n; i++) {
        scores[i] = 80 + i * 5;
    }

    // 访问
    cout << "成绩：";
    for (int i = 0; i < n; i++) {
        cout << scores[i] << " ";
    }
    cout << endl;

    // 计算平均分
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += scores[i];
    }
    cout << "平均分：" << sum / (double)n << endl;

    // 释放
    delete[] scores;
    cout << "释放：delete[] scores;" << endl;
    cout << endl;

    // ========================================
    // 5. 根据用户输入创建数组
    // ========================================
    cout << "【5. 根据用户输入创建数组】" << endl;

    cout << "请输入数组大小：";
    int userSize;
    cin >> userSize;

    if (userSize <= 0 || userSize > 1000) {
        cout << "无效大小！使用默认值5" << endl;
        userSize = 5;
    }

    int* userArr = new int[userSize];
    cout << "已创建大小为 " << userSize << " 的数组" << endl;

    // 初始化
    for (int i = 0; i < userSize; i++) {
        userArr[i] = i + 1;
    }

    cout << "元素：";
    for (int i = 0; i < userSize; i++) {
        cout << userArr[i] << " ";
    }
    cout << endl;

    delete[] userArr;
    cout << endl;

    // ========================================
    // 6. 动态二维数组
    // ========================================
    cout << "【6. 动态二维数组】" << endl;

    int rows = 3, cols = 4;
    cout << "创建 " << rows << "x" << cols << " 的二维数组" << endl;

    // 方法1：先分配行指针数组
    int** matrix = new int*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new int[cols];  // 为每行分配列
    }

    // 初始化
    int value = 1;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = value++;
        }
    }

    // 打印
    cout << "矩阵：" << endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }
    cout << endl;

    // 释放二维数组（必须先释放每行，再释放行指针数组）
    for (int i = 0; i < rows; i++) {
        delete[] matrix[i];  // 释放每行
    }
    delete[] matrix;  // 释放行指针数组
    cout << "释放二维数组：先delete[]每行，再delete[]行指针" << endl;
    cout << endl;

    // ========================================
    // 7. 动态数组 vs 静态数组
    // ========================================
    cout << "【7. 动态数组 vs 静态数组】" << endl;
    cout << "┌────────────────┬──────────────┬──────────────┐" << endl;
    cout << "│   特性         │   静态数组   │   动态数组   │" << endl;
    cout << "├────────────────┼──────────────┼──────────────┤" << endl;
    cout << "│ 大小确定时机   │   编译时     │   运行时     │" << endl;
    cout << "│ 内存位置       │   栈         │   堆         │" << endl;
    cout << "│ 大小限制       │   较小       │   较大       │" << endl;
    cout << "│ 生命周期       │   自动       │   手动       │" << endl;
    cout << "│ 释放方式       │   自动       │   delete[]   │" << endl;
    cout << "│ 灵活性         │   低         │   高         │" << endl;
    cout << "└────────────────┴──────────────┴──────────────┘" << endl;
    cout << endl;

    // ========================================
    // 8. 常见错误
    // ========================================
    cout << "【8. 常见错误】" << endl;

    cout << "❌ 错误1：new[]配delete（应该用delete[]）" << endl;
    cout << "  int* arr = new int[5];" << endl;
    cout << "  delete arr;  // 错误！应该用delete[]" << endl;
    cout << "  结果：未定义行为，可能内存泄漏" << endl;
    cout << endl;

    cout << "❌ 错误2：delete[]配new（应该用delete）" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete[] p;  // 错误！应该用delete" << endl;
    cout << "  结果：未定义行为" << endl;
    cout << endl;

    cout << "❌ 错误3：忘记释放" << endl;
    cout << "  int* arr = new int[100];" << endl;
    cout << "  // 忘记delete[]" << endl;
    cout << "  结果：内存泄漏" << endl;
    cout << endl;

    cout << "❌ 错误4：数组越界" << endl;
    cout << "  int* arr = new int[5];" << endl;
    cout << "  arr[10] = 100;  // 越界！" << endl;
    cout << "  结果：未定义行为，可能崩溃" << endl;
    cout << endl;

    cout << "❌ 错误5：释放后使用" << endl;
    cout << "  int* arr = new int[5];" << endl;
    cout << "  delete[] arr;" << endl;
    cout << "  arr[0] = 10;  // 使用已释放的内存！" << endl;
    cout << "  结果：未定义行为" << endl;
    cout << endl;

    // ========================================
    // 9. 最佳实践
    // ========================================
    cout << "【9. 最佳实践】" << endl;

    cout << "✅ 规则1：new[]必须配delete[]" << endl;
    cout << "  int* arr = new int[10];" << endl;
    cout << "  delete[] arr;  // 正确" << endl;
    cout << endl;

    cout << "✅ 规则2：new必须配delete" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete p;  // 正确" << endl;
    cout << endl;

    cout << "✅ 规则3：释放后设为nullptr" << endl;
    cout << "  delete[] arr;" << endl;
    cout << "  arr = nullptr;  // 防止悬空指针" << endl;
    cout << endl;

    cout << "✅ 规则4：使用前检查分配是否成功" << endl;
    cout << "  int* arr = new int[size];" << endl;
    cout << "  if (arr == nullptr) { /* 处理错误 */ }" << endl;
    cout << endl;

    cout << "✅ 规则5：记录数组大小" << endl;
    cout << "  int* arr = new int[size];" << endl;
    cout << "  int arrSize = size;  // 保存大小" << endl;
    cout << endl;

    cout << "✅ 规则6：考虑使用vector（C++标准库）" << endl;
    cout << "  #include <vector>" << endl;
    cout << "  vector<int> v(10);  // 自动管理内存" << endl;
    cout << endl;

    // ========================================
    // 10. 总结
    // ========================================
    cout << "【10. 总结】" << endl;

    cout << "动态数组的关键点：" << endl;
    cout << "  1. 使用new[]分配，delete[]释放" << endl;
    cout << "  2. 大小可以在运行时决定" << endl;
    cout << "  3. 使用堆内存，可以很大" << endl;
    cout << "  4. 必须手动管理生命周期" << endl;
    cout << "  5. 注意内存泄漏和越界" << endl;
    cout << endl;

    cout << "⚠️ 重要提醒：" << endl;
    cout << "  现代C++推荐使用vector、array等容器" << endl;
    cout << "  它们自动管理内存，更安全" << endl;
    cout << "  但理解动态数组是理解底层原理的基础" << endl;
    cout << endl;

    return 0;
}
