# Qt 综合项目规划 📋

> **目标**：设计并开发一个完整的 Qt 桌面应用，整合所有学过的技术

---

## 📋 目录

1. [项目候选方案](#项目候选方案)
2. [推荐项目：个人任务管理器](#推荐项目个人任务管理器)
3. [功能设计](#功能设计)
4. [技术选型](#技术选型)
5. [架构设计](#架构设计)
6. [开发计划](#开发计划)
7. [预期成果](#预期成果)

---

## 项目候选方案

### 方案1：个人任务管理器 ⭐⭐⭐⭐⭐（推荐）

**项目描述**：
一个功能完善的个人任务管理应用，支持任务分类、优先级、截止日期、提醒、统计等功能。

**核心功能**：
- 任务增删改查
- 任务分类（工作、学习、生活等）
- 优先级设置（高、中、低）
- 截止日期和提醒
- 任务统计和可视化
- 数据导入导出
- 云同步（可选）

**技术整合**：
- ✅ UI：QMainWindow、控件、布局、样式表
- ✅ 数据库：SQLite、QSqlTableModel
- ✅ 网络：云同步（HTTP API）
- ✅ 多线程：后台同步、数据导出

**难度**：⭐⭐⭐⭐（中高）

---

### 方案2：Markdown 笔记应用 ⭐⭐⭐⭐

**项目描述**：
一个支持 Markdown 语法的笔记应用，支持笔记分类、搜索、导出等功能。

**核心功能**：
- Markdown 编辑和预览
- 笔记分类和标签
- 全文搜索
- 导出为 PDF/HTML
- 笔记同步

**技术整合**：
- ✅ UI：QTextEdit、QWebEngineView（Markdown 预览）
- ✅ 数据库：SQLite
- ✅ 网络：笔记同步
- ✅ 多线程：全文搜索、导出

**难度**：⭐⭐⭐⭐（中高）

---

### 方案3：局域网聊天工具 ⭐⭐⭐⭐⭐

**项目描述**：
一个局域网内的即时通讯工具，支持文本消息、文件传输、群聊等功能。

**核心功能**：
- 用户登录和注册
- 一对一聊天
- 群聊
- 文件传输
- 消息历史记录

**技术整合**：
- ✅ UI：QListWidget、QTextEdit
- ✅ 数据库：SQLite（消息历史）
- ✅ 网络：QTcpSocket、QTcpServer
- ✅ 多线程：消息接收、文件传输

**难度**：⭐⭐⭐⭐⭐（高）

---

### 方案4：RSS 阅读器 ⭐⭐⭐

**项目描述**：
一个 RSS 订阅和阅读应用，支持订阅管理、文章阅读、离线缓存等功能。

**核心功能**：
- RSS 订阅管理
- 文章列表和阅读
- 离线缓存
- 文章搜索
- 导出为 PDF

**技术整合**：
- ✅ UI：QListWidget、QTextBrowser
- ✅ 数据库：SQLite（订阅和文章）
- ✅ 网络：HTTP 请求、XML 解析
- ✅ 多线程：RSS 更新、文章下载

**难度**：⭐⭐⭐（中）

---

### 方案5：系统监控工具 ⭐⭐⭐⭐

**项目描述**：
一个系统资源监控工具，实时显示 CPU、内存、磁盘、网络使用情况。

**核心功能**：
- 实时监控 CPU、内存、磁盘、网络
- 历史数据图表
- 进程管理
- 系统信息查看
- 性能报告导出

**技术整合**：
- ✅ UI：QChart（图表）、QTableView
- ✅ 数据库：SQLite（历史数据）
- ✅ 网络：网络流量监控
- ✅ 多线程：实时监控、数据采集

**难度**：⭐⭐⭐⭐（中高）

---

## 推荐项目：个人任务管理器

**选择理由**：
1. ✅ 功能明确，需求清晰
2. ✅ 能够整合所有学过的技术
3. ✅ 实用性强，日常可用
4. ✅ 难度适中，适合综合练习
5. ✅ 可扩展性强，后续可以添加更多功能

---

## 功能设计

### 核心功能（必须实现）

#### 1. 任务管理
- **添加任务**：标题、描述、分类、优先级、截止日期
- **编辑任务**：修改任务信息
- **删除任务**：删除任务（支持批量删除）
- **完成任务**：标记任务为已完成
- **任务列表**：显示所有任务，支持排序和筛选

#### 2. 任务分类
- **预设分类**：工作、学习、生活、其他
- **自定义分类**：用户可以添加自定义分类
- **分类管理**：编辑、删除分类

#### 3. 优先级
- **三个级别**：高、中、低
- **颜色标识**：不同优先级用不同颜色显示

#### 4. 截止日期
- **日期选择**：使用 QDateEdit 选择日期
- **逾期提醒**：逾期任务用红色标识
- **即将到期提醒**：3天内到期的任务用橙色标识

#### 5. 任务统计
- **总任务数**：显示总任务数
- **已完成任务数**：显示已完成任务数
- **完成率**：计算完成率
- **分类统计**：每个分类的任务数
- **优先级统计**：每个优先级的任务数

### 扩展功能（可选实现）

#### 6. 任务搜索
- **关键词搜索**：搜索任务标题和描述
- **高级筛选**：按分类、优先级、状态筛选

#### 7. 数据导入导出
- **导出为 CSV**：导出任务列表
- **导出为 JSON**：导出任务数据
- **导入 CSV/JSON**：导入任务数据

#### 8. 云同步（高级）
- **账号系统**：用户注册和登录
- **数据同步**：将任务同步到云端
- **多设备同步**：在多个设备间同步任务

#### 9. 任务提醒（高级）
- **系统通知**：到期前弹出系统通知
- **邮件提醒**：发送邮件提醒

#### 10. 数据可视化
- **任务趋势图**：显示任务完成趋势
- **分类饼图**：显示各分类任务占比
- **优先级柱状图**：显示各优先级任务数量

---

## 技术选型

### UI 技术
- **QMainWindow**：主窗口框架
- **QTableView + QSqlTableModel**：任务列表显示
- **QDialog**：添加/编辑任务对话框
- **QDateEdit**：日期选择
- **QComboBox**：分类和优先级选择
- **Qt 样式表**：美化界面（深色主题）
- **QMenuBar + QToolBar**：菜单栏和工具栏
- **QStatusBar**：状态栏

### 数据库技术
- **SQLite**：本地数据库
- **QSqlDatabase**：数据库连接
- **QSqlQuery**：SQL 查询
- **QSqlTableModel**：Model-View 架构
- **事务**：保证数据一致性

### 网络技术（可选）
- **QNetworkAccessManager**：HTTP 请求
- **QJsonDocument**：JSON 数据交换
- **RESTful API**：云同步接口

### 多线程技术
- **QtConcurrent::run()**：后台任务（数据导出、同步）
- **QFutureWatcher**：监听任务完成

### 其他技术
- **QFile**：文件读写（导入导出）
- **QSettings**：保存用户设置
- **QSystemTrayIcon**：系统托盘图标（可选）

---

## 架构设计

### 模块划分

```
TaskManager/
├── main.cpp                    # 程序入口
├── MainWindow.h/cpp            # 主窗口
├── TaskDialog.h/cpp            # 添加/编辑任务对话框
├── CategoryDialog.h/cpp        # 分类管理对话框
├── StatisticsWidget.h/cpp      # 统计面板
├── DatabaseManager.h/cpp       # 数据库管理类
├── Task.h/cpp                  # 任务数据类
├── Category.h/cpp              # 分类数据类
├── TaskModel.h/cpp             # 任务模型（QSqlTableModel）
├── ExportManager.h/cpp         # 导入导出管理类
├── SyncManager.h/cpp           # 云同步管理类（可选）
└── resources/                  # 资源文件
    ├── icons/                  # 图标
    └── styles/                 # 样式表
```

### 类设计

#### 1. Task 类（任务数据类）
```cpp
class Task {
public:
    int id;                     // 任务ID
    QString title;              // 标题
    QString description;        // 描述
    int categoryId;             // 分类ID
    int priority;               // 优先级（1-高，2-中，3-低）
    QDate dueDate;              // 截止日期
    bool completed;             // 是否完成
    QDateTime createdAt;        // 创建时间
    QDateTime updatedAt;        // 更新时间
};
```

#### 2. Category 类（分类数据类）
```cpp
class Category {
public:
    int id;                     // 分类ID
    QString name;               // 分类名称
    QString color;              // 分类颜色
};
```

#### 3. DatabaseManager 类（数据库管理类）
```cpp
class DatabaseManager {
public:
    static DatabaseManager& instance();
    
    bool initialize();
    bool addTask(const Task &task);
    bool updateTask(const Task &task);
    bool deleteTask(int taskId);
    QList<Task> getAllTasks();
    QList<Task> getTasksByCategory(int categoryId);
    
    bool addCategory(const Category &category);
    bool updateCategory(const Category &category);
    bool deleteCategory(int categoryId);
    QList<Category> getAllCategories();
    
    // 统计
    int getTotalTaskCount();
    int getCompletedTaskCount();
    QMap<int, int> getTaskCountByCategory();
    QMap<int, int> getTaskCountByPriority();
};
```

#### 4. MainWindow 类（主窗口）
```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    
private slots:
    void onAddTask();
    void onEditTask();
    void onDeleteTask();
    void onCompleteTask();
    void onFilterChanged();
    void onExportData();
    void onImportData();
    void onShowStatistics();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void loadTasks();
    void updateStatistics();
    
    QTableView *taskTableView;
    QSqlTableModel *taskModel;
    QComboBox *categoryFilter;
    QComboBox *priorityFilter;
    QLabel *statsLabel;
};
```

### 数据库设计

#### 表1：tasks（任务表）
```sql
CREATE TABLE tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    description TEXT,
    category_id INTEGER,
    priority INTEGER DEFAULT 2,
    due_date TEXT,
    completed INTEGER DEFAULT 0,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (category_id) REFERENCES categories(id)
);
```

#### 表2：categories（分类表）
```sql
CREATE TABLE categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    color TEXT DEFAULT '#3498db'
);
```

---

## 开发计划

### 第1阶段：基础框架（1-2天）
- [ ] 创建项目结构
- [ ] 设计数据库表
- [ ] 实现 DatabaseManager 类
- [ ] 创建 Task 和 Category 数据类
- [ ] 实现 MainWindow 基本框架

### 第2阶段：核心功能（2-3天）
- [ ] 实现任务增删改查
- [ ] 实现任务列表显示（QTableView + QSqlTableModel）
- [ ] 实现任务对话框（添加/编辑）
- [ ] 实现分类管理
- [ ] 实现优先级和截止日期

### 第3阶段：UI 美化（1天）
- [ ] 设计深色主题样式表
- [ ] 添加图标
- [ ] 优化布局
- [ ] 添加菜单栏和工具栏

### 第4阶段：统计功能（1天）
- [ ] 实现任务统计
- [ ] 显示统计面板
- [ ] 实现筛选功能

### 第5阶段：导入导出（1天）
- [ ] 实现导出为 CSV
- [ ] 实现导出为 JSON
- [ ] 实现导入 CSV/JSON
- [ ] 使用 QtConcurrent 后台导出

### 第6阶段：测试和优化（1天）
- [ ] 功能测试
- [ ] 性能优化
- [ ] Bug 修复
- [ ] 编写用户文档

### 第7阶段：扩展功能（可选）
- [ ] 实现云同步
- [ ] 实现任务提醒
- [ ] 实现数据可视化
- [ ] 实现系统托盘

---

## 预期成果

### 功能成果
- ✅ 完整的任务管理功能
- ✅ 美观的用户界面
- ✅ 稳定的数据存储
- ✅ 实用的统计功能
- ✅ 便捷的导入导出

### 技术成果
- ✅ 整合了所有学过的 Qt 技术
- ✅ 掌握了完整的项目开发流程
- ✅ 积累了项目架构设计经验
- ✅ 提升了代码组织能力

### 项目展示
- ✅ 可以作为作品集项目
- ✅ 可以发布到 GitHub
- ✅ 可以打包成安装程序
- ✅ 可以继续扩展功能

---

## 🎯 下一步行动

1. **确认项目选择**
   - 确定是否选择"个人任务管理器"
   - 或者选择其他候选项目

2. **细化需求**
   - 确定核心功能范围
   - 确定扩展功能优先级

3. **开始开发**
   - 创建项目结构
   - 实现数据库设计
   - 开发基础框架

---

**准备好开始 Qt 综合项目开发了吗？** 🚀

选择一个项目，开始你的综合实战之旅！
