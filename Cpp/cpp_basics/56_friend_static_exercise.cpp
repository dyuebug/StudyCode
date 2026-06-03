// ============================================
// 56_friend_static_exercise.cpp - 超详细注释版
// 日期：第17天
// 难度：⭐⭐ 中级
// 主题：friend 与 static 综合练习
// ============================================

/*
【本次练习目标】

综合运用第54、55课所学：
- friend 友元函数与友元类
- static 静态成员变量与静态成员函数

项目：实现一个简化的 StudentRegistry（学生注册表）系统
      - 所有 Student 对象注册到全局注册表（static）
      - 注册表可以统计、查询学生信息（friend 访问私有数据）
      - 提供工厂函数创建学生（static）
*/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

// ============================================
// 第一部分：系统设计说明 ⭐⭐⭐⭐⭐
// ============================================

/*
【设计目标】

Student 类：
  - 私有：name、score、id
  - 静态：totalCount（学生总数）、totalScore（总分，用于算平均）
  - 静态工厂：create(name, score)
  - 友元：StudentRegistry（可访问私有数据做统计）

StudentRegistry 类（友元类）：
  - 注册/注销学生
  - 统计：总数、平均分、最高分、最低分
  - 查询：按名字查找、按分数排序
  - 报告：格式化打印所有学生信息

【friend + static 的配合】

  friend  → 让 StudentRegistry 读取 Student 的 private name/score/id
  static  → 让 Student 跟踪全局总数和总分，无需外部维护
*/

// 前向声明：Student 类需要声明 StudentRegistry 为友元
class StudentRegistry;

// ============================================
// 第二部分：Student 类实现
// ============================================

class Student {
private:
    string name;   // 学生姓名（private）
    double score;  // 分数 0-100（private）
    int    id;     // 唯一 ID（private）

    // 静态成员变量：属于类，所有 Student 对象共享
    static int    totalCount;  // 当前存活的学生对象数
    static double totalScore;  // 所有学生分数之和（用于计算平均分）
    static int    nextId;      // 下一个可用 ID

public:
    // 构造函数：初始化成员，更新静态计数
    Student(const string& name, double score)
        : name(name), score(score) {
        id = nextId++;         // 分配唯一 ID
        ++totalCount;          // 学生总数 +1
        totalScore += score;   // 累加到总分
    }

    // 析构函数：更新静态计数
    ~Student() {
        --totalCount;          // 学生总数 -1
        totalScore -= score;   // 从总分减去
    }

    // 禁止拷贝（避免 ID 重复、计数混乱）
    Student(const Student&)            = delete;
    Student& operator=(const Student&) = delete;

    // ---- 静态工厂函数 ----
    // 语义清晰：Student::create("Alice", 90) 比 new Student("Alice", 90) 更明确
    // 返回 Student* 指针（堆对象，生命周期由调用者管理）
    static Student* create(const string& name, double score) {
        // 分数验证
        if (score < 0 || score > 100) {
            cout << "  [错误] 无效分数：" << score << "（必须 0-100）" << endl;
            return nullptr;
        }
        return new Student(name, score);
    }

    // ---- 静态统计函数 ----
    static int    getCount()   { return totalCount; }
    static double getAverage() {
        if (totalCount == 0) return 0.0;
        return totalScore / totalCount;
    }

    // ---- 普通成员函数（只暴露必要信息）----
    int getId() const { return id; }
    // 注意：不提供 getName()/getScore() 的 public 版本
    // StudentRegistry 通过友元访问私有成员，不需要 public getter

    // 声明 StudentRegistry 为友元类
    // StudentRegistry 的所有成员函数都可以访问 Student 的 private 成员
    friend class StudentRegistry;

    // 友元运算符：格式化输出（左操作数是 ostream，必须是友元）
    friend ostream& operator<<(ostream& out, const Student& s);
};

// ---- 类外初始化静态成员 ----
int    Student::totalCount = 0;
double Student::totalScore = 0.0;
int    Student::nextId     = 1;

// 友元 operator<< 定义
ostream& operator<<(ostream& out, const Student& s) {
    // 访问 private 成员 name、score、id（友元权限）
    out << "Student(#" << s.id << " " << s.name
        << ", 分数=" << s.score << ")";
    return out;
}

// ============================================
// 第三部分：StudentRegistry 友元类实现
// ============================================

class StudentRegistry {
private:
    // 存储所有注册学生的指针
    vector<Student*> students;
    string           registryName;

public:
    StudentRegistry(const string& name = "默认注册表")
        : registryName(name) {}

    // 注册学生
    void add(Student* s) {
        if (s == nullptr) {
            cout << "  [注册表] 忽略空指针" << endl;
            return;
        }
        students.push_back(s);
        // 作为友元，可以访问 s->name（private）
        cout << "  [注册] " << s->name << " (ID=" << s->id << ")" << endl;
    }

    // 注销学生（从注册表移除，但不 delete）
    void remove(int id) {
        auto it = find_if(students.begin(), students.end(),
            [id](const Student* s) { return s->id == id; });  // 访问 private id
        if (it != students.end()) {
            cout << "  [注销] " << (*it)->name << " (ID=" << id << ")" << endl;
            students.erase(it);
        }
    }

    // ---- 统计功能（通过友元访问 private score）----

    double getAverage() const {
        if (students.empty()) return 0.0;
        double sum = 0;
        for (const auto* s : students) sum += s->score;  // 访问 private score
        return sum / students.size();
    }

    // 返回最高分学生（友元访问 private score）
    const Student* getTopStudent() const {
        if (students.empty()) return nullptr;
        return *max_element(students.begin(), students.end(),
            [](const Student* a, const Student* b) {
                return a->score < b->score;  // 访问 private score
            });
    }

    // 返回最低分学生
    const Student* getBottomStudent() const {
        if (students.empty()) return nullptr;
        return *min_element(students.begin(), students.end(),
            [](const Student* a, const Student* b) {
                return a->score < b->score;
            });
    }

    // 按分数排序（降序）并返回副本
    vector<const Student*> getSortedByScore() const {
        vector<const Student*> sorted(students.begin(), students.end());
        sort(sorted.begin(), sorted.end(),
            [](const Student* a, const Student* b) {
                return a->score > b->score;  // 降序
            });
        return sorted;
    }

    // 按名字查找
    const Student* findByName(const string& name) const {
        auto it = find_if(students.begin(), students.end(),
            [&name](const Student* s) {
                return s->name == name;  // 访问 private name
            });
        return it != students.end() ? *it : nullptr;
    }

    // ---- 报告输出 ----

    void printReport() const {
        cout << "\n" << string(60, '-') << endl;
        cout << "  注册表：" << registryName << endl;
        cout << "  学生总数：" << students.size() << endl;
        cout << "  平均分：" << getAverage() << endl;

        if (!students.empty()) {
            const Student* top = getTopStudent();
            const Student* bot = getBottomStudent();
            // 访问 private name 和 score（友元类权限）
            cout << "  最高分：" << top->name << " (" << top->score << ")" << endl;
            cout << "  最低分：" << bot->name << " (" << bot->score << ")" << endl;
        }

        cout << "\n  成绩排名（降序）：" << endl;
        int rank = 1;
        for (const auto* s : getSortedByScore()) {
            cout << "    " << rank++ << ". "
                 << s->name << "\t" << s->score << endl;
        }
        cout << string(60, '-') << endl;
    }
};

// ============================================
// 第四部分：演示与测试
// ============================================

void demoSystem() {
    cout << string(60, '=') << endl;
    cout << "第四部分：StudentRegistry 演示" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 创建学生（静态工厂）---" << endl;
    // 使用静态工厂函数（无需先创建对象）
    Student* alice   = Student::create("Alice",   92.5);
    Student* bob     = Student::create("Bob",     78.0);
    Student* charlie = Student::create("Charlie", 85.5);
    Student* diana   = Student::create("Diana",   96.0);
    Student* eve     = Student::create("Eve",     62.0);

    cout << "\n--- 静态统计（全局）---" << endl;
    cout << "全局学生数：" << Student::getCount() << endl;
    cout << "全局平均分：" << Student::getAverage() << endl;

    cout << "\n--- 注册到注册表（友元类访问私有数据）---" << endl;
    StudentRegistry registry("2026年春季班");
    registry.add(alice);
    registry.add(bob);
    registry.add(charlie);
    registry.add(diana);
    registry.add(eve);

    cout << "\n--- 注册表报告 ---" << endl;
    registry.printReport();

    cout << "\n--- 按名字查找 ---" << endl;
    const Student* found = registry.findByName("Charlie");
    if (found) cout << "找到：" << *found << endl;  // 调用 operator<<

    cout << "\n--- 注销学生 ---" << endl;
    registry.remove(bob->getId());
    registry.printReport();

    cout << "\n--- 无效分数（工厂函数验证）---" << endl;
    Student* invalid = Student::create("Test", 110);  // 超出范围
    cout << "无效学生指针：" << (invalid == nullptr ? "nullptr（创建失败）" : "非空") << endl;

    cout << "\n--- 销毁学生对象 ---" << endl;
    delete alice;
    delete bob;
    delete charlie;
    delete diana;
    delete eve;
    cout << "全部销毁后，全局学生数：" << Student::getCount() << endl;
}

// ============================================
// 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：友元类声明在前向声明之前

   // 错误顺序：
   class StudentRegistry {
       void process(Student& s) { ... }  // ❌ Student 还未定义
   };
   class Student {
       friend class StudentRegistry;
   };

   ✅ 正确：先前向声明，再定义使用它的类
   class StudentRegistry;   // 前向声明
   class Student {
       friend class StudentRegistry;
   };
   class StudentRegistry {  // 完整定义
       void process(Student& s) { ... }
   };

------------------------------------------------------------

❌ 错误2：静态成员在构造函数初始化列表中更新（逻辑错误）

   // 正确但次优写法：
   Student(string n, double s) : name(n), score(s) {
       totalCount++;   // 在构造函数体中更新静态变量 ✅
   }

   // 常见误解：在初始化列表中写静态成员
   Student(string n, double s) : name(n), score(s), totalCount(totalCount+1) {}
   // ❌ 编译错误：静态成员不能在初始化列表中

------------------------------------------------------------

❌ 错误3：友元类只获得了访问权，但仍然依赖对象存在

   class Registry {
       const Student* ptr;
   public:
       Registry(const Student* s) : ptr(s) {}
       void show() {
           cout << ptr->name;   // ❌ 如果 ptr 指向的 Student 已被 delete，是悬空指针！
       }
   };

   ✅ 友元类不延长被友元对象的生命周期
   使用前检查对象是否仍然有效，或用智能指针管理生命周期

------------------------------------------------------------

❌ 错误4：在拷贝构造函数中忘记更新静态计数

   Student(const Student& other) : name(other.name), score(other.score) {
       // ❌ 忘记：id = nextId++; ++totalCount; totalScore += score;
   }
   // 拷贝后 ID 重复，计数偏少

   ✅ 本例直接禁用拷贝（= delete），避免此问题
   Student(const Student&) = delete;

------------------------------------------------------------

❌ 错误5：静态工厂返回 nullptr 后未检查就使用

   Student* s = Student::create("Test", 150);  // 无效分数，返回 nullptr
   registry.add(s);   // ❌ 如果 add 内部不检查，解引用空指针 → 崩溃

   ✅ 工厂函数返回指针时，调用方必须检查：
   if (s != nullptr) registry.add(s);
   // 或者 add() 内部检查（本例已做到）

------------------------------------------------------------

❌ 错误6：析构时没有从总分减去（统计数据不一致）

   ~Student() {
       --totalCount;    // ✅ 减了计数
       // ❌ 忘记：totalScore -= score;
   }
   // 此后平均分 = totalScore / totalCount 计算偏高

   ✅ 构造和析构必须对称：加了什么，析构时就减什么
)";
}

// ============================================
// 第六部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【friend + static 组合模式速查】

对象计数器（static）：
    class Foo {
        static int count;                 // 类内声明
    public:
        Foo()  { ++count; }
        ~Foo() { --count; }
        static int getCount() { return count; }
    };
    int Foo::count = 0;                   // 类外初始化

静态工厂（static）：
    static Type* create(args) {
        if (无效参数) return nullptr;
        return new Type(args);
    }

友元统计类（friend class）：
    class Tracker {
    public:
        void analyze(const Foo& f) {
            f.privateField;   // 访问私有成员
        }
    };
    class Foo {
        int privateField;
        friend class Tracker;   // Tracker 可访问所有私有成员
    };

友元输出运算符（friend function）：
    friend ostream& operator<<(ostream& out, const Type& t) {
        out << t.privateField;  // 访问私有成员
        return out;
    }
)";
}

// ============================================
// 第七部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第七部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 扩展题：为 StudentRegistry 添加以下功能

   a) 分组统计：
      - countAbove(double threshold) — 统计高于阈值的学生数
      - getPassRate() — 及格率（60分以上的比例）

   b) 修改学生分数（需要友元权限）：
      void updateScore(int studentId, double newScore)
      提示：修改后还要更新 Student::totalScore 静态变量

   c) 导出报告：
      void exportReport(ostream& out) — 将报告写入任意输出流
      验证：registry.exportReport(cout)、registry.exportReport(fileStream)

------------------------------------------------------------

2. 独立实现：InventoryManager（库存管理）

   class Product {
       string name;
       double price;
       int    stock;
       static int totalProducts;
       static double totalValue;   // 所有产品总价值（price * stock 之和）
   public:
       static Product* create(string name, double price, int stock);
       static void printGlobalStats();
       friend class InventoryManager;
       friend ostream& operator<<(ostream&, const Product&);
   };

   class InventoryManager {
   public:
       void addProduct(Product* p);
       void restock(int productId, int qty);    // 补货（修改 stock 和 totalValue）
       void sell(int productId, int qty);        // 销售（减少 stock）
       void printCatalog() const;               // 格式化输出所有产品
       Product* findCheapest() const;           // 找最便宜的
   };

------------------------------------------------------------

3. 思考题：以下设计哪个更好？请说明理由

   方案A：
   class Student {
       int score;
   public:
       int getScore() const { return score; }  // public getter
   };
   class Registry {
       void analyze(const Student& s) { s.getScore(); }
   };

   方案B：
   class Student {
       int score;
       friend class Registry;
   };
   class Registry {
       void analyze(const Student& s) { s.score; }  // 直接访问
   };

   分析维度：封装性、耦合度、维护性、访问控制粒度
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 56_friend_static_exercise.cpp - friend 与 static 综合练习" << endl;
    cout << string(60, '=') << endl;

    demoSystem();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. friend class：让另一个类访问所有私有成员（用于紧密协作）" << endl;
    cout << "2. static + friend 组合：全局统计（static）+ 详细访问（friend）" << endl;
    cout << "3. 工厂函数：在创建时做验证，返回 nullptr 表示失败" << endl;
    cout << "4. 构造和析构对称：静态变量的增减必须成对出现" << endl;
    cout << "5. 禁用拷贝：= delete 避免 ID 重复和计数错误" << endl;

    return 0;
}
