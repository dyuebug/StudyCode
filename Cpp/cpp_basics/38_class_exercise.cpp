// ============================================
// 类与对象练习 - 超详细注释版
// 日期：2026-03-17（第11天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】类的设计与实践

是什么？
- 通过实际案例学习如何设计和实现类
- 培养面向对象思维

为什么需要？
- 理论结合实践，加深理解
- 学习类的设计方法和最佳实践
- 培养解决实际问题的能力

核心特点：
- 从需求出发设计类
- 合理划分成员变量和成员函数
- 遵循封装原则
*/

// ============================================
// 第一部分：学生管理系统
// ============================================

#include <iostream>
#include <string>
using namespace std;

// 定义学生类
class Student {
private:
    // 私有成员变量：学生的基本信息
    string id;          // 学号
    string name;        // 姓名
    int age;            // 年龄
    double mathScore;   // 数学成绩
    double englishScore; // 英语成绩
    double cppScore;    // C++成绩

public:
    // 初始化方法
    // 参数：学号、姓名、年龄
    void init(string studentId, string studentName, int studentAge) {
        id = studentId;
        name = studentName;
        age = studentAge;
        // 成绩初始化为0
        mathScore = 0.0;
        englishScore = 0.0;
        cppScore = 0.0;
    }

    // getter方法：获取学号
    string getId() {
        return id;
    }

    // getter方法：获取姓名
    string getName() {
        return name;
    }

    // getter方法：获取年龄
    int getAge() {
        return age;
    }

    // setter方法：设置数学成绩
    // 参数：score - 成绩（0-100）
    // 返回：bool - 是否设置成功
    bool setMathScore(double score) {
        // 数据验证：成绩必须在0-100之间
        if (score < 0 || score > 100) {
            cout << "错误：成绩必须在0-100之间" << endl;
            return false;
        }
        mathScore = score;
        return true;
    }

    // setter方法：设置英语成绩
    bool setEnglishScore(double score) {
        if (score < 0 || score > 100) {
            cout << "错误：成绩必须在0-100之间" << endl;
            return false;
        }
        englishScore = score;
        return true;
    }

    // setter方法：设置C++成绩
    bool setCppScore(double score) {
        if (score < 0 || score > 100) {
            cout << "错误：成绩必须在0-100之间" << endl;
            return false;
        }
        cppScore = score;
        return true;
    }

    // 计算平均分
    // 返回：double - 平均分
    double getAverageScore() {
        return (mathScore + englishScore + cppScore) / 3.0;
    }

    // 判断是否全部及格
    // 返回：bool - true表示全部及格
    bool isAllPassed() {
        return mathScore >= 60 && englishScore >= 60 && cppScore >= 60;
    }

    // 获取等级
    // 返回：string - 等级（优秀/良好/中等/及格/不及格）
    string getGrade() {
        double avg = getAverageScore();
        if (avg >= 90) return "优秀";
        if (avg >= 80) return "良好";
        if (avg >= 70) return "中等";
        if (avg >= 60) return "及格";
        return "不及格";
    }

    // 显示学生信息
    void display() {
        cout << string(50, '-') << endl;
        cout << "学号：" << id << endl;
        cout << "姓名：" << name << endl;
        cout << "年龄：" << age << endl;
        cout << "数学成绩：" << mathScore << endl;
        cout << "英语成绩：" << englishScore << endl;
        cout << "C++成绩：" << cppScore << endl;
        cout << "平均分：" << getAverageScore() << endl;
        cout << "等级：" << getGrade() << endl;
        cout << "是否全部及格：" << (isAllPassed() ? "是" : "否") << endl;
        cout << string(50, '-') << endl;
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：学生管理系统" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 创建学生对象
    // --------------------------------------------

    cout << "\n1.1 创建学生" << endl;

    Student stu1;
    stu1.init("2024001", "张三", 18);

    // 设置成绩
    stu1.setMathScore(85.5);
    stu1.setEnglishScore(92.0);
    stu1.setCppScore(88.5);

    cout << "\n学生1信息：" << endl;
    stu1.display();

    // --------------------------------------------
    // 1.2 创建多个学生
    // --------------------------------------------

    cout << "\n1.2 创建多个学生" << endl;

    Student stu2;
    stu2.init("2024002", "李四", 19);
    stu2.setMathScore(75.0);
    stu2.setEnglishScore(68.5);
    stu2.setCppScore(82.0);

    cout << "\n学生2信息：" << endl;
    stu2.display();

    Student stu3;
    stu3.init("2024003", "王五", 18);
    stu3.setMathScore(55.0);
    stu3.setEnglishScore(48.5);
    stu3.setCppScore(62.0);

    cout << "\n学生3信息：" << endl;
    stu3.display();

    // --------------------------------------------
    // 1.3 数据验证
    // --------------------------------------------

    cout << "\n1.3 数据验证" << endl;

    Student stu4;
    stu4.init("2024004", "赵六", 20);

    cout << "\n尝试设置无效成绩：" << endl;
    stu4.setMathScore(105);  // 超过100，设置失败
    stu4.setEnglishScore(-10);  // 小于0，设置失败

    cout << "\n设置有效成绩：" << endl;
    stu4.setMathScore(90);
    stu4.setEnglishScore(85);
    stu4.setCppScore(95);

    cout << "\n学生4信息：" << endl;
    stu4.display();

    return 0;
}

// ============================================
// 第二部分：图书管理系统
// ============================================

#include <iostream>
#include <string>
using namespace std;

// 定义图书类
class Book {
private:
    // 私有成员变量：图书的基本信息
    string isbn;        // ISBN号
    string title;       // 书名
    string author;      // 作者
    double price;       // 价格
    int stock;          // 库存数量
    bool isBorrowed;    // 是否被借出

public:
    // 初始化方法
    void init(string bookIsbn, string bookTitle, string bookAuthor, double bookPrice, int bookStock) {
        isbn = bookIsbn;
        title = bookTitle;
        author = bookAuthor;
        price = bookPrice;
        stock = bookStock;
        isBorrowed = false;  // 初始未被借出
    }

    // getter方法
    string getIsbn() { return isbn; }
    string getTitle() { return title; }
    string getAuthor() { return author; }
    double getPrice() { return price; }
    int getStock() { return stock; }
    bool getIsBorrowed() { return isBorrowed; }

    // 借书方法
    // 返回：bool - 是否借书成功
    bool borrow() {
        // 检查是否有库存
        if (stock <= 0) {
            cout << "借书失败：库存不足" << endl;
            return false;
        }

        // 检查是否已被借出
        if (isBorrowed) {
            cout << "借书失败：该书已被借出" << endl;
            return false;
        }

        // 借书成功
        stock--;  // 库存减1
        isBorrowed = true;  // 标记为已借出
        cout << "借书成功：《" << title << "》" << endl;
        return true;
    }

    // 还书方法
    // 返回：bool - 是否还书成功
    bool returnBook() {
        // 检查是否已借出
        if (!isBorrowed) {
            cout << "还书失败：该书未被借出" << endl;
            return false;
        }

        // 还书成功
        stock++;  // 库存加1
        isBorrowed = false;  // 标记为未借出
        cout << "还书成功：《" << title << "》" << endl;
        return true;
    }

    // 增加库存
    // 参数：count - 增加的数量
    void addStock(int count) {
        if (count > 0) {
            stock += count;
            cout << "增加库存成功：+" << count << "本" << endl;
        }
    }

    // 打折
    // 参数：discount - 折扣（0.1-1.0，例如0.8表示8折）
    bool applyDiscount(double discount) {
        if (discount < 0.1 || discount > 1.0) {
            cout << "错误：折扣必须在0.1-1.0之间" << endl;
            return false;
        }
        price *= discount;
        cout << "打折成功：" << (discount * 100) << "折" << endl;
        return true;
    }

    // 显示图书信息
    void display() {
        cout << string(50, '-') << endl;
        cout << "ISBN：" << isbn << endl;
        cout << "书名：" << title << endl;
        cout << "作者：" << author << endl;
        cout << "价格：" << price << "元" << endl;
        cout << "库存：" << stock << "本" << endl;
        cout << "状态：" << (isBorrowed ? "已借出" : "可借阅") << endl;
        cout << string(50, '-') << endl;
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第二部分：图书管理系统" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 创建图书对象
    // --------------------------------------------

    cout << "\n2.1 创建图书" << endl;

    Book book1;
    book1.init("978-7-115-12345-6", "C++ Primer", "Stanley B. Lippman", 128.0, 5);

    cout << "\n图书1信息：" << endl;
    book1.display();

    // --------------------------------------------
    // 2.2 借书和还书
    // --------------------------------------------

    cout << "\n2.2 借书和还书" << endl;

    cout << "\n第1次借书：" << endl;
    book1.borrow();
    book1.display();

    cout << "\n尝试再次借书（应该失败）：" << endl;
    book1.borrow();

    cout << "\n还书：" << endl;
    book1.returnBook();
    book1.display();

    cout << "\n第2次借书：" << endl;
    book1.borrow();
    book1.display();

    // --------------------------------------------
    // 2.3 库存管理
    // --------------------------------------------

    cout << "\n2.3 库存管理" << endl;

    Book book2;
    book2.init("978-7-115-54321-0", "Effective C++", "Scott Meyers", 89.0, 3);

    cout << "\n图书2信息：" << endl;
    book2.display();

    cout << "\n增加库存：" << endl;
    book2.addStock(10);
    book2.display();

    // --------------------------------------------
    // 2.4 价格管理
    // --------------------------------------------

    cout << "\n2.4 价格管理" << endl;

    cout << "\n原价：" << book2.getPrice() << "元" << endl;

    cout << "\n打8折：" << endl;
    book2.applyDiscount(0.8);
    book2.display();

    cout << "\n尝试无效折扣：" << endl;
    book2.applyDiscount(1.5);  // 超过1.0，失败

    return 0;
}

// ============================================
// 第三部分：综合练习
// ============================================

#include <iostream>
#include <string>
using namespace std;

// 定义计数器类
class Counter {
private:
    int count;      // 计数值
    int step;       // 步长
    int maxValue;   // 最大值
    int minValue;   // 最小值

public:
    // 初始化
    void init(int initialValue = 0, int counterStep = 1, int max = 100, int min = 0) {
        count = initialValue;
        step = counterStep;
        maxValue = max;
        minValue = min;
    }

    // 增加计数
    void increment() {
        if (count + step <= maxValue) {
            count += step;
            cout << "计数增加：" << count << endl;
        } else {
            cout << "已达到最大值：" << maxValue << endl;
        }
    }

    // 减少计数
    void decrement() {
        if (count - step >= minValue) {
            count -= step;
            cout << "计数减少：" << count << endl;
        } else {
            cout << "已达到最小值：" << minValue << endl;
        }
    }

    // 重置计数
    void reset() {
        count = 0;
        cout << "计数已重置" << endl;
    }

    // 获取当前计数
    int getCount() {
        return count;
    }

    // 显示信息
    void display() {
        cout << "当前计数：" << count << endl;
        cout << "步长：" << step << endl;
        cout << "范围：[" << minValue << ", " << maxValue << "]" << endl;
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第三部分：综合练习" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 计数器练习
    // --------------------------------------------

    cout << "\n3.1 计数器练习" << endl;

    Counter counter;
    counter.init(0, 5, 50, 0);  // 初始值0，步长5，最大值50，最小值0

    cout << "\n初始状态：" << endl;
    counter.display();

    cout << "\n增加计数：" << endl;
    counter.increment();  // 5
    counter.increment();  // 10
    counter.increment();  // 15

    cout << "\n当前状态：" << endl;
    counter.display();

    cout << "\n减少计数：" << endl;
    counter.decrement();  // 10
    counter.decrement();  // 5

    cout << "\n当前状态：" << endl;
    counter.display();

    cout << "\n测试边界：" << endl;
    for (int i = 0; i < 15; i++) {
        counter.increment();  // 尝试超过最大值
    }

    cout << "\n重置计数：" << endl;
    counter.reset();
    counter.display();

    return 0;
}

// ============================================
// 练习题
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 设计一个矩形类（Rectangle）
   要求：
   - 私有成员变量：长度（length）、宽度（width）
   - 公有成员函数：
     * init(double len, double wid)：初始化
     * getArea()：计算面积
     * getPerimeter()：计算周长
     * isSquare()：判断是否为正方形
     * display()：显示信息
   提示：
   - 长度和宽度必须大于0
   - 正方形的长度等于宽度

2. 设计一个时间类（Time）
   要求：
   - 私有成员变量：小时（hour）、分钟（minute）、秒（second）
   - 公有成员函数：
     * init(int h, int m, int s)：初始化
     * addSecond()：增加1秒
     * addMinute()：增加1分钟
     * addHour()：增加1小时
     * display()：显示时间（格式：HH:MM:SS）
   提示：
   - 小时范围：0-23
   - 分钟和秒范围：0-59
   - 注意进位（60秒=1分钟，60分钟=1小时）

3. 设计一个温度类（Temperature）
   要求：
   - 私有成员变量：摄氏度（celsius）
   - 公有成员函数：
     * init(double c)：初始化
     * setCelsius(double c)：设置摄氏度
     * getCelsius()：获取摄氏度
     * toFahrenheit()：转换为华氏度
     * toKelvin()：转换为开尔文
     * display()：显示所有温度单位
   提示：
   - 华氏度 = 摄氏度 * 9/5 + 32
   - 开尔文 = 摄氏度 + 273.15
   - 绝对零度：-273.15°C

4. 设计一个购物车类（ShoppingCart）
   要求：
   - 私有成员变量：商品数量（itemCount）、总价（totalPrice）
   - 公有成员函数：
     * init()：初始化
     * addItem(double price)：添加商品
     * removeItem(double price)：移除商品
     * clear()：清空购物车
     * getItemCount()：获取商品数量
     * getTotalPrice()：获取总价
     * applyDiscount(double rate)：应用折扣
     * display()：显示购物车信息
   提示：
   - 商品价格必须大于0
   - 折扣率范围：0.1-1.0
   - 移除商品时检查是否存在

5. 设计一个密码管理类（Password）
   要求：
   - 私有成员变量：密码（password）、尝试次数（attempts）
   - 公有成员函数：
     * init(string pwd)：初始化
     * verify(string pwd)：验证密码
     * change(string oldPwd, string newPwd)：修改密码
     * reset()：重置尝试次数
     * isLocked()：是否被锁定
     * display()：显示状态
   提示：
   - 密码长度至少6位
   - 最多尝试3次，超过则锁定
   - 修改密码需要验证旧密码
)" << endl;

    return 0;
}
