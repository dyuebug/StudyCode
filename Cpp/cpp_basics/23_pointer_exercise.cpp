/*
 * ============================================================================
 * 文件名：23_pointer_exercise.cpp
 * 主题：指针练习
 * 学习目标：
 *   1. 用指针交换两个变量
 *   2. 用指针遍历数组
 *   3. 用指针操作字符串
 *   4. 用指针查找数组中的最大值和最小值
 *   5. 用指针实现数组反转
 *
 * 编译命令：g++ -std=c++11 -o 23_pointer_exercise 23_pointer_exercise.cpp
 * 运行命令：./23_pointer_exercise (Linux/Mac) 或 23_pointer_exercise.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <cstring>
using namespace std;

// 函数声明
void swap(int* a, int* b);
void printArray(int* arr, int size);
int findMax(int* arr, int size);
int findMin(int* arr, int size);
void reverseArray(int* arr, int size);
int stringLength(const char* str);
void stringCopy(char* dest, const char* src);
void stringReverse(char* str);

int main() {
    cout << "=== 指针练习 ===" << endl << endl;

    // ========================================
    // 练习1：用指针交换两个变量
    // ========================================
    cout << "【练习1：用指针交换两个变量】" << endl;

    int x = 10, y = 20;
    cout << "交换前：x = " << x << ", y = " << y << endl;

    swap(&x, &y);

    cout << "交换后：x = " << x << ", y = " << y << endl;
    cout << "✓ 通过传递指针，函数可以修改外部变量" << endl;
    cout << endl;

    // ========================================
    // 练习2：用指针遍历数组并求和
    // ========================================
    cout << "【练习2：用指针遍历数组并求和】" << endl;

    int arr[5] = {10, 20, 30, 40, 50};
    cout << "数组元素：";
    printArray(arr, 5);

    // 用指针求和
    int sum = 0;
    int* p = arr;
    for (int i = 0; i < 5; i++) {
        sum += *p;
        p++;
    }

    cout << "数组总和：" << sum << endl;
    cout << "平均值：" << sum / 5.0 << endl;
    cout << endl;

    // ========================================
    // 练习3：用指针查找最大值和最小值
    // ========================================
    cout << "【练习3：用指针查找最大值和最小值】" << endl;

    int numbers[8] = {45, 12, 78, 23, 89, 34, 67, 56};
    cout << "数组元素：";
    printArray(numbers, 8);

    int max = findMax(numbers, 8);
    int min = findMin(numbers, 8);

    cout << "最大值：" << max << endl;
    cout << "最小值：" << min << endl;
    cout << endl;

    // ========================================
    // 练习4：用指针反转数组
    // ========================================
    cout << "【练习4：用指针反转数组】" << endl;

    int data[6] = {1, 2, 3, 4, 5, 6};
    cout << "反转前：";
    printArray(data, 6);

    reverseArray(data, 6);

    cout << "反转后：";
    printArray(data, 6);
    cout << endl;

    // ========================================
    // 练习5：用指针操作字符串
    // ========================================
    cout << "【练习5：用指针操作字符串】" << endl;

    char str1[] = "Hello";
    cout << "字符串：" << str1 << endl;

    // 用指针计算长度
    int len = stringLength(str1);
    cout << "字符串长度：" << len << endl;

    // 用指针复制字符串
    char str2[20];
    stringCopy(str2, str1);
    cout << "复制后的字符串：" << str2 << endl;

    // 用指针反转字符串
    char str3[] = "World";
    cout << "反转前：" << str3 << endl;
    stringReverse(str3);
    cout << "反转后：" << str3 << endl;
    cout << endl;

    // ========================================
    // 练习6：用指针统计字符串中的字符类型
    // ========================================
    cout << "【练习6：用指针统计字符串中的字符类型】" << endl;

    char text[] = "Hello123World!";
    cout << "字符串：" << text << endl;

    int letters = 0, digits = 0, others = 0;
    char* pt = text;

    while (*pt != '\0') {
        if ((*pt >= 'a' && *pt <= 'z') || (*pt >= 'A' && *pt <= 'Z')) {
            letters++;
        } else if (*pt >= '0' && *pt <= '9') {
            digits++;
        } else {
            others++;
        }
        pt++;
    }

    cout << "字母数量：" << letters << endl;
    cout << "数字数量：" << digits << endl;
    cout << "其他字符：" << others << endl;
    cout << endl;

    // ========================================
    // 练习7：用指针查找数组中的特定元素
    // ========================================
    cout << "【练习7：用指针查找数组中的特定元素】" << endl;

    int scores[7] = {85, 92, 78, 95, 88, 76, 90};
    int target = 95;

    cout << "数组元素：";
    printArray(scores, 7);
    cout << "查找目标：" << target << endl;

    int* ps = scores;
    int position = -1;

    for (int i = 0; i < 7; i++) {
        if (*ps == target) {
            position = i;
            break;
        }
        ps++;
    }

    if (position != -1) {
        cout << "找到目标，位置：" << position << endl;
    } else {
        cout << "未找到目标" << endl;
    }
    cout << endl;

    // ========================================
    // 练习8：用指针实现数组元素移位
    // ========================================
    cout << "【练习8：用指针实现数组元素移位】" << endl;

    int shift[5] = {1, 2, 3, 4, 5};
    cout << "移位前：";
    printArray(shift, 5);

    // 右移一位（最后一个元素移到开头）
    int last = shift[4];
    int* pshift = shift + 4;  // 指向最后一个元素

    for (int i = 4; i > 0; i--) {
        *pshift = *(pshift - 1);
        pshift--;
    }
    shift[0] = last;

    cout << "右移后：";
    printArray(shift, 5);
    cout << endl;

    // ========================================
    // 练习9：用指针合并两个数组
    // ========================================
    cout << "【练习9：用指针合并两个数组】" << endl;

    int arr1[3] = {1, 2, 3};
    int arr2[3] = {4, 5, 6};
    int merged[6];

    cout << "数组1：";
    printArray(arr1, 3);
    cout << "数组2：";
    printArray(arr2, 3);

    // 用指针合并
    int* pm = merged;
    int* p1 = arr1;
    int* p2 = arr2;

    for (int i = 0; i < 3; i++) {
        *pm = *p1;
        pm++;
        p1++;
    }

    for (int i = 0; i < 3; i++) {
        *pm = *p2;
        pm++;
        p2++;
    }

    cout << "合并后：";
    printArray(merged, 6);
    cout << endl;

    // ========================================
    // 练习10：指针与动态内存（预告）
    // ========================================
    cout << "【练习10：指针的强大之处】" << endl;
    cout << "指针可以：" << endl;
    cout << "  ✓ 高效遍历数组" << endl;
    cout << "  ✓ 在函数间传递大量数据（不复制）" << endl;
    cout << "  ✓ 实现复杂数据结构（链表、树）" << endl;
    cout << "  ✓ 动态分配内存（后续学习）" << endl;
    cout << endl;

    cout << "⚠️ 使用指针的注意事项：" << endl;
    cout << "  1. 始终初始化指针" << endl;
    cout << "  2. 使用前检查是否为空" << endl;
    cout << "  3. 不要访问越界地址" << endl;
    cout << "  4. 不要使用已释放的指针" << endl;
    cout << endl;

    return 0;
}

// ========================================
// 函数实现
// ========================================

// 交换两个变量
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// 打印数组
void printArray(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        cout << *(arr + i) << " ";
    }
    cout << endl;
}

// 查找最大值
int findMax(int* arr, int size) {
    int max = *arr;  // 假设第一个元素最大
    int* p = arr + 1;

    for (int i = 1; i < size; i++) {
        if (*p > max) {
            max = *p;
        }
        p++;
    }

    return max;
}

// 查找最小值
int findMin(int* arr, int size) {
    int min = *arr;
    int* p = arr + 1;

    for (int i = 1; i < size; i++) {
        if (*p < min) {
            min = *p;
        }
        p++;
    }

    return min;
}

// 反转数组
void reverseArray(int* arr, int size) {
    int* left = arr;
    int* right = arr + size - 1;

    while (left < right) {
        swap(left, right);
        left++;
        right--;
    }
}

// 计算字符串长度
int stringLength(const char* str) {
    int len = 0;
    while (*str != '\0') {
        len++;
        str++;
    }
    return len;
}

// 复制字符串
void stringCopy(char* dest, const char* src) {
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';  // 添加结束符
}

// 反转字符串
void stringReverse(char* str) {
    char* left = str;
    char* right = str;

    // 找到字符串末尾
    while (*right != '\0') {
        right++;
    }
    right--;  // 回退到最后一个字符

    // 交换字符
    while (left < right) {
        char temp = *left;
        *left = *right;
        *right = temp;
        left++;
        right--;
    }
}
