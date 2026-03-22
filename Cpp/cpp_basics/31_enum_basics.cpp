/*
 * ============================================================================
 * 文件名：31_enum_basics.cpp
 * 主题：枚举类型详解
 * 学习目标：
 *   1. 理解枚举的概念（命名的整数常量）
 *   2. 掌握枚举的定义和使用
 *   3. 理解枚举的底层类型
 *   4. 掌握枚举类（C++11强类型枚举）
 *   5. 学会枚举的实际应用场景
 *
 * 编译命令：g++ -std=c++11 -o 31_enum_basics 31_enum_basics.cpp
 * 运行命令：./31_enum_basics (Linux/Mac) 或 31_enum_basics.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// 定义星期枚举
enum Weekday {
    MONDAY,      // 0
    TUESDAY,     // 1
    WEDNESDAY,   // 2
    THURSDAY,    // 3
    FRIDAY,      // 4
    SATURDAY,    // 5
    SUNDAY       // 6
};

// 定义颜色枚举（指定值）
enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 4,
    YELLOW = 8
};

// 定义状态枚举
enum Status {
    PENDING = 0,
    RUNNING = 1,
    SUCCESS = 2,
    FAILED = 3
};

// C++11枚举类（强类型枚举）
enum class TrafficLight {
    RED,
    YELLOW,
    GREEN
};

enum class Direction {
    NORTH,
    SOUTH,
    EAST,
    WEST
};

// 函数声明
string getWeekdayName(Weekday day);
string getStatusName(Status s);
string getTrafficLightName(TrafficLight light);

int main() {
    cout << "=== 枚举类型详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是枚举？
    // ========================================
    cout << "【1. 什么是枚举？】" << endl;
    cout << "枚举（enum）是一组命名的整数常量" << endl;
    cout << "作用：提高代码可读性，避免魔法数字" << endl;
    cout << endl;

    cout << "为什么需要枚举？" << endl;
    cout << "  ✗ 魔法数字：if (status == 1) { ... }  // 1是什么意思？" << endl;
    cout << "  ✓ 枚举：if (status == RUNNING) { ... }  // 清晰易懂！" << endl;
    cout << endl;

    // ========================================
    // 2. 枚举的定义
    // ========================================
    cout << "【2. 枚举的定义】" << endl;
    cout << "定义格式：" << endl;
    cout << "  enum 枚举名 {" << endl;
    cout << "      枚举值1," << endl;
    cout << "      枚举值2," << endl;
    cout << "      ..." << endl;
    cout << "  };" << endl;
    cout << endl;

    cout << "示例：Weekday枚举" << endl;
    cout << "  enum Weekday {" << endl;
    cout << "      MONDAY,    // 默认从0开始" << endl;
    cout << "      TUESDAY,   // 1" << endl;
    cout << "      WEDNESDAY, // 2" << endl;
    cout << "      ..." << endl;
    cout << "  };" << endl;
    cout << endl;

    // ========================================
    // 3. 枚举的使用
    // ========================================
    cout << "【3. 枚举的使用】" << endl;

    Weekday today = MONDAY;
    cout << "Weekday today = MONDAY;" << endl;
    cout << "today的值：" << today << " (整数值)" << endl;
    cout << "today的名称：" << getWeekdayName(today) << endl;
    cout << endl;

    // 枚举可以用在switch语句中
    cout << "使用switch判断星期：" << endl;
    switch (today) {
        case MONDAY:
            cout << "  今天是星期一，新的一周开始！" << endl;
            break;
        case FRIDAY:
            cout << "  今天是星期五，快到周末了！" << endl;
            break;
        case SATURDAY:
        case SUNDAY:
            cout << "  今天是周末，休息日！" << endl;
            break;
        default:
            cout << "  工作日" << endl;
    }
    cout << endl;

    // ========================================
    // 4. 枚举的底层值
    // ========================================
    cout << "【4. 枚举的底层值】" << endl;

    cout << "默认值（从0开始递增）：" << endl;
    cout << "  MONDAY = " << MONDAY << endl;
    cout << "  TUESDAY = " << TUESDAY << endl;
    cout << "  WEDNESDAY = " << WEDNESDAY << endl;
    cout << "  THURSDAY = " << THURSDAY << endl;
    cout << "  FRIDAY = " << FRIDAY << endl;
    cout << "  SATURDAY = " << SATURDAY << endl;
    cout << "  SUNDAY = " << SUNDAY << endl;
    cout << endl;

    cout << "指定值：" << endl;
    cout << "  enum Color {" << endl;
    cout << "      RED = 1," << endl;
    cout << "      GREEN = 2," << endl;
    cout << "      BLUE = 4," << endl;
    cout << "      YELLOW = 8" << endl;
    cout << "  };" << endl;
    cout << "  RED = " << RED << endl;
    cout << "  GREEN = " << GREEN << endl;
    cout << "  BLUE = " << BLUE << endl;
    cout << "  YELLOW = " << YELLOW << endl;
    cout << endl;

    // ========================================
    // 5. 枚举与整数的转换
    // ========================================
    cout << "【5. 枚举与整数的转换】" << endl;

    // 枚举转整数（隐式转换）
    int dayValue = WEDNESDAY;
    cout << "枚举转整数：int dayValue = WEDNESDAY;" << endl;
    cout << "dayValue = " << dayValue << endl;
    cout << endl;

    // 整数转枚举（需要显式转换）
    Weekday day = static_cast<Weekday>(3);
    cout << "整数转枚举：Weekday day = static_cast<Weekday>(3);" << endl;
    cout << "day = " << getWeekdayName(day) << endl;
    cout << endl;

    // ========================================
    // 6. 枚举的实际应用
    // ========================================
    cout << "【6. 枚举的实际应用】" << endl;

    Status taskStatus = PENDING;
    cout << "任务状态：" << getStatusName(taskStatus) << endl;

    // 模拟任务执行
    taskStatus = RUNNING;
    cout << "任务开始执行..." << endl;
    cout << "任务状态：" << getStatusName(taskStatus) << endl;

    taskStatus = SUCCESS;
    cout << "任务执行完成！" << endl;
    cout << "任务状态：" << getStatusName(taskStatus) << endl;
    cout << endl;

    // ========================================
    // 7. 传统枚举的问题
    // ========================================
    cout << "【7. 传统枚举的问题】" << endl;

    cout << "问题1：枚举值会污染外层作用域" << endl;
    cout << "  enum Color { RED, GREEN, BLUE };" << endl;
    cout << "  enum TrafficLight { RED, YELLOW, GREEN };  // 错误！RED重复定义" << endl;
    cout << endl;

    cout << "问题2：枚举值可以隐式转换为整数" << endl;
    cout << "  Weekday day = MONDAY;" << endl;
    cout << "  int x = day + 10;  // 可以编译，但语义不明确" << endl;
    int x = MONDAY + 10;
    cout << "  结果：x = " << x << endl;
    cout << endl;

    cout << "问题3：不同枚举类型可以比较" << endl;
    cout << "  if (MONDAY == RED) { ... }  // 可以编译，但没有意义" << endl;
    if (MONDAY == RED) {
        cout << "  MONDAY == RED 为真（都是1）" << endl;
    }
    cout << endl;

    // ========================================
    // 8. C++11枚举类（强类型枚举）
    // ========================================
    cout << "【8. C++11枚举类（强类型枚举）】" << endl;

    cout << "定义格式：enum class 枚举名 { ... };" << endl;
    cout << "  enum class TrafficLight {" << endl;
    cout << "      RED," << endl;
    cout << "      YELLOW," << endl;
    cout << "      GREEN" << endl;
    cout << "  };" << endl;
    cout << endl;

    TrafficLight light = TrafficLight::RED;
    cout << "使用：TrafficLight light = TrafficLight::RED;" << endl;
    cout << "注意：必须使用作用域限定符 ::" << endl;
    cout << endl;

    // 枚举类的优势
    cout << "枚举类的优势：" << endl;
    cout << "  ✓ 不污染外层作用域" << endl;
    cout << "  ✓ 不能隐式转换为整数" << endl;
    cout << "  ✓ 不同枚举类型不能比较" << endl;
    cout << "  ✓ 可以指定底层类型" << endl;
    cout << endl;

    // 枚举类不能隐式转换为整数
    // int lightValue = light;  // 编译错误！
    int lightValue = static_cast<int>(light);
    cout << "枚举类转整数（需要显式转换）：" << endl;
    cout << "  int lightValue = static_cast<int>(light);" << endl;
    cout << "  lightValue = " << lightValue << endl;
    cout << endl;

    // 使用枚举类
    cout << "使用枚举类：" << endl;
    switch (light) {
        case TrafficLight::RED:
            cout << "  红灯停！" << endl;
            break;
        case TrafficLight::YELLOW:
            cout << "  黄灯等！" << endl;
            break;
        case TrafficLight::GREEN:
            cout << "  绿灯行！" << endl;
            break;
    }
    cout << endl;

    // ========================================
    // 9. 指定枚举的底层类型
    // ========================================
    cout << "【9. 指定枚举的底层类型】" << endl;

    cout << "默认底层类型：int" << endl;
    cout << "可以指定其他类型：" << endl;
    cout << "  enum class Size : char {" << endl;
    cout << "      SMALL," << endl;
    cout << "      MEDIUM," << endl;
    cout << "      LARGE" << endl;
    cout << "  };" << endl;
    cout << "好处：节省内存空间" << endl;
    cout << endl;

    // ========================================
    // 10. 枚举的最佳实践
    // ========================================
    cout << "【10. 枚举的最佳实践】" << endl;

    cout << "✓ 推荐做法：" << endl;
    cout << "  1. 优先使用enum class（C++11）" << endl;
    cout << "  2. 枚举名使用大驼峰命名：TrafficLight" << endl;
    cout << "  3. 枚举值使用全大写：RED, GREEN, BLUE" << endl;
    cout << "  4. 为枚举值添加注释说明含义" << endl;
    cout << "  5. 避免魔法数字，用枚举表示状态和类型" << endl;
    cout << endl;

    cout << "✗ 避免做法：" << endl;
    cout << "  1. 不要用整数代替枚举" << endl;
    cout << "  2. 不要在枚举值之间进行算术运算" << endl;
    cout << "  3. 不要依赖枚举的默认值（除非有明确意图）" << endl;
    cout << endl;

    // ========================================
    // 11. 枚举的应用场景
    // ========================================
    cout << "【11. 枚举的应用场景】" << endl;

    cout << "常见应用场景：" << endl;
    cout << "  ✓ 状态机：IDLE, RUNNING, PAUSED, STOPPED" << endl;
    cout << "  ✓ 错误码：SUCCESS, ERROR_FILE_NOT_FOUND, ERROR_PERMISSION_DENIED" << endl;
    cout << "  ✓ 方向：NORTH, SOUTH, EAST, WEST" << endl;
    cout << "  ✓ 颜色：RED, GREEN, BLUE" << endl;
    cout << "  ✓ 优先级：LOW, MEDIUM, HIGH" << endl;
    cout << "  ✓ 日志级别：DEBUG, INFO, WARNING, ERROR" << endl;
    cout << endl;

    // ========================================
    // 12. 总结
    // ========================================
    cout << "【12. 总结】" << endl;

    cout << "传统枚举 vs 枚举类：" << endl;
    cout << "┌────────────────┬──────────────┬──────────────┐" << endl;
    cout << "│   特性         │   传统枚举   │   枚举类     │" << endl;
    cout << "├────────────────┼──────────────┼──────────────┤" << endl;
    cout << "│ 作用域         │   污染外层   │   独立作用域 │" << endl;
    cout << "│ 隐式转换       │   可以       │   不可以     │" << endl;
    cout << "│ 类型安全       │   弱         │   强         │" << endl;
    cout << "│ 使用方式       │   直接使用   │   需要::     │" << endl;
    cout << "│ 推荐程度       │   不推荐     │   推荐       │" << endl;
    cout << "└────────────────┴──────────────┴──────────────┘" << endl;
    cout << endl;

    cout << "记住：" << endl;
    cout << "  枚举让代码更易读、更易维护" << endl;
    cout << "  优先使用enum class（C++11）" << endl;
    cout << "  避免魔法数字，用枚举表达意图" << endl;
    cout << endl;

    return 0;
}

// 函数实现
string getWeekdayName(Weekday day) {
    switch (day) {
        case MONDAY: return "星期一";
        case TUESDAY: return "星期二";
        case WEDNESDAY: return "星期三";
        case THURSDAY: return "星期四";
        case FRIDAY: return "星期五";
        case SATURDAY: return "星期六";
        case SUNDAY: return "星期日";
        default: return "未知";
    }
}

string getStatusName(Status s) {
    switch (s) {
        case PENDING: return "等待中";
        case RUNNING: return "运行中";
        case SUCCESS: return "成功";
        case FAILED: return "失败";
        default: return "未知";
    }
}

string getTrafficLightName(TrafficLight light) {
    switch (light) {
        case TrafficLight::RED: return "红灯";
        case TrafficLight::YELLOW: return "黄灯";
        case TrafficLight::GREEN: return "绿灯";
        default: return "未知";
    }
}
