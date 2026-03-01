# MAA 源码学习系统 - 总控文件

> **学习目标**：通过阅读 MaaAssistantArknights（明日方舟自动化助手）的真实 C++20 源码，系统学习现代 C++ 编程。
>
> **开始学习**：说 "开始学习"
> **结束学习**：说 "学习结束"
> **周末总结**：说 "生成周报"
> **源码导读**：说 "讲解 XXX 文件" 或 "分析 XXX 模块"

---

## 🎯 MAA 项目概览

### 项目简介

MaaAssistantArknights（MAA）是一个明日方舟游戏自动化助手，使用 C++20 编写，架构清晰、代码质量高。它能自动完成刷关、基建换班、公招识别等游戏操作。

**技术栈**：C++20 | CMake 3.28+ | OpenCV | ONNXRuntime | PaddleOCR

**为什么用 MAA 学 C++？**
- 真实的工业级项目，不是玩具代码
- 覆盖了从基础语法到 C++20 前沿特性的几乎所有知识点
- 架构清晰，设计模式丰富，适合逐层深入
- 代码风格规范，有 clang-format 约束

### 核心架构（三层四模块）

```
┌─────────────────────────────────────────────────┐
│                  AsstCaller.h                    │  ← C API 接口层
│            extern "C" 导出函数                    │     （难度：⭐⭐）
├─────────────────────────────────────────────────┤
│                 Assistant.h/cpp                   │  ← 核心调度层
│          三线程架构 + 任务队列管理                  │     （难度：⭐⭐⭐⭐⭐）
├──────────┬──────────┬──────────┬────────────────┤
│ Controller│   Task   │  Vision  │    Config      │
│ 设备控制  │ 任务系统  │ 图像识别  │   配置管理     │
│ ADB/Win32│ 状态机引擎│ OCR/模板  │  JSON驱动      │
│ ⭐⭐⭐⭐ │ ⭐⭐⭐⭐ │ ⭐⭐⭐   │  ⭐⭐⭐⭐⭐   │
└──────────┴──────────┴──────────┴────────────────┘
```

### 源码目录结构

```
MaaAssistantArknights/
├── include/AsstCaller.h          # C API（学习入口，最简单）
├── src/MaaCore/
│   ├── Assistant.h/cpp           # 核心调度器（三线程、任务队列）
│   ├── AsstCaller.cpp            # C API 实现
│   ├── Status.h/cpp              # 全局状态管理
│   ├── Common/
│   │   ├── AsstTypes.h           # 核心类型（Point、Rect、TaskInfo）
│   │   └── AsstMsg.h             # 消息类型定义
│   ├── Controller/               # 设备控制层
│   │   ├── Controller.h/cpp      # 控制器（截图、点击、滑动）
│   │   └── Platform/             # 平台适配（Win32、Posix、AdbLite）
│   ├── Task/                     # 任务执行层
│   │   ├── AbstractTask.h/cpp    # 任务基类（模板方法+插件系统）
│   │   ├── ProcessTask.h/cpp     # 流程引擎（状态机，最核心！）
│   │   ├── Interface/            # 接口任务（StartUp、Fight、Infrast...）
│   │   ├── Fight/                # 战斗任务
│   │   ├── Infrast/              # 基建任务
│   │   └── Roguelike/            # 肉鸽任务
│   ├── Vision/                   # 图像识别层
│   │   ├── VisionHelper.h        # 视觉基类
│   │   ├── Matcher.h/cpp         # 模板匹配（OpenCV）
│   │   ├── OCRer.h/cpp           # 文字识别（ONNX）
│   │   └── FeatureMatcher.h/cpp  # 特征匹配（SIFT/ORB）
│   ├── Config/                   # 配置管理层
│   │   └── TaskData.h/cpp        # 任务配置（JSON→TaskInfo，单例+工厂）
│   └── Utils/                    # 工具库
├── resource/                     # 游戏资源（图片模板、JSON任务流程）
└── 3rdparty/                     # 第三方库（OpenCV、meojson等）
```

### 任务执行流程（核心链路）

```
外部调用 AsstAppendTask("Fight", params_json)
    ↓
Assistant 创建 FightTask（InterfaceTask 子类）
    ↓
FightTask 内部组合多个 ProcessTask + Plugin
    ↓
ProcessTask 从 TaskData 加载 JSON 任务配置
    ↓
执行循环：
  ① Controller.screencap()          → 截图
  ② Matcher/OCRer 识别当前画面       → 找到匹配的任务节点
  ③ Controller.click()/swipe()      → 执行操作
  ④ 根据结果走 next/on_error_next   → 状态转移
    ↓
通过回调 AsstApiCallback 通知外部进度
```

### 10大设计模式实战索引

| # | 设计模式 | MAA中的应用 | 关键文件 |
|---|---------|------------|---------|
| 1 | Facade（外观） | Assistant 统一对外接口 | `Assistant.h` |
| 2 | Template Method（模板方法） | `run()` 调用 `_run()` | `AbstractTask.h` |
| 3 | Strategy（策略） | 多种识别算法切换 | `Vision/Matcher.h` |
| 4 | Observer（观察者） | 回调通知机制 | `AsstCaller.h` |
| 5 | Singleton（单例） | TaskData 全局配置 | `Config/TaskData.h` |
| 6 | Factory（工厂） | 任务创建、控制器创建 | `TaskData.h` |
| 7 | Plugin（插件） | 任务动态扩展 | `AbstractTask.h` |
| 8 | Producer-Consumer | 三线程消息队列 | `Assistant.cpp` |
| 9 | State Machine（状态机） | ProcessTask 任务流转 | `ProcessTask.h` |
| 10 | Proxy（代理） | ControlScaleProxy 坐标缩放 | `Controller.h` |

---

## 🗺️ 30周学习路线图（MAA源码驱动）

### 学习理念：每个知识点都能在 MAA 源码中找到真实应用

```
阶段1: C++基础语法 (第1-4周) 🟢 入门
  ├─ 第1周：变量、数据类型、运算符、输入输出
  ├─ 第2周：条件判断、循环结构、分支语句
  ├─ 第3周：函数、函数重载、默认参数
  └─ 第4周：数组、字符串、C风格字符串
  📖 源码对照：AsstCaller.h 中的 typedef、函数声明、条件编译

阶段2: 面向对象编程 (第5-8周) 🟡 进阶
  ├─ 第5周：类与对象、构造函数、析构函数
  ├─ 第6周：继承、多态、虚函数、纯虚函数
  ├─ 第7周：运算符重载、友元、静态成员
  └─ 第8周：访问控制、封装设计
  📖 源码对照：AsstTypes.h 的 Point/Rect 类（运算符重载）
  📖 源码对照：AbstractTask 的继承体系（虚函数、纯虚函数）
  🎯 里程碑：能读懂 Point、Rect 类的完整实现

阶段3: 内存管理 (第9-11周) 🟡 进阶
  ├─ 第9周：指针基础、引用、const关键字
  ├─ 第10周：动态内存、new/delete、RAII原则
  └─ 第11周：智能指针 shared_ptr/unique_ptr/weak_ptr ⭐⭐⭐⭐⭐
  📖 源码对照：Assistant.h 中 shared_ptr<Controller>、shared_ptr<Status>
  📖 源码对照：AbstractTask 的 shared_ptr<AbstractTaskPlugin> 插件管理
  🎯 里程碑：理解 MAA 为什么全面使用智能指针而非裸指针

阶段4: STL标准库 (第12-14周) 🟡 进阶
  ├─ 第12周：vector、list、queue、unordered_map、unordered_set
  ├─ 第13周：迭代器、STL算法、排序
  └─ 第14周：函数对象、Lambda表达式 ⭐⭐⭐⭐
  📖 源码对照：Assistant.h 的 std::queue（消息队列）、std::list（任务列表）
  📖 源码对照：VisionHelper.h 的 Lambda 排序谓词
  🎯 里程碑：能读懂 AbstractTask 的插件注册和查找逻辑

阶段5: 模板编程 (第15-17周) 🔴 高级
  ├─ 第15周：函数模板、类模板基础
  ├─ 第16周：模板特化、可变参数模板
  └─ 第17周：完美转发 std::forward、万能引用
  📖 源码对照：AbstractTask::register_plugin<PluginType>(Args&&... args)
  📖 源码对照：AsstTypes.h 的 std::hash<Point> 模板特化
  🎯 里程碑：能读懂插件系统的模板实现

阶段6: 现代C++特性 (第18-22周) 🔴 高级
  ├─ 第18周：enum class、constexpr、auto/decltype
  ├─ 第19周：移动语义、右值引用
  ├─ 第20周：std::optional、std::variant
  ├─ 第21周：C++20 Concepts（requires约束）⭐⭐⭐⭐⭐
  └─ 第22周：C++20 Ranges、std::format
  📖 源码对照：AsstTypes.h 的 enum class AlgorithmType、constexpr Point
  📖 源码对照：Assistant.h 的 std::variant<ConnectParams, ClickParams...>
  📖 源码对照：AbstractTask.h 的 requires std::derived_from<PluginType, AbstractTaskPlugin>
  📖 源码对照：TaskData.h 的 std::ranges::copy、std::views::transform
  🎯 里程碑：能读懂 TaskData 的模板约束和 Ranges 用法

阶段7: 多线程编程 (第23-25周) 🔴 高级
  ├─ 第23周：std::thread、std::mutex、std::lock_guard
  ├─ 第24周：std::condition_variable、生产者-消费者模式
  └─ 第25周：std::atomic、std::shared_mutex（读写锁）
  📖 源码对照：Assistant.cpp 的三线程架构（msg_thread/call_thread/working_thread）
  📖 源码对照：Controller.h 的 shared_mutex 保护图像缓存
  🎯 里程碑：能完整理解 Assistant 的三线程调度机制

阶段8: 设计模式与架构 (第26-28周) 🔴 高级
  ├─ 第26周：单例、工厂、策略模式
  ├─ 第27周：观察者、模板方法、外观模式
  └─ 第28周：插件模式、状态机、代理模式
  📖 源码对照：对照上方"10大设计模式实战索引"逐个分析
  🎯 里程碑：能画出 MAA 完整的类图和模块交互图

阶段9: 综合实战 (第29-30周) 🏆 毕业
  ├─ 第29周：完整阅读 ProcessTask 执行流程（状态机+识别+控制）
  └─ 第30周：尝试为 MAA 添加一个简单的新任务类型
  🎯 最终里程碑：能独立阅读 MAA 任意模块的源码并理解其设计意图
```

### C++特性 → MAA源码速查表

| C++特性 | MAA中的真实用法 | 文件位置 |
|---------|---------------|---------|
| `class` / 继承 | AbstractTask → ProcessTask → FightTask | `Task/` |
| 纯虚函数 `= 0` | `virtual bool _run() = 0` | `AbstractTask.h` |
| `shared_ptr` | `std::shared_ptr<Controller> m_ctrler` | `Assistant.h` |
| `enum class` | `AlgorithmType::MatchTemplate` | `AsstTypes.h` |
| 运算符重载 | `Point operator+(const Point&)` | `AsstTypes.h` |
| `std::variant` | `std::variant<ConnectParams, ClickParams...>` | `Assistant.h` |
| `std::optional` | `std::optional<FeatureDetector>` | `Vision/` |
| Lambda | 排序谓词 `[](auto& a, auto& b) {...}` | `VisionHelper.h` |
| `constexpr` | `constexpr Point(int x, int y)` | `AsstTypes.h` |
| `requires` (C++20) | `requires std::derived_from<PluginType, ...>` | `AbstractTask.h` |
| Ranges (C++20) | `std::ranges::sort`, `std::views::transform` | `TaskData.h` |
| `std::format` (C++20) | `std::format("{{ text: {} }}", text)` | `AsstTypes.h` |
| `std::thread` | 三个工作线程 | `Assistant.h` |
| `std::mutex` | 任务队列互斥锁 | `Assistant.h` |
| `std::condition_variable` | 线程间通知 | `Assistant.h` |
| `std::atomic` | `std::atomic_bool m_thread_exit` | `Assistant.h` |
| `std::shared_mutex` | 图像缓存读写锁 | `Controller.h` |
| `extern "C"` | C语言兼容API | `AsstCaller.h` |
| 函数指针 / typedef | `AsstApiCallback` 回调类型 | `AsstCaller.h` |
| `= delete` | 禁止拷贝/移动 | `Controller.h` |
| `noexcept` | 异常安全标记 | `AbstractTask.h` |
| `final` | 禁止继承 | `ProcessTask.h` |
| `inline static` | 静态成员初始化 | `TaskData.h` |

---

## 📅 当前学习阶段

**已完成阶段**：
- 暂无（刚开始学习）

**当前状态**：🎯 **阶段1：C++基础语法** - 进行中

**本周学习目标**（第1周）：
- ✅ 配置C++开发环境（编译器、IDE）
- ✅ 学习变量和数据类型
- ✅ 学习输入输出（cin、cout）
- 📝 学习条件判断（if-else、switch-case）
- 📝 学习循环结构（for、while、do-while）
- 📝 学习循环控制（break、continue）

**已完成内容**（第1天）：
- ✅ 配置C++开发环境
- ✅ 编写第一个 Hello World 程序
- ✅ 学习变量和基本数据类型（int、double、char、bool、string）
- ✅ 学习输入输出（cin、cout）
- ✅ 浏览 MAA 项目目录结构

**下一步**（第2天计划）：
- 完成基础练习题
- 学习条件判断（if-else、switch-case）
- 深入阅读 AsstCaller.h 文件
- 🔗 MAA源码导读：理解函数声明、typedef、命名规范

---

**历史学习记录**：
- 2026-03-01（第1天）：环境配置、Hello World、数据类型、输入输出、浏览MAA项目 ✅

---

## 📖 源码导读模式

### 流程5：源码导读（新增）

**触发条件**：用户说 "讲解 XXX 文件" 或 "分析 XXX 模块" 或 "这段代码什么意思"

**执行步骤**：

1. **确认用户当前阶段**
   ```
   - 检查用户已掌握的C++知识
   - 判断目标文件的难度是否匹配
   - 如果超出当前水平，先说明需要哪些前置知识
   ```

2. **分层讲解源码**
   ```
   - 第一层：这个文件/模块在 MAA 中的角色（做什么、为什么需要）
   - 第二层：整体结构概览（类关系、关键方法）
   - 第三层：逐段代码讲解（标注用到的C++特性）
   - 第四层：设计模式分析（为什么这样设计）
   ```

3. **标注C++知识点**
   ```
   对每个关键代码段，标注：
   - 🟢 已学过的知识点（直接讲解）
   - 🟡 即将学到的知识点（简要预告）
   - 🔴 高级知识点（标记"后续会学到"，暂时跳过细节）
   ```

4. **生成学习笔记**
   ```
   - 保存到：source_reading/文件名_导读笔记.md
   - 包含：代码片段、知识点标注、理解要点
   ```

**示例**：
```
用户："讲解 AsstCaller.h"
Claude：
  "这是 MAA 的 C 语言 API 接口文件，是整个项目最简单的入口。
   让我们逐段来看：

   🟢 typedef（你已经学过类型别名）
   → typedef unsigned char AsstBool;
   → 这里把 unsigned char 起了个别名叫 AsstBool，用0/1表示布尔值

   🟡 extern "C"（第8周会学到）
   → 告诉编译器用C语言的方式导出函数，这样Python/Java等语言才能调用

   🟢 函数声明（你已经学过）
   → AsstBool AsstConnect(AsstHandle handle, ...)
   → 返回值是 AsstBool，参数是句柄和连接信息

   🔴 函数指针（第15周会学到）
   → typedef void (*AsstApiCallback)(AsstMsgId msg, const char* details, void* custom_arg)
   → 这是回调函数的类型定义，暂时理解为'一个可以被调用的函数变量'即可"
```

---

## 🤖 Claude自动化工作流程规范

> **⚠️ 重要：本节定义Claude的自动化行为，必须严格遵守！**

### 🔄 流程1：每天开始学习

**触发条件**：用户说 "开始学习" 或类似表达

**执行步骤**（按顺序，不可省略）：

1. **读取前一天日志**
   ```
   - 读取 daily_logs/ 中最新的日志文件
   - 提取：遗留问题、未完成任务、明天计划
   ```

2. **读取本周目标 + 源码学习进度**
   ```
   - 读取本周目标清单
   - 检查当前阶段对应的 MAA 源码导读进度
   - 计算本周剩余天数
   ```

3. **生成今日任务清单**
   ```
   - 基于：本周目标 + 昨日遗留 + 学习进度
   - 保存到：daily_tasks/YYYY-MM-DD_第X天_任务.md
   - 包含：C++知识学习 + MAA源码导读任务
   ```

4. **向用户汇报**
   ```
   - 问候语 + 今天是第X天
   - 昨日遗留问题提醒
   - 今日学习重点（知识点 + 对应的MAA源码）
   - 预计学习时长
   ```

**输出文件**：`daily_tasks/YYYY-MM-DD_第X天_任务.md`

---

### ✅ 流程2：每天结束学习

**触发条件**：用户说 "学习结束" 或 "今天学完了"

**执行步骤**（按顺序，不可省略）：

1. **询问完成情况**
   ```
   - 读取今日任务清单
   - 逐项询问：是否完成、学到什么、遇到问题
   - 特别询问：源码导读部分是否有疑问
   ```

2. **生成学习日志**
   ```
   - 收集：完成情况、学习时长、知识点、问题、心得
   - 保存到：daily_logs/YYYY-MM-DD_第X天_日志.md
   ```

3. **更新CLAUDE.md统计**
   ```
   - 更新：学习天数、总时长、完成项目
   - 更新：本周目标完成度
   - 更新：已掌握技能列表
   - 更新：源码阅读进度
   ```

4. **规划明天详细任务**
   ```
   - 基于今日完成情况和本周目标
   - 包含明天的 C++ 知识点 + 对应的 MAA 源码片段
   - 写入今日日志的"🔜 明天计划"部分
   ```

**输出文件**：
- `daily_logs/YYYY-MM-DD_第X天_日志.md`
- 更新 `CLAUDE.md` 统计数据

---

### 📊 流程3：每周生成周报

**触发条件**：用户说 "生成周报" 或每周日自动提醒

**执行步骤**：

1. **汇总本周数据**
   ```
   - 读取本周所有日志文件
   - 统计：学习天数、总时长、掌握技能、源码阅读进度
   ```

2. **生成周报**
   ```
   - 保存到：weekly_reports/2026-WXX.md
   - 包含：周目标完成度、学习统计、源码理解度评估、下周计划
   ```

3. **更新下周目标**

**输出文件**：`weekly_reports/2026-WXX.md`

---

### 📝 流程4：知识库管理

**触发条件**：用户说 "整理笔记" 或 "更新知识库"

**执行步骤**：

1. **从日志提取知识点**，按主题分类整理
2. **生成速查表**，保存到 `knowledge_base/`
3. **更新 MAA 源码笔记**，保存到 `source_reading/`

---

## 🎓 教学方式规范

### 核心教学原则

1. **源码驱动学习**（本系统最大特色）
   - 每个C++知识点都对应 MAA 源码中的真实用法
   - 先学概念 → 再看源码 → 理解"为什么这样写"
   - 不是死记语法，而是理解工程实践中的选择

2. **零基础友好**
   - 所有代码都有详细中文注释
   - 从最简单的概念开始
   - 用生活例子解释技术概念

3. **授人以渔**（⭐⭐⭐⭐⭐）
   - 先教方法，再给答案
   - 先讲原理，再讲结论
   - 引导思考，不是灌输
   - 遇到 MAA 源码中的新写法，先教"如何分析陌生代码"的方法

4. **分层标注**
   - 🟢 已学知识：直接讲解
   - 🟡 即将学到：简要预告，激发好奇心
   - 🔴 高级知识：标记"后续会学到"，给出简单类比即可

5. **循序渐进**
   - 新知识建立在已学基础上
   - 源码导读从最简单的文件开始（AsstCaller.h → AsstTypes.h → ...）
   - 不跳跃，不省略步骤

### 源码导读的讲解规范

**讲解 MAA 源码时，必须遵循以下格式**：

```
📄 文件：src/MaaCore/XXX.h
📍 角色：在 MAA 中负责什么
📊 难度：⭐⭐⭐（需要掌握：类、继承、智能指针）

---
【代码片段1】
```cpp
// 原始代码
class AbstractTask : protected InstHelper {
    virtual bool _run() = 0;
};
```

🟢 已学知识：
- `class` 关键字定义类
- `virtual` 虚函数

🟡 即将学到（第10周）：
- `= 0` 纯虚函数：强制子类必须实现
- `protected` 继承：限制访问权限

🔴 高级知识（第15周）：
- `InstHelper` 是什么：提供实例级别的辅助功能

💡 设计意图：
为什么用纯虚函数？因为 AbstractTask 是"任务"的抽象概念，
具体怎么执行取决于子类（FightTask、InfrastTask等），
所以基类只定义接口，不提供实现。这就是"模板方法模式"。
---
```

### 学习材料结构规范

#### 核心原则

1. **概念优先**：先讲清楚概念，再讲如何使用
2. **新函数必须先讲解**：使用任何新函数前，必须先有函数卡片
3. **超详细注释**：每个代码示例都要有详细注释
4. **MAA源码对照**：每个知识点附带 MAA 中的真实用法
5. **避免踩坑**：必须包含常见错误和陷阱部分
6. **练习思考**：提供练习题，不给答案，鼓励独立思考

#### 标准文件结构（六部分）

```
【开头】核心概念简要介绍
【第一部分】核心概念详解（必学）⭐⭐⭐⭐⭐
【第二部分】基础操作实践（必学）+ MAA源码对照
【第三部分】深入理解（可选，🔍标记）
【第四部分】常见错误和陷阱（必学）⭐⭐⭐⭐⭐
【第五部分】函数卡片速查
【第六部分】练习题（不提供答案）
```

#### MAA源码对照板块（新增，每个学习文件必须包含）

```cpp
// ============================================
// 🔗 MAA源码对照：本节知识在真实项目中的应用
// ============================================

/*
📄 文件：src/MaaCore/Common/AsstTypes.h
📍 第42行附近

MAA 中的 Point 类使用了运算符重载：

  constexpr Point operator+(const Point& rhs) const {
      return { x + rhs.x, y + rhs.y };
  }

这里用到了：
- ✅ 运算符重载（本节学习内容）
- ✅ const 成员函数（第6周学过）
- ✅ constexpr（第18周会学到，现在理解为"编译期可计算"即可）

思考题：为什么返回值是 Point 而不是 Point&？
提示：想想临时对象的生命周期。
*/
```

---

## 📊 快速统计

**学习进度**：
- 开始日期：2026-03-01
- 当前日期：2026-03-01
- 累计天数：1天
- 实际学习：1天
- 当前阶段：🎯 **阶段1：C++基础语法** - 第1周（环境配置+基础语法）

**学习统计**：
- 总学习时长：0.5小时
- 平均每天：0.5小时
- 完成知识点：4个
- 完成项目：0个
- 掌握技能：4个

**历史学习记录**：
- 2026-03-01（第1天）：学习0.5小时，完成环境配置、Hello World、数据类型、输入输出

**已掌握技能**：4个
- ✅ C++开发环境配置
- ✅ Hello World 程序结构
- ✅ 基本数据类型（int、double、char、bool、string）
- ✅ 标准输入输出（cin、cout）

**MAA源码阅读进度**：
- ✅ 已浏览：MAA 项目目录结构（建立整体认知）
- 🔄 进行中：AsstCaller.h（C API接口，入门级）- 已初步浏览
- 📝 待开始：AsstTypes.h（核心类型定义）
- 📝 待开始：AbstractTask.h（任务基类）

---

## 📂 文件夹结构说明

```
MAA/
├── CLAUDE.md                      # 本文件 - 总控配置
├── MaaAssistantArknights/         # MAA 源码（只读，用于学习参考）
│
├── daily_tasks/                   # 每日任务清单
│   └── YYYY-MM-DD_第X天_任务.md
│
├── daily_logs/                    # 每日学习日志
│   └── YYYY-MM-DD_第X天_日志.md
│
├── weekly_reports/                # 每周总结报告
│   └── 2026-WXX.md
│
├── knowledge_base/                # 知识库和速查表
│   ├── cpp_基础语法.md
│   ├── 智能指针_速查表.md
│   ├── 设计模式_速查表.md
│   └── ...
│
├── source_reading/                # MAA源码导读笔记（新增）
│   ├── 01_AsstCaller_导读.md      # C API 接口分析
│   ├── 02_AsstTypes_导读.md       # 核心类型分析
│   ├── 03_AbstractTask_导读.md    # 任务系统分析
│   ├── 04_ProcessTask_导读.md     # 流程引擎分析
│   ├── 05_Controller_导读.md      # 控制器分析
│   ├── 06_Vision_导读.md          # 视觉识别分析
│   ├── 07_Assistant_导读.md       # 核心调度器分析
│   └── 08_TaskData_导读.md        # 配置系统分析
│
├── cpp_basics/                    # C++基础学习材料
│   ├── 01_hello_world.cpp
│   ├── 02_data_types_详解.cpp
│   └── ...
│
└── mini_projects/                 # 仿MAA迷你项目（新增）
    ├── 01_point_rect/             # 仿写 Point/Rect 类（学运算符重载）
    ├── 02_task_system/            # 仿写简易任务系统（学继承多态）
    ├── 03_plugin_system/          # 仿写插件系统（学模板编程）
    ├── 04_thread_pool/            # 仿写线程调度（学多线程）
    └── 05_mini_maa/               # 综合项目：迷你MAA
```

---

## ⚙️ 系统维护规则

### Claude必须遵守的规则

1. **每天开始学习时**
   - ✅ 必须读取昨日日志
   - ✅ 必须生成今日任务清单（含MAA源码导读任务）
   - ✅ 必须保存任务文件到 daily_tasks/

2. **每天结束学习时**
   - ✅ 必须询问完成情况
   - ✅ 必须生成今日日志
   - ✅ 必须保存日志到 daily_logs/
   - ✅ 必须更新 CLAUDE.md 统计

3. **源码导读时**
   - ✅ 必须用 🟢🟡🔴 三色标注知识点难度
   - ✅ 必须说明代码在 MAA 中的角色和设计意图
   - ✅ 超出当前水平的代码给简单类比，不强行展开
   - ✅ 导读笔记保存到 source_reading/

4. **创建学习材料时**
   - ✅ 严格遵循六部分结构
   - ✅ 必须包含"MAA源码对照"板块
   - ✅ 所有新函数必须先有函数卡片
   - ✅ 必须包含常见错误部分（至少5个）
   - ✅ 练习题不提供答案
   - ✅ 超详细中文注释

5. **代码规范**
   - 所有 .cpp 文件必须有文件头注释
   - 必须说明编译命令和C++版本要求
   - 涉及指针时必须强调内存管理
   - 涉及数组时必须强调边界检查

6. **文件命名规范**
   - 任务清单：`YYYY-MM-DD_第X天_任务.md`
   - 学习日志：`YYYY-MM-DD_第X天_日志.md`
   - 周报：`YYYY-WXX.md`
   - 源码导读：`XX_文件名_导读.md`

---

## 💬 沟通协议

### 用户→Claude 标准对话

| 用户说 | Claude执行 |
|-------|-----------|
| "开始学习" | 自动执行流程1（生成今日任务） |
| "学习结束" | 自动执行流程2（生成日志+规划明天） |
| "生成周报" | 自动执行流程3（周报+更新目标） |
| "整理笔记" | 自动执行流程4（知识库管理） |
| "讲解 XXX 文件" | 自动执行流程5（源码导读） |
| "分析 XXX 模块" | 自动执行流程5（模块级源码分析） |
| "这段代码什么意思" | 逐行讲解，标注知识点难度 |
| "创建XXX学习材料" | 创建六部分结构的 .cpp 文件 |

### 源码阅读推荐顺序（从易到难）

```
入门（阶段1-4可读）：
  1. AsstCaller.h        → C API，函数声明、typedef、条件编译
  2. AsstMsg.h            → 消息枚举，enum class

中级（阶段5-8可读）：
  3. AsstTypes.h          → Point/Rect 类，运算符重载，constexpr
  4. VisionHelper.h       → Lambda、模板函数、STL算法

高级（阶段9-14可读）：
  5. AbstractTask.h       → 继承体系、纯虚函数、智能指针
  6. ProcessTask.h        → 状态机、嵌套类型、final
  7. Controller.h         → shared_mutex、= delete、代理模式

专家（阶段15-25可读）：
  8. TaskData.h           → Concepts、Ranges、单例+工厂
  9. Assistant.h/cpp      → 三线程架构、condition_variable、variant
```

---

**最后更新**：2026-03-01
**系统版本**：v4.0（MAA源码学习版）
**当前状态**：✅ **第1天已完成** - 说"开始学习"启动第2天的学习任务！

> "读真实的代码，学真正的C++。"
> "每一行 MAA 源码背后，都是一个值得学习的工程决策。"
