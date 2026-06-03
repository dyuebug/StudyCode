// ============================================
// 53_operator_overload_exercise.cpp - 超详细注释版
// 日期：第17天
// 难度：⭐⭐ 中级
// 主题：运算符重载综合练习 - Matrix2x2（2×2矩阵）
// ============================================

/*
【本次练习目标】

综合运用第51、52课所学：
- 成员函数运算符重载（+、-、*、==、!=、[]、()）
- 友元函数运算符重载（<<、标量乘法）

项目：实现一个完整的 Matrix2x2（2×2矩阵）类
      支持矩阵加减、矩阵乘法、标量乘法、元素访问、格式化输出

数学背景（2×2矩阵）：
    |a b|
    |c d|

    加法：对应元素相加
    乘法：行列内积
    |a b|   |e f|   |ae+bg  af+bh|
    |c d| × |g h| = |ce+dg  cf+dh|
*/

#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace std;

// ============================================
// 第一部分：Matrix2x2 类定义与运算符实现 ⭐⭐⭐⭐⭐
// ============================================

class Matrix2x2 {
private:
    // 存储矩阵元素：m[行][列]，下标均为 0 或 1
    double m[2][2];

public:
    // ---- 构造函数 ----
    // 参数：a,b,c,d — 按行排列的四个元素
    //   |a b|
    //   |c d|
    // 默认构造：零矩阵
    Matrix2x2(double a = 0, double b = 0,
              double c = 0, double d = 0) {
        m[0][0] = a; m[0][1] = b;
        m[1][0] = c; m[1][1] = d;
    }

    // --------------------------------------------
    // operator+ — 矩阵加法（成员函数）
    // --------------------------------------------
    // 数学：结果矩阵的每个元素 = 对应位置元素之和
    // 参数：rhs — 右操作数矩阵（const 引用，不修改）
    // 返回：新矩阵（不修改 *this）
    // const：加法不应修改左操作数
    Matrix2x2 operator+(const Matrix2x2& rhs) const {
        return Matrix2x2(
            m[0][0] + rhs.m[0][0],  // 第0行第0列相加
            m[0][1] + rhs.m[0][1],  // 第0行第1列相加
            m[1][0] + rhs.m[1][0],  // 第1行第0列相加
            m[1][1] + rhs.m[1][1]   // 第1行第1列相加
        );
    }

    // --------------------------------------------
    // operator- — 矩阵减法（成员函数）
    // --------------------------------------------
    // 数学：对应元素相减
    Matrix2x2 operator-(const Matrix2x2& rhs) const {
        return Matrix2x2(
            m[0][0] - rhs.m[0][0],
            m[0][1] - rhs.m[0][1],
            m[1][0] - rhs.m[1][0],
            m[1][1] - rhs.m[1][1]
        );
    }

    // --------------------------------------------
    // operator* — 矩阵乘法（成员函数）
    // --------------------------------------------
    // 数学：行乘以列求和
    // 结果[i][j] = 左矩阵第i行 · 右矩阵第j列（点积）
    // 注意：矩阵乘法不满足交换律（A*B ≠ B*A）
    Matrix2x2 operator*(const Matrix2x2& rhs) const {
        return Matrix2x2(
            // 第0行 × 第0列：a*e + b*g
            m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0],
            // 第0行 × 第1列：a*f + b*h
            m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1],
            // 第1行 × 第0列：c*e + d*g
            m[1][0]*rhs.m[0][0] + m[1][1]*rhs.m[1][0],
            // 第1行 × 第1列：c*f + d*h
            m[1][0]*rhs.m[0][1] + m[1][1]*rhs.m[1][1]
        );
    }

    // --------------------------------------------
    // operator* — 标量乘法：矩阵 * 数（成员函数）
    // --------------------------------------------
    // 参数：scalar — 标量（每个元素都乘以它）
    // 返回：新矩阵
    // 注意：只支持 m * 2.0，不支持 2.0 * m（见友元版本）
    Matrix2x2 operator*(double scalar) const {
        return Matrix2x2(
            m[0][0]*scalar, m[0][1]*scalar,
            m[1][0]*scalar, m[1][1]*scalar
        );
    }

    // --------------------------------------------
    // operator== — 矩阵相等（成员函数）
    // --------------------------------------------
    // 注意：浮点数比较用差值 < epsilon，而非直接 ==
    bool operator==(const Matrix2x2& rhs) const {
        const double eps = 1e-9;  // 浮点比较容差
        return fabs(m[0][0]-rhs.m[0][0]) < eps &&
               fabs(m[0][1]-rhs.m[0][1]) < eps &&
               fabs(m[1][0]-rhs.m[1][0]) < eps &&
               fabs(m[1][1]-rhs.m[1][1]) < eps;
    }

    // --------------------------------------------
    // operator!= — 矩阵不等（复用 ==）
    // --------------------------------------------
    bool operator!=(const Matrix2x2& rhs) const {
        return !(*this == rhs);  // DRY：复用已实现的 ==
    }

    // --------------------------------------------
    // operator[] — 下标访问（返回引用，支持读写）
    // --------------------------------------------
    // 参数：row — 行号（0 或 1）
    // 返回：double* 指针，指向该行首元素
    // 用法：m[0][1] 先用 [] 取第0行指针，再用原生 [] 取第1列
    // 注意：必须返回引用/指针，才能支持 m[0][1] = 5.0 赋值
    double* operator[](int row) {
        if (row < 0 || row > 1)
            throw out_of_range("行下标只能是0或1");
        return m[row];  // 返回指向该行数组的指针
    }

    // const 版本：用于 const 对象的只读访问
    const double* operator[](int row) const {
        if (row < 0 || row > 1)
            throw out_of_range("行下标只能是0或1");
        return m[row];
    }

    // --------------------------------------------
    // operator() — 函数调用运算符（按行列获取元素）
    // --------------------------------------------
    // 参数：row, col — 行号和列号（0 或 1）
    // 返回：引用，支持 mat(0,1) = 3.0 赋值
    // 与 [] 的区别：() 直接指定行列，语义更清晰
    double& operator()(int row, int col) {
        if (row < 0 || row > 1 || col < 0 || col > 1)
            throw out_of_range("下标只能是0或1");
        return m[row][col];
    }

    const double& operator()(int row, int col) const {
        if (row < 0 || row > 1 || col < 0 || col > 1)
            throw out_of_range("下标只能是0或1");
        return m[row][col];
    }

    // --------------------------------------------
    // operator+= — 矩阵复合加法（成员函数）
    // --------------------------------------------
    // 返回：*this 引用（支持链式）
    Matrix2x2& operator+=(const Matrix2x2& rhs) {
        m[0][0] += rhs.m[0][0]; m[0][1] += rhs.m[0][1];
        m[1][0] += rhs.m[1][0]; m[1][1] += rhs.m[1][1];
        return *this;
    }

    // --------------------------------------------
    // 友元声明：允许非成员函数访问 private 成员
    // --------------------------------------------
    // 注意：friend 声明放在 public 区域，位置与访问权限无关
    friend ostream& operator<<(ostream& out, const Matrix2x2& mat);
    friend Matrix2x2 operator*(double scalar, const Matrix2x2& mat);
};

// ============================================
// 友元非成员函数实现
// ============================================

// --------------------------------------------
// operator<< — 格式化输出矩阵（友元）
// --------------------------------------------
// 参数：
//   out — 输出流（必须是引用，不能是值）
//   mat — 要输出的矩阵（const 引用，不修改）
// 返回：out 引用（支持链式：cout << a << b）
// 必须是友元：左操作数是 ostream，不是 Matrix2x2
ostream& operator<<(ostream& out, const Matrix2x2& mat) {
    out << "|" << mat.m[0][0] << "\t" << mat.m[0][1] << "|\n";
    out << "|" << mat.m[1][0] << "\t" << mat.m[1][1] << "|";
    return out;  // 返回 out 引用，支持链式输出
}

// --------------------------------------------
// operator* — 标量乘法：数 * 矩阵（友元）
// --------------------------------------------
// 解决"2.0 * mat"的问题：
//   mat * 2.0 → 可以用成员函数（mat 是 this）
//   2.0 * mat → 左操作数是 double，不能用成员函数
// 参数：scalar — 左边的标量，mat — 右边的矩阵
Matrix2x2 operator*(double scalar, const Matrix2x2& mat) {
    return mat * scalar;  // 复用成员函数 operator*(double)
}

// ============================================
// 第二部分：演示与测试
// ============================================

void demoArithmetic() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：矩阵算术运算" << endl;
    cout << string(60, '=') << endl;

    // 构造两个矩阵
    // A = |1 2|    B = |5 6|
    //     |3 4|        |7 8|
    Matrix2x2 A(1, 2, 3, 4);
    Matrix2x2 B(5, 6, 7, 8);

    cout << "\nA =\n" << A << endl;
    cout << "\nB =\n" << B << endl;

    cout << "\n--- 矩阵加法 A + B ---" << endl;
    Matrix2x2 sumAB = A + B;
    cout << sumAB << endl;
    // 期望：|6  8 |
    //       |10 12|

    cout << "\n--- 矩阵减法 B - A ---" << endl;
    cout << (B - A) << endl;
    // 期望：|4 4|
    //       |4 4|

    cout << "\n--- 矩阵乘法 A * B ---" << endl;
    Matrix2x2 prodAB = A * B;
    cout << prodAB << endl;
    // 计算验证：
    // (0,0) = 1*5 + 2*7 = 19
    // (0,1) = 1*6 + 2*8 = 22
    // (1,0) = 3*5 + 4*7 = 43
    // (1,1) = 3*6 + 4*8 = 50
}

void demoScalarAndAccess() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：标量乘法与元素访问" << endl;
    cout << string(60, '=') << endl;

    Matrix2x2 M(1, 2, 3, 4);
    cout << "M =\n" << M << endl;

    cout << "\n--- 标量乘法 M * 3.0 ---" << endl;
    cout << (M * 3.0) << endl;

    cout << "\n--- 标量乘法 2.0 * M（友元版）---" << endl;
    cout << (2.0 * M) << endl;

    cout << "\n--- 下标访问 M[0][1] ---" << endl;
    cout << "M[0][1] = " << M[0][1] << endl;  // 输出 2

    cout << "\n--- 修改元素：M[1][0] = 99 ---" << endl;
    M[1][0] = 99;  // operator[] 返回引用，支持赋值
    cout << "修改后 M =\n" << M << endl;

    cout << "\n--- 函数调用访问 M(0,0) ---" << endl;
    cout << "M(0,0) = " << M(0,0) << endl;

    cout << "\n--- 修改元素：M(1,1) = 77 ---" << endl;
    M(1,1) = 77;
    cout << "修改后 M =\n" << M << endl;
}

void demoCompare() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：比较与复合赋值" << endl;
    cout << string(60, '=') << endl;

    Matrix2x2 A(1, 2, 3, 4);
    Matrix2x2 B(1, 2, 3, 4);
    Matrix2x2 C(0, 0, 0, 0);

    cout << "A == B: " << (A == B ? "true" : "false") << endl;  // true
    cout << "A != C: " << (A != C ? "true" : "false") << endl;  // true

    cout << "\n--- 复合赋值 C += A ---" << endl;
    cout << "C 初始 =\n" << C << endl;
    C += A;
    cout << "C += A 后 =\n" << C << endl;
}

// ============================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：矩阵乘法顺序（行×列）写反

   ✗ 错误：
   return Matrix2x2(
       m[0][0]*rhs.m[0][0] + m[1][0]*rhs.m[0][1],  // 把列当行了
       ...
   );

   ✅ 正确：结果[i][j] = 左矩阵第i行 · 右矩阵第j列
   return Matrix2x2(
       m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0],  // 第0行 · 第0列
       m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1],  // 第0行 · 第1列
       ...
   );

------------------------------------------------------------

❌ 错误2：operator[] 返回值而非引用/指针

   ✗ 错误：
   double operator[](int row) { return m[row][0]; }
   // m[0][1] = 5.0;  ← 编译报错！不能对临时值赋值

   ✅ 正确：返回指针或引用，支持二维下标和赋值
   double* operator[](int row) { return m[row]; }
   // m[0][1] = 5.0;  ← 正确，先取第0行指针再取第1列

------------------------------------------------------------

❌ 错误3：标量乘法只实现成员函数，导致 2.0 * mat 失败

   ✗ 只有：Matrix2x2 operator*(double s) const { ... }
   Matrix2x2 m(1,2,3,4);
   auto r1 = m * 2.0;   // ✅ 正常
   auto r2 = 2.0 * m;   // ❌ 编译错误！double 没有 operator*(Matrix2x2)

   ✅ 补充友元非成员函数：
   friend Matrix2x2 operator*(double s, const Matrix2x2& mat) {
       return mat * s;  // 复用已有成员函数
   }

------------------------------------------------------------

❌ 错误4：浮点数用 == 直接比较

   ✗ 错误：
   bool operator==(const Matrix2x2& rhs) const {
       return m[0][0] == rhs.m[0][0] && ...;  // 浮点精度问题！
   }
   // 0.1 + 0.2 == 0.3 → false（浮点误差）

   ✅ 正确：用差值绝对值 < epsilon
   const double eps = 1e-9;
   return fabs(m[0][0] - rhs.m[0][0]) < eps && ...;

------------------------------------------------------------

❌ 错误5：operator() 没有 const 重载

   ✗ 只有：double& operator()(int r, int c) { return m[r][c]; }
   const Matrix2x2 cm(1,2,3,4);
   double v = cm(0,1);  // ❌ const 对象调用非 const 方法，编译错误

   ✅ 同时提供 const 和非 const 版本：
   double& operator()(int r, int c) { return m[r][c]; }
   const double& operator()(int r, int c) const { return m[r][c]; }

------------------------------------------------------------

❌ 错误6：矩阵乘法与标量乘法用同名 operator* 混淆

   ✗ 误写：Matrix2x2 operator*(const Matrix2x2& rhs) const { ... }
   然后忘记写 scalar 版本，导致 m * 2.0 调用矩阵乘法（编译错误）

   ✅ C++ 重载解析：根据参数类型自动选择
   operator*(const Matrix2x2& rhs)  // 矩阵参数 → 矩阵乘法
   operator*(double scalar)          // double 参数 → 标量乘法
   两个可以并存，不冲突

)";
}

// ============================================
// 第四部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【矩阵算术运算符模板】

矩阵加法（成员，const，返回新对象）：
    Matrix operator+(const Matrix& rhs) const {
        return Matrix(各元素之和);
    }

矩阵乘法（成员，const，返回新对象）：
    Matrix operator*(const Matrix& rhs) const {
        // result[i][j] = 左矩阵第i行 · 右矩阵第j列
    }

标量乘法两个方向（成员 + 友元）：
    Matrix operator*(double s) const { ... }          // m * 2.0
    friend Matrix operator*(double s, const Matrix& m) // 2.0 * m
    { return m * s; }

【下标与函数调用访问模板】

双版本 operator[]（支持读写 + const 对象）：
    double* operator[](int i) { return data[i]; }
    const double* operator[](int i) const { return data[i]; }

双版本 operator()：
    double& operator()(int r, int c) { return data[r][c]; }
    const double& operator()(int r, int c) const { return data[r][c]; }

【流输出（必须是友元非成员）】
    friend ostream& operator<<(ostream& out, const Matrix& m) {
        // 格式化输出
        return out;  // 必须返回 out 引用
    }
)";
}

// ============================================
// 第五部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 扩展题：为 Matrix2x2 添加以下功能

   a) double det() const — 计算行列式：ad - bc
   b) Matrix2x2 transpose() const — 转置矩阵（行变列）
   c) operator- （一元负号）：返回每个元素取负的矩阵
      使用：Matrix2x2 neg = -mat;

   验证：
   Matrix2x2 A(1,2,3,4);
   cout << A.det()       << endl;  // -2
   cout << A.transpose() << endl;  // |1 3|
                                   // |2 4|
   cout << -A            << endl;  // |-1 -2|
                                   // |-3 -4|

------------------------------------------------------------

2. 实现 Matrix3x3（3×3矩阵）类

   挑战：
   - 存储：double m[3][3]
   - 实现 operator+、operator*（3×3矩阵乘法）
   - 实现 operator[]（返回 double*）
   - 实现 operator<<（格式化输出3行）

   提示：矩阵乘法需要三重循环（i, j, k）

------------------------------------------------------------

3. 思考题：为 Matrix2x2 实现 operator/ 是否合理？

   a) 实现 operator/(double scalar)（每个元素除以标量）
      矩阵 / 2.0 = 每个元素 / 2

   b) 实现 operator/(const Matrix2x2& rhs) 是否合理？
      数学中"矩阵除法"等价于 A * B^(-1)（乘以逆矩阵）
      请先实现 inverse() 方法（当行列式不为零时），再考虑 /

   提示：inverse() 使用行列式：
   |a b|^-1 = (1/det) * | d -b|
   |c d|                |-c  a|
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 53_operator_overload_exercise.cpp - 矩阵运算符综合练习" << endl;
    cout << string(60, '=') << endl;

    demoArithmetic();
    demoScalarAndAccess();
    demoCompare();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 矩阵乘法：结果[i][j] = 左矩阵第i行 · 右矩阵第j列（行×列）" << endl;
    cout << "2. 标量乘法两方向：成员函数处理 m*s，友元处理 s*m" << endl;
    cout << "3. operator[]：返回指针，支持二维下标和赋值" << endl;
    cout << "4. 浮点比较：用 fabs(a-b) < epsilon，不用直接 ==" << endl;
    cout << "5. 每个运算符都要提供 const 和非 const 两个版本" << endl;

    return 0;
}
