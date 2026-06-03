// ============================================
// 98_cpp20_knowledge_map.cpp - C++20 知识地图
// 功能：展示 C++20 特性之间的关系和应用场景
// 日期：2026-06-06（第37天）
// 难度：⭐⭐⭐⭐ 高级
// 编译：g++ -std=c++20 -o 98_cpp20_knowledge_map.exe 98_cpp20_knowledge_map.cpp
// ============================================

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：C++20 特性知识地图 ⭐⭐⭐⭐⭐
// ============================================

/*
【C++20 特性知识地图】

C++20 四大特性构成了现代 C++ 的核心能力：

                    C++20 核心特性
                         │
        ┌────────────────┼────────────────┐
        │                │                │
    类型安全层       数据处理层      异步编程层      代码组织层
        │                │                │                │
    Concepts          Ranges         Coroutines        Modules
        │                │                │                │
   ┌────┴────┐      ┌────┴────┐      ┌────┴────┐      ┌────┴────┐
   │         │      │         │      │         │      │         │
 模板编程  泛型算法  函数式   性能优化  生成器  异步I/O  编译速度  封装性
   │         │      │         │      │         │      │         │
 库开发   类型安全  数据处理  零拷贝   状态机  事件驱动  依赖管理  模块化

【特性之间的关系】

1. 互补关系：
   - Concepts 为 Ranges 提供类型约束
   - Ranges 可以处理 Coroutines 生成的数据
   - Modules 可以包含其他三个特性

2. 组合使用：
   - Concepts + Ranges：约束范围算法的类型
   - Ranges + Coroutines：处理异步生成的数据流
   - Modules + Concepts：模块化的类型安全代码
   - Modules + Ranges：模块化的数据处理库

3. 应用层次：
   - 底层：Modules（代码组织）
   - 中层：Concepts（类型约束）、Ranges（数据处理）
   - 上层：Coroutines（异步编程）
*/

int main() {
    cout << string(60, '=') << endl;
    cout << "C++20 知识地图" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 知识地图：整体结构
    // --------------------------------------------

    cout << "\n【知识地图1】整体结构" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
C++20 核心特性知识地图
════════════════════════════════════════════════════════════

                        C++20
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
    编译期特性        运行期特性        代码组织
        │                 │                 │
    ┌───┴───┐         ┌───┴───┐             │
    │       │         │       │             │
Concepts  constexpr  Ranges  Coroutines  Modules
    │       │         │       │             │
    │       │         │       │             │
类型约束  编译期计算  数据处理  异步编程   模块化

【层次关系】

第一层：代码组织（Modules）
- 提供模块化的代码组织方式
- 包含其他所有特性
- 提升编译速度和封装性

第二层：类型系统（Concepts）
- 提供编译期类型约束
- 为其他特性提供类型安全
- 提升错误提示质量

第三层：数据处理（Ranges）
- 提供高效的数据处理方式
- 函数式编程风格
- 惰性求值和零拷贝

第四层：异步编程（Coroutines）
- 提供异步编程能力
- 生成器和状态机
- 避免回调地狱

【依赖关系】

Modules（独立）
  └─ 可以包含 Concepts、Ranges、Coroutines

Concepts（独立）
  └─ 可以约束 Ranges 和 Coroutines 的类型

Ranges（依赖 Concepts）
  └─ 使用 Concepts 约束范围类型

Coroutines（独立）
  └─ 可以与 Ranges 组合处理异步数据
)" << endl;

    // --------------------------------------------
    // 1.2 知识地图：应用场景分类
    // --------------------------------------------

    cout << "\n【知识地图2】应用场景分类" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
按应用场景分类的知识地图
════════════════════════════════════════════════════════════

1. 模板编程场景
   │
   ├─ Concepts：约束模板参数
   │   └─ 示例：template<std::integral T> void func(T x);
   │
   └─ Ranges：泛型算法
       └─ 示例：ranges::sort(container);

2. 数据处理场景
   │
   ├─ Ranges：过滤、转换、聚合
   │   └─ 示例：data | views::filter(...) | views::transform(...)
   │
   └─ Concepts：约束数据类型
       └─ 示例：template<std::ranges::range R> void process(R&& r);

3. 异步编程场景
   │
   ├─ Coroutines：异步 I/O
   │   └─ 示例：Task<string> fetch_data(string url);
   │
   ├─ Coroutines：生成器
   │   └─ 示例：Generator<int> fibonacci();
   │
   └─ Ranges：处理异步数据流
       └─ 示例：async_data | views::take(10);

4. 代码组织场景
   │
   ├─ Modules：模块化代码
   │   └─ 示例：export module math;
   │
   ├─ Modules + Concepts：类型安全的模块
   │   └─ 示例：export template<Sortable T> void sort(T& c);
   │
   └─ Modules + Ranges：数据处理模块
       └─ 示例：export auto process(auto&& data);

5. 性能优化场景
   │
   ├─ Ranges：零拷贝、惰性求值
   │   └─ 示例：views::filter(...) 不创建临时容器
   │
   ├─ Modules：编译速度提升
   │   └─ 示例：模块只编译一次，提速 10-100 倍
   │
   └─ Coroutines：内存优化
       └─ 示例：生成器按需生成，节省内存

6. 类型安全场景
   │
   ├─ Concepts：编译期类型检查
   │   └─ 示例：concept Sortable = requires(T a, T b) { a < b; };
   │
   └─ Modules：封装性
       └─ 示例：未 export 的内容是私有的
)" << endl;

    // --------------------------------------------
    // 1.3 知识地图：技术栈选择
    // --------------------------------------------

    cout << "\n【知识地图3】技术栈选择" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
不同项目类型的技术栈选择
════════════════════════════════════════════════════════════

1. 新项目（推荐全部使用）
   ┌─────────────────────────────────────┐
   │ Modules：模块化代码组织             │
   │ Concepts：类型安全                  │
   │ Ranges：数据处理                    │
   │ Coroutines：异步编程（按需）        │
   └─────────────────────────────────────┘

2. 大型项目（优先编译速度和类型安全）
   ┌─────────────────────────────────────┐
   │ Modules：提升编译速度（优先）       │
   │ Concepts：提升类型安全（优先）      │
   │ Ranges：数据处理（推荐）            │
   │ Coroutines：异步编程（按需）        │
   └─────────────────────────────────────┘

3. 数据处理项目（优先 Ranges）
   ┌─────────────────────────────────────┐
   │ Ranges：数据处理（核心）            │
   │ Concepts：类型约束（推荐）          │
   │ Modules：代码组织（可选）           │
   │ Coroutines：异步数据流（按需）      │
   └─────────────────────────────────────┘

4. 异步 I/O 项目（优先 Coroutines）
   ┌─────────────────────────────────────┐
   │ Coroutines：异步编程（核心）        │
   │ Ranges：数据处理（推荐）            │
   │ Concepts：类型约束（推荐）          │
   │ Modules：代码组织（可选）           │
   └─────────────────────────────────────┘

5. 库开发项目（优先类型安全和封装）
   ┌─────────────────────────────────────┐
   │ Concepts：类型约束（核心）          │
   │ Modules：封装性（推荐）             │
   │ Ranges：泛型算法（推荐）            │
   │ Coroutines：异步接口（按需）        │
   └─────────────────────────────────────┘

6. 现有项目（渐进式引入）
   ┌─────────────────────────────────────┐
   │ Concepts：新代码使用（优先）        │
   │ Ranges：性能关键部分（推荐）        │
   │ Modules：独立模块（谨慎）           │
   │ Coroutines：异步部分（按需）        │
   └─────────────────────────────────────┘
)" << endl;

    // ============================================
    // 第二部分：特性组合使用示例 ⭐⭐⭐⭐⭐
    // ============================================

    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：特性组合使用示例" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 Concepts + Ranges 组合
    // --------------------------------------------

    cout << "\n【组合1】Concepts + Ranges" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景】
使用 Concepts 约束 Ranges 算法的类型参数

【示例代码】
------------------------------------------------------------
// 定义概念：可排序的范围
template<typename R>
concept SortableRange = std::ranges::range<R> &&
    requires(R r) {
        std::ranges::sort(r);
    };

// 使用概念约束函数参数
template<SortableRange R>
void process_and_sort(R&& data) {
    // 使用 Ranges 处理数据
    auto result = data
        | views::filter([](auto x) { return x > 0; })
        | views::transform([](auto x) { return x * 2; });

    // 排序
    std::ranges::sort(result);
}

// 使用
vector<int> vec = {3, -1, 4, -2, 5};
process_and_sort(vec);

【优势】
1. 类型安全：编译期检查类型是否可排序
2. 错误提示清晰：不满足约束时给出明确错误
3. 代码简洁：Ranges 管道操作 + Concepts 约束

【适用场景】
- 泛型算法库开发
- 需要类型约束的数据处理
- 提升代码可读性和安全性
)" << endl;

    // --------------------------------------------
    // 2.2 Ranges + Coroutines 组合
    // --------------------------------------------

    cout << "\n【组合2】Ranges + Coroutines" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景】
使用 Coroutines 生成数据，用 Ranges 处理数据

【示例代码】
------------------------------------------------------------
// 使用协程生成斐波那契数列
Generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        int next = a + b;
        a = b;
        b = next;
    }
}

// 使用 Ranges 处理协程生成的数据
auto result = fibonacci()
    | views::take(10)                    // 取前 10 个
    | views::filter([](int x) { return x % 2 == 0; })  // 过滤偶数
    | views::transform([](int x) { return x * 2; });   // 乘以 2

// 输出：0 4 16 68
for (int x : result) {
    cout << x << " ";
}

【优势】
1. 惰性求值：协程按需生成，Ranges 按需处理
2. 内存高效：不需要存储整个序列
3. 代码简洁：管道操作组合多个步骤

【适用场景】
- 处理无限序列
- 异步数据流处理
- 内存受限的场景
)" << endl;

    // --------------------------------------------
    // 2.3 Modules + Concepts 组合
    // --------------------------------------------

    cout << "\n【组合3】Modules + Concepts" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景】
使用 Modules 组织代码，用 Concepts 提供类型安全的接口

【示例代码】
------------------------------------------------------------
// math.cppm（模块接口单元）
export module math;

import std;

// 定义概念
export template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// 导出使用概念约束的函数
export template<Numeric T>
T add(T a, T b) {
    return a + b;
}

export template<Numeric T>
T multiply(T a, T b) {
    return a * b;
}

// 私有辅助函数（不导出）
template<typename T>
T validate(T x) {
    return x >= 0 ? x : -x;
}

// main.cpp
import math;

int main() {
    int x = add(1, 2);           // ✅ 正确：int 满足 Numeric
    double y = multiply(1.5, 2.0); // ✅ 正确：double 满足 Numeric
    // string z = add("a", "b");  // ❌ 错误：string 不满足 Numeric
    return 0;
}

【优势】
1. 模块化：清晰的代码组织
2. 类型安全：Concepts 约束导出的接口
3. 封装性：私有函数不导出
4. 编译速度：模块只编译一次

【适用场景】
- 库开发：提供类型安全的 API
- 大型项目：模块化组织代码
- 新项目：从头开始的项目
)" << endl;

    // --------------------------------------------
    // 2.4 Modules + Ranges 组合
    // --------------------------------------------

    cout << "\n【组合4】Modules + Ranges" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景】
使用 Modules 组织数据处理代码，提供 Ranges 风格的接口

【示例代码】
------------------------------------------------------------
// data_processor.cppm（模块接口单元）
export module data_processor;

import std;

// 导出数据处理函数
export template<std::ranges::range R>
auto filter_positive(R&& data) {
    return data | std::views::filter([](auto x) { return x > 0; });
}

export template<std::ranges::range R>
auto double_values(R&& data) {
    return data | std::views::transform([](auto x) { return x * 2; });
}

export template<std::ranges::range R>
auto process_pipeline(R&& data) {
    return data
        | filter_positive
        | double_values
        | std::views::take(10);
}

// main.cpp
import data_processor;
import std;

int main() {
    std::vector<int> vec = {1, -2, 3, -4, 5, 6, 7, 8, 9, 10};

    // 使用模块提供的数据处理管道
    auto result = process_pipeline(vec);

    for (int x : result) {
        std::cout << x << " ";  // 2 6 10 12 14 16 18 20
    }

    return 0;
}

【优势】
1. 模块化：数据处理逻辑封装在模块中
2. 可复用：导出的函数可以在多个地方使用
3. 高效：Ranges 惰性求值，零拷贝
4. 简洁：管道操作组合多个步骤

【适用场景】
- 数据处理库开发
- 可复用的数据处理管道
- 大型数据处理项目
)" << endl;

    // --------------------------------------------
    // 2.5 四大特性综合组合
    // --------------------------------------------

    cout << "\n【组合5】四大特性综合" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【应用场景】
综合使用四大特性构建完整的应用

【示例代码】
------------------------------------------------------------
// async_data_processor.cppm（模块接口单元）
export module async_data_processor;

import std;

// 1. 使用 Concepts 定义类型约束
export template<typename T>
concept Processable = std::integral<T> || std::floating_point<T>;

// 2. 使用 Coroutines 实现异步数据生成器
export template<Processable T>
Generator<T> async_generate(int count) {
    for (int i = 0; i < count; ++i) {
        co_yield static_cast<T>(i);
    }
}

// 3. 使用 Ranges 处理数据
export template<Processable T, std::ranges::range R>
auto process_data(R&& data) {
    return data
        | std::views::filter([](T x) { return x > 0; })
        | std::views::transform([](T x) { return x * 2; })
        | std::views::take(10);
}

// 4. 综合使用
export template<Processable T>
auto async_process(int count) {
    auto data = async_generate<T>(count);
    return process_data<T>(data);
}

// main.cpp
import async_data_processor;
import std;

int main() {
    // 使用四大特性：
    // - Modules：模块化代码组织
    // - Concepts：类型约束（Processable）
    // - Coroutines：异步生成数据
    // - Ranges：处理数据流

    auto result = async_process<int>(20);

    for (int x : result) {
        std::cout << x << " ";  // 2 4 6 8 10 12 14 16 18 20
    }

    return 0;
}

【优势】
1. 模块化：清晰的代码组织（Modules）
2. 类型安全：编译期类型检查（Concepts）
3. 异步编程：惰性生成数据（Coroutines）
4. 高效处理：零拷贝、惰性求值（Ranges）

【适用场景】
- 复杂的异步数据处理系统
- 需要类型安全的大型项目
- 高性能数据处理应用
)" << endl;

    // ============================================
    // 第三部分：决策树 ⭐⭐⭐⭐⭐
    // ============================================

    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：特性选择决策树" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 决策树：我应该使用哪个特性？
    // --------------------------------------------

    cout << "\n【决策树】我应该使用哪个特性？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
特性选择决策树
════════════════════════════════════════════════════════════

问题1：你在做什么类型的项目？
│
├─ 新项目
│   └─ 推荐：Modules + Concepts + Ranges + Coroutines（按需）
│
├─ 现有项目
│   └─ 推荐：渐进式引入 Concepts 和 Ranges
│
└─ 库开发
    └─ 推荐：Concepts（核心）+ Modules（推荐）

────────────────────────────────────────────────────────────

问题2：你的主要需求是什么？
│
├─ 类型安全
│   └─ 使用 Concepts
│       ├─ 模板编程：约束模板参数
│       ├─ 泛型算法：定义类型要求
│       └─ 库开发：提供清晰的 API 约束
│
├─ 数据处理
│   └─ 使用 Ranges
│       ├─ 过滤、转换：管道操作
│       ├─ 性能优化：零拷贝、惰性求值
│       └─ 函数式编程：链式调用
│
├─ 异步编程
│   └─ 使用 Coroutines
│       ├─ 异步 I/O：网络请求、文件读写
│       ├─ 生成器：惰性生成序列
│       └─ 状态机：复杂的状态转换
│
└─ 编译速度
    └─ 使用 Modules
        ├─ 大型项目：显著提升编译速度
        ├─ 新项目：从头开始使用
        └─ 强封装：隐藏实现细节

────────────────────────────────────────────────────────────

问题3：你的编译器支持情况如何？
│
├─ GCC 11+ / Clang 16+ / MSVC 2019+
│   └─ 可以使用所有四大特性
│
├─ GCC 10+ / Clang 13+ / MSVC 2019+
│   └─ 可以使用 Concepts 和 Ranges
│       Coroutines 和 Modules 支持有限
│
└─ 更旧的编译器
    └─ 建议升级编译器
        或使用传统方法（SFINAE、STL 算法）

────────────────────────────────────────────────────────────

问题4：你的团队熟悉程度如何？
│
├─ 团队熟悉现代 C++
│   └─ 可以使用所有四大特性
│
├─ 团队部分熟悉
│   └─ 优先使用 Concepts 和 Ranges
│       逐步引入 Coroutines 和 Modules
│
└─ 团队不熟悉
    └─ 从 Concepts 开始学习
        逐步引入其他特性

────────────────────────────────────────────────────────────

问题5：你的性能要求如何？
│
├─ 性能关键
│   └─ 使用 Ranges（零拷贝、惰性求值）
│       谨慎使用 Coroutines（有一定开销）
│
├─ 一般性能要求
│   └─ 可以使用所有四大特性
│
└─ 性能不敏感
    └─ 优先考虑代码可读性和维护性
        使用 Concepts 和 Ranges
)" << endl;

    // ============================================
    // 第四部分：实战建议 ⭐⭐⭐⭐⭐
    // ============================================

    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：实战建议" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 不同场景的最佳实践
    // --------------------------------------------

    cout << "\n【实战1】不同场景的最佳实践" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
场景1：Web 后端开发
════════════════════════════════════════════════════════════
推荐技术栈：
✅ Coroutines（核心）：异步 I/O、网络请求
✅ Ranges（推荐）：数据处理、JSON 解析
✅ Concepts（推荐）：类型安全的 API
✅ Modules（可选）：模块化代码组织

示例：
------------------------------------------------------------
// 使用协程处理异步请求
Task<Response> handle_request(Request req) {
    // 异步查询数据库
    auto data = co_await db.query(req.params);

    // 使用 Ranges 处理数据
    auto result = data
        | views::filter([](auto& item) { return item.active; })
        | views::transform([](auto& item) { return item.to_json(); });

    co_return Response{result};
}

────────────────────────────────────────────────────────────

场景2：数据分析工具
════════════════════════════════════════════════════════════
推荐技术栈：
✅ Ranges（核心）：数据处理、过滤、聚合
✅ Concepts（推荐）：类型约束
✅ Coroutines（可选）：处理大数据集
✅ Modules（可选）：模块化数据处理库

示例：
------------------------------------------------------------
// 使用 Ranges 处理数据
auto analyze_data(const vector<Record>& records) {
    return records
        | views::filter([](auto& r) { return r.valid(); })
        | views::transform([](auto& r) { return r.value; })
        | views::chunk(100)  // 分块处理
        | views::transform([](auto chunk) {
            return std::ranges::accumulate(chunk, 0.0) / chunk.size();
        });
}

────────────────────────────────────────────────────────────

场景3：游戏引擎开发
════════════════════════════════════════════════════════════
推荐技术栈：
✅ Coroutines（推荐）：状态机、事件驱动
✅ Ranges（推荐）：实体管理、数据处理
✅ Concepts（推荐）：组件系统类型约束
✅ Modules（可选）：模块化引擎架构

示例：
------------------------------------------------------------
// 使用协程实现游戏逻辑
Task<void> enemy_ai() {
    while (alive) {
        co_await wait_for_player();
        co_await attack();
        co_await wait(1s);
    }
}

// 使用 Ranges 处理实体
auto active_enemies = entities
    | views::filter([](auto& e) { return e.is_enemy() && e.alive; });

────────────────────────────────────────────────────────────

场景4：库开发
════════════════════════════════════════════════════════════
推荐技术栈：
✅ Concepts（核心）：类型安全的 API
✅ Modules（推荐）：封装性、清晰的接口
✅ Ranges（推荐）：泛型算法
✅ Coroutines（按需）：异步接口

示例：
------------------------------------------------------------
// 使用 Concepts 约束库接口
export module my_library;

export template<typename T>
concept Serializable = requires(T t) {
    { t.serialize() } -> std::convertible_to<std::string>;
};

export template<Serializable T>
void save(const T& obj, const std::string& path);

────────────────────────────────────────────────────────────

场景5：系统工具开发
════════════════════════════════════════════════════════════
推荐技术栈：
✅ Ranges（推荐）：文件处理、文本处理
✅ Coroutines（推荐）：异步文件 I/O
✅ Concepts（可选）：类型约束
✅ Modules（可选）：模块化工具库

示例：
------------------------------------------------------------
// 使用 Ranges 处理文件
auto process_log_file(const string& path) {
    auto lines = read_lines(path);
    return lines
        | views::filter([](auto& line) { return line.contains("ERROR"); })
        | views::transform([](auto& line) { return parse_error(line); });
}
)" << endl;

    // --------------------------------------------
    // 4.2 迁移策略
    // --------------------------------------------

    cout << "\n【实战2】迁移策略" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
从传统 C++ 迁移到 C++20
════════════════════════════════════════════════════════════

阶段1：引入 Concepts（1-2 周）
────────────────────────────────────────────────────────────
目标：提升类型安全，改善错误提示

步骤：
1. 识别使用 SFINAE 的代码
2. 用 Concepts 替代 enable_if
3. 为模板函数添加类型约束
4. 测试编译错误信息是否改善

示例：
------------------------------------------------------------
// 旧代码（SFINAE）
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
T add(T a, T b) { return a + b; }

// 新代码（Concepts）
template<std::integral T>
T add(T a, T b) { return a + b; }

────────────────────────────────────────────────────────────

阶段2：引入 Ranges（2-3 周）
────────────────────────────────────────────────────────────
目标：提升数据处理效率，简化代码

步骤：
1. 识别复杂的 STL 算法调用
2. 用 Ranges 管道操作替代
3. 优化性能关键部分
4. 测试性能改善

示例：
------------------------------------------------------------
// 旧代码（传统 STL）
vector<int> temp1, temp2, result;
copy_if(vec.begin(), vec.end(), back_inserter(temp1),
        [](int x) { return x > 0; });
transform(temp1.begin(), temp1.end(), back_inserter(temp2),
          [](int x) { return x * 2; });
copy_n(temp2.begin(), 5, back_inserter(result));

// 新代码（Ranges）
auto result = vec
    | views::filter([](int x) { return x > 0; })
    | views::transform([](int x) { return x * 2; })
    | views::take(5);

────────────────────────────────────────────────────────────

阶段3：引入 Coroutines（3-5 周）
────────────────────────────────────────────────────────────
目标：改善异步编程体验

步骤：
1. 识别回调地狱的代码
2. 用协程替代回调
3. 实现生成器替代手动迭代器
4. 测试异步代码可读性

示例：
------------------------------------------------------------
// 旧代码（回调）
fetch_data(url, [](auto data) {
    process_data(data, [](auto result) {
        save_result(result, [](bool success) {
            if (success) { /* ... */ }
        });
    });
});

// 新代码（协程）
Task<void> async_workflow() {
    auto data = co_await fetch_data(url);
    auto result = co_await process_data(data);
    bool success = co_await save_result(result);
    if (success) { /* ... */ }
}

────────────────────────────────────────────────────────────

阶段4：引入 Modules（按需）
────────────────────────────────────────────────────────────
目标：提升编译速度，改善代码组织

步骤：
1. 评估编译时间瓶颈
2. 选择独立的模块进行迁移
3. 逐步将头文件转换为模块
4. 测试编译速度改善

注意：
- Modules 迁移成本较高
- 需要编译器良好支持
- 建议新项目优先使用
)" << endl;

    // --------------------------------------------
    // 4.3 常见陷阱和解决方案
    // --------------------------------------------

    cout << "\n【实战3】常见陷阱和解决方案" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
陷阱1：过度使用 Concepts
════════════════════════════════════════════════════════════
问题：为每个函数都定义概念，增加复杂度

解决方案：
- 只在需要类型约束的地方使用
- 简单函数直接用 auto 或具体类型
- 复用标准库概念（std::integral、std::ranges::range 等）

────────────────────────────────────────────────────────────

陷阱2：Ranges 视图的生命周期问题
════════════════════════════════════════════════════════════
问题：视图引用的数据被销毁，导致悬空引用

❌ 错误示例：
------------------------------------------------------------
auto get_filtered_data() {
    vector<int> temp = {1, 2, 3, 4, 5};
    return temp | views::filter([](int x) { return x > 2; });
    // 错误：temp 被销毁，返回的视图引用悬空数据
}

✅ 正确示例：
------------------------------------------------------------
auto get_filtered_data() {
    vector<int> temp = {1, 2, 3, 4, 5};
    // 方案1：转换为容器
    return temp | views::filter([](int x) { return x > 2; })
                | ranges::to<vector>();

    // 方案2：返回容器本身
    vector<int> result;
    ranges::copy(temp | views::filter([](int x) { return x > 2; }),
                 back_inserter(result));
    return result;
}

────────────────────────────────────────────────────────────

陷阱3：Coroutines 的复杂性
════════════════════════════════════════════════════════════
问题：自己实现 promise_type 和 awaiter 太复杂

解决方案：
- 使用现成的库（cppcoro、libunifex）
- 先用库提供的类型（Task、Generator）
- 理解原理后再自己实现

────────────────────────────────────────────────────────────

陷阱4：Modules 的编译器兼容性
════════════════════════════════════════════════════════════
问题：不同编译器对 Modules 的支持不同

解决方案：
- 检查编译器版本和支持情况
- 使用 CMake 3.20+ 管理模块编译
- 新项目优先使用，现有项目谨慎迁移
- 保留头文件作为备选方案
)" << endl;

    // --------------------------------------------
    // 4.4 总结
    // --------------------------------------------

    cout << "\n【总结】C++20 特性知识地图" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【核心要点】

1. 四大特性互补：
   - Concepts：类型安全层
   - Ranges：数据处理层
   - Coroutines：异步编程层
   - Modules：代码组织层

2. 选择策略：
   - 新项目：优先使用所有特性
   - 现有项目：渐进式引入
   - 库开发：Concepts + Modules
   - 数据处理：Ranges + Concepts
   - 异步编程：Coroutines + Ranges

3. 学习路径：
   - 第一步：Concepts（类型安全）
   - 第二步：Ranges（数据处理）
   - 第三步：Modules（代码组织）
   - 第四步：Coroutines（异步编程）

4. 实战建议：
   - 不要过度使用
   - 注意编译器支持
   - 关注性能影响
   - 持续学习和实践

【下一步】

完成知识地图学习后，建议：
1. 创建综合实践项目
2. 应用多个 C++20 特性
3. 理解特性之间的协作
4. 积累实战经验

祝你在 C++20 的学习之路上越走越远！🚀
)" << endl;

    cout << "\n程序结束" << endl;
    return 0;
}
