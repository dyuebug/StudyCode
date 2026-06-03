# Qt 学习总结 - 阶段7至阶段10回顾 🎓

> **学习时间**：第38天 - 第51天（共14天）  
> **学习内容**：Qt 图形界面编程、数据库编程、网络编程、多线程编程

---

## 📋 目录

1. [学习路线图](#学习路线图)
2. [阶段7：Qt 图形界面编程](#阶段7qt-图形界面编程)
3. [阶段8：Qt 数据库编程](#阶段8qt-数据库编程)
4. [阶段9：Qt 网络编程](#阶段9qt-网络编程)
5. [阶段10：Qt 多线程编程](#阶段10qt-多线程编程)
6. [Qt 技能树](#qt-技能树)
7. [完成的项目](#完成的项目)
8. [学习心得](#学习心得)

---

## 学习路线图

```
阶段7: Qt 图形界面编程 (第38-44天，7天)
  ├─ Qt 基础（QObject、信号与槽、控件、布局）
  ├─ Qt 样式表（深色/浅色主题）
  ├─ Qt 对话框（QMessageBox、QInputDialog、QFileDialog）
  ├─ QMainWindow（菜单栏、工具栏、状态栏）
  └─ 实战项目：简单计算器、记事本应用

阶段8: Qt 数据库编程 (第45-46天，2天)
  ├─ QSqlDatabase（数据库连接）
  ├─ QSqlQuery（SQL 查询、CRUD）
  ├─ QSqlTableModel + QTableView（Model-View 架构）
  ├─ 事务、JOIN 查询、GROUP BY 聚合
  └─ 实战项目：联系人管理、带分组联系人管理

阶段9: Qt 网络编程 (第47-49天，3天)
  ├─ QNetworkAccessManager（HTTP 请求）
  ├─ QJsonDocument（JSON 解析）
  ├─ 公开 API 调用（IP查询、UUID生成、汇率查询）
  ├─ 文件下载器（进度条、速度计算、取消下载）
  └─ 实战项目：天气查询应用

阶段10: Qt 多线程编程 (第49-51天，3天)
  ├─ QThread 基础（moveToThread 模式）
  ├─ 线程同步（QMutex、QReadWriteLock、死锁预防）
  ├─ QtConcurrent 高级并发（run、mapped、filtered）
  ├─ QFuture + QFutureWatcher（异步任务管理）
  └─ 实战项目：多线程文件搜索工具
```

---

## 阶段7：Qt 图形界面编程

### 核心技术

#### 1. Qt 核心概念
- **QObject 基类**：所有 Qt 对象的基类
- **对象树**：自动内存管理（父对象销毁时自动销毁子对象）
- **信号与槽机制**：对象间通信的核心机制
- **MOC（元对象编译器）**：实现信号槽的编译器

#### 2. 基础控件
- **QLabel**：显示文本或图片
- **QLineEdit**：单行文本输入
- **QPushButton**：按钮
- **QComboBox**：下拉框
- **QCheckBox**：复选框
- **QRadioButton**：单选按钮
- **QSpinBox**：数字输入框

#### 3. 布局管理
- **QVBoxLayout**：垂直布局
- **QHBoxLayout**：水平布局
- **QGridLayout**：网格布局
- **QFormLayout**：表单布局
- **布局嵌套**：组合使用多种布局

#### 4. Qt 样式表
- **选择器**：类型选择器、ID选择器、类选择器
- **伪状态**：`:hover`、`:pressed`、`:checked`
- **子控件选择器**：`::drop-down`、`::indicator`
- **主题设计**：深色主题、浅色主题

#### 5. 对话框
- **QMessageBox**：消息框（information、warning、critical、question）
- **QInputDialog**：输入对话框（getText、getInt、getDouble、getItem）
- **QFileDialog**：文件对话框（打开、保存、选择目录）
- **QColorDialog**：颜色选择对话框
- **自定义 QDialog**：继承 QDialog 创建自定义对话框

#### 6. QMainWindow
- **QMenuBar**：菜单栏
- **QToolBar**：工具栏
- **QStatusBar**：状态栏
- **QAction**：动作对象（菜单项、工具栏按钮）
- **closeEvent**：关闭事件处理

### 完成的项目
1. **简单计算器**：四则运算、连续运算、深色主题
2. **记事本应用**：新建/打开/保存文件、菜单栏、工具栏

### 学习文件
- `01_qt_basics.cpp` - Qt 基础
- `02_more_controls.cpp` - 更多控件
- `03_qt_styles.cpp` - Qt 样式表
- `04_dialogs.cpp` - 对话框
- `05_calculator.cpp` - 简单计算器
- `06_notepad.cpp` - 记事本应用
- `07_stage7_summary.md` - 阶段7总结

---

## 阶段8：Qt 数据库编程

### 核心技术

#### 1. 数据库连接
- **QSqlDatabase**：数据库连接管理
- **addDatabase()**：添加数据库连接
- **setDatabaseName()**：设置数据库文件
- **open()**：打开数据库连接

#### 2. SQL 查询
- **QSqlQuery**：执行 SQL 语句
- **exec()**：执行查询
- **next()**：移动到下一条记录
- **value()**：获取字段值
- **参数化查询**：防止 SQL 注入（`:name`、`?`）

#### 3. CRUD 操作
- **CREATE**：创建表（`CREATE TABLE`）
- **INSERT**：插入数据（`INSERT INTO`）
- **SELECT**：查询数据（`SELECT * FROM`）
- **UPDATE**：更新数据（`UPDATE SET`）
- **DELETE**：删除数据（`DELETE FROM`）

#### 4. Model-View 架构
- **QSqlTableModel**：表格模型
- **QTableView**：表格视图
- **setTable()**：设置表名
- **select()**：查询数据
- **submitAll()**：提交修改

#### 5. 高级查询
- **事务**：`transaction()`、`commit()`、`rollback()`
- **JOIN 查询**：`INNER JOIN`、`LEFT JOIN`
- **GROUP BY 聚合**：`COUNT()`、`SUM()`、`AVG()`
- **QSqlRelationalTableModel**：关联表模型（外键关系）

### 完成的项目
1. **联系人管理**：增删改查、QSqlTableModel + QTableView
2. **带分组联系人管理**：分组管理、JOIN 查询、外键关系

### 学习文件
- `08_sql_basics.cpp` - SQL 基础
- `09_sql_advanced.cpp` - SQL 进阶
- `10_contact_manager.cpp` - 联系人管理
- `11_contact_with_groups.cpp` - 带分组联系人管理
- `12_sql_summary.md` - SQL 速查表

---

## 阶段9：Qt 网络编程

### 核心技术

#### 1. HTTP 请求
- **QNetworkAccessManager**：网络请求管理器
- **QNetworkRequest**：网络请求对象
- **QNetworkReply**：网络响应对象
- **get()**：发送 GET 请求
- **post()**：发送 POST 请求

#### 2. JSON 解析
- **QJsonDocument**：JSON 文档
- **QJsonObject**：JSON 对象
- **QJsonArray**：JSON 数组
- **fromJson()**：解析 JSON 字符串
- **object()**、`array()`：获取 JSON 对象/数组

#### 3. 公开 API 调用
- **IP 查询**：`ipinfo.io`
- **UUID 生成**：`httpbin.org/uuid`
- **汇率查询**：`api.exchangerate-api.com`
- **天气查询**：`wttr.in`

#### 4. 文件下载
- **downloadProgress**：下载进度信号
- **readyRead**：数据就绪信号
- **流式写入**：边下载边写入文件
- **进度条**：显示下载进度和速度
- **取消下载**：`abort()`

#### 5. 嵌套 JSON 解析
- **多层嵌套**：`object["key1"]["key2"]`
- **数组遍历**：`for (const QJsonValue &val : array)`
- **类型检查**：`isObject()`、`isArray()`、`isString()`

### 完成的项目
1. **天气查询应用**：城市天气查询、嵌套 JSON 解析、卡片 UI

### 学习文件
- `13_network_basics.cpp` - 网络基础
- `14_json_parsing.cpp` - JSON 解析
- `15_public_api.cpp` - 公开 API 调用
- `16_file_downloader.cpp` - 文件下载器
- `17_weather_app.cpp` - 天气查询应用
- `31_network_summary.md` - 网络编程速查表

---

## 阶段10：Qt 多线程编程

### 核心技术

#### 1. QThread 基础
- **moveToThread 模式**：推荐的多线程模式
- **线程间信号槽通信**：自动队列连接（线程安全）
- **线程生命周期管理**：`start()`、`quit()`、`wait()`
- **取消机制**：布尔标志 + 定期检查

#### 2. 线程同步
- **数据竞争问题**：多线程同时访问共享数据
- **QMutex 互斥锁**：`lock()`、`unlock()`、`tryLock()`
- **QMutexLocker 自动锁管理**：RAII 方式（推荐）
- **QReadWriteLock 读写锁**：`lockForRead()`、`lockForWrite()`
- **死锁预防**：固定加锁顺序、`tryLock()`、减少锁持有时间

#### 3. QtConcurrent 高级并发
- **QtConcurrent::run()**：在线程池中运行函数
- **QtConcurrent::mapped()**：并行映射
- **QtConcurrent::filtered()**：并行过滤
- **QFuture**：异步任务结果（`result()`、`isFinished()`、`cancel()`）
- **QFutureWatcher**：监听任务完成（`finished()`、`progressValueChanged()`）

#### 4. 线程池
- **QThreadPool**：全局线程池
- **maxThreadCount()**：最大线程数（默认 CPU 核心数）
- **activeThreadCount()**：活跃线程数
- **自动管理**：QtConcurrent 自动使用线程池

#### 5. QThread vs QtConcurrent
- **QThread**：手动管理、适合长期后台任务
- **QtConcurrent**：自动管理、适合短期计算任务

### 完成的项目
1. **多线程文件搜索工具**：多目录并发搜索、实时结果、取消机制

### 学习文件
- `32_thread_basics.cpp` - QThread 基础
- `33_thread_practice.cpp` - 多线程实战（斐波那契计算器）
- `34_thread_sync.cpp` - 线程同步基础
- `35_qtconcurrent.cpp` - QtConcurrent 高级并发
- `36_file_search.cpp` - 多线程文件搜索工具
- `37_thread_summary.md` - 多线程速查表

---

## Qt 技能树

### 已掌握的核心技术

```
Qt 核心
├─ QObject（对象树、信号与槽、MOC）
├─ 事件处理（closeEvent、keyPressEvent）
└─ 内存管理（自动内存管理、deleteLater）

Qt 界面
├─ 基础控件（QLabel、QLineEdit、QPushButton、QComboBox、QCheckBox、QRadioButton、QSpinBox）
├─ 布局管理（QVBoxLayout、QHBoxLayout、QGridLayout、QFormLayout）
├─ Qt 样式表（选择器、伪状态、主题设计）
├─ 对话框（QMessageBox、QInputDialog、QFileDialog、QColorDialog、自定义 QDialog）
└─ QMainWindow（QMenuBar、QToolBar、QStatusBar、QAction）

Qt 数据库
├─ QSqlDatabase（数据库连接）
├─ QSqlQuery（SQL 查询、CRUD、参数化查询）
├─ QSqlTableModel + QTableView（Model-View 架构）
├─ 事务（transaction、commit、rollback）
├─ JOIN 查询（INNER JOIN、LEFT JOIN）
├─ GROUP BY 聚合（COUNT、SUM、AVG）
└─ QSqlRelationalTableModel（外键关系）

Qt 网络
├─ QNetworkAccessManager（HTTP 请求管理器）
├─ QNetworkRequest（网络请求对象）
├─ QNetworkReply（网络响应对象）
├─ HTTP GET/POST 请求
├─ QJsonDocument（JSON 解析）
├─ 公开 API 调用
├─ 文件下载（downloadProgress、流式写入、取消下载）
└─ 嵌套 JSON 解析

Qt 多线程
├─ QThread（moveToThread 模式、线程生命周期管理）
├─ 线程间信号槽通信（自动队列连接）
├─ 线程同步（QMutex、QMutexLocker、QReadWriteLock）
├─ 死锁预防（固定加锁顺序、tryLock）
├─ QtConcurrent（run、mapped、filtered）
├─ QFuture + QFutureWatcher（异步任务管理）
└─ QThreadPool（线程池管理）
```

### 技能统计

- **学习天数**：14天（第38天 - 第51天）
- **学习文件**：37个
- **完成项目**：6个
- **掌握技能**：约150个

---

## 完成的项目

### 1. 简单计算器（阶段7）
- **功能**：四则运算、连续运算、除零保护、深色主题
- **技术**：QGridLayout、QPushButton、QLineEdit、状态机、Qt 样式表
- **代码量**：约300行

### 2. 记事本应用（阶段7）
- **功能**：新建/打开/保存文件、菜单栏、工具栏、状态栏、关闭确认
- **技术**：QMainWindow、QAction、QMenuBar、QToolBar、QStatusBar、QFile、closeEvent
- **代码量**：约400行

### 3. 联系人管理（阶段8）
- **功能**：联系人增删改查、QSqlTableModel、QTableView、参数化查询
- **技术**：QSqlDatabase、QSqlQuery、QSqlTableModel、QTableView、Model-View 架构
- **代码量**：约350行

### 4. 带分组联系人管理（阶段8）
- **功能**：分组管理、外键关系、JOIN 查询、GROUP BY 聚合、QSqlRelationalTableModel
- **技术**：事务、INNER/LEFT JOIN、QSqlRelationalTableModel、外键约束
- **代码量**：约450行

### 5. 天气查询应用（阶段9）
- **功能**：城市天气查询、嵌套 JSON 解析、卡片 UI、QScrollArea
- **技术**：QNetworkAccessManager、wttr.in API、QJsonDocument、QUrl::toPercentEncoding
- **代码量**：约400行

### 6. 多线程文件搜索工具（阶段10）
- **功能**：多目录并发搜索、文件名模式匹配、实时结果显示、进度更新、取消搜索
- **技术**：QtConcurrent::run()、QFutureWatcher、QDirIterator、线程池
- **代码量**：约450行

---

## 学习心得

### 收获

1. **Qt 框架的强大**
   - 信号与槽机制简化了对象间通信
   - 自动内存管理减少了内存泄漏
   - 丰富的控件和布局管理器加速开发
   - 跨平台支持（Windows、Linux、macOS）

2. **Model-View 架构的优雅**
   - 数据和界面分离
   - QSqlTableModel + QTableView 自动同步
   - 减少了大量手动代码

3. **QtConcurrent 的便利**
   - 比 QThread 简单得多
   - 自动线程池管理
   - 一行代码启动异步任务

4. **实战项目的重要性**
   - 理论知识需要通过项目巩固
   - 每个项目都整合了多个技术
   - 项目经验是最宝贵的财富

### 挑战

1. **信号与槽的理解**
   - 初期不太理解信号槽的工作原理
   - 通过多次实践逐渐掌握

2. **多线程的复杂性**
   - 数据竞争、死锁等问题难以调试
   - 需要仔细设计线程间通信

3. **JSON 解析的嵌套**
   - 嵌套 JSON 解析容易出错
   - 需要仔细检查每一层的类型

### 下一步计划

1. **Qt 综合项目**
   - 整合所有学过的技术
   - 开发一个完整的桌面应用
   - 可能的项目：任务管理器、笔记应用、聊天工具

2. **Qt 进阶技术**
   - Qt Quick（QML）
   - Qt 3D
   - Qt Charts（图表）
   - Qt WebEngine（嵌入浏览器）

3. **项目打包和发布**
   - 学习如何打包 Qt 应用
   - 制作安装程序
   - 发布到 GitHub

---

## 🎯 总结

经过14天的学习，我已经掌握了 Qt 的核心技术：
- ✅ 图形界面编程（控件、布局、样式、对话框）
- ✅ 数据库编程（SQL、Model-View）
- ✅ 网络编程（HTTP、JSON、文件下载）
- ✅ 多线程编程（QThread、QtConcurrent、线程同步）

完成了6个实战项目，积累了丰富的项目经验。

**接下来，准备开发一个 Qt 综合项目，整合所有学过的技术！** 🚀
