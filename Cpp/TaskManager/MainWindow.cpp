#include "MainWindow.h"
#include "DatabaseManager.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>

// ========================================
// 构造函数和析构函数
// ========================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentCategoryFilter(0)
    , m_currentPriorityFilter(0)
    , m_currentStatusFilter(0)
{
    // 初始化数据库
    if (!DatabaseManager::instance().initialize()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败！");
        return;
    }

    // 设置 UI
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // 加载任务列表
    loadTasks();

    // 更新统计信息
    updateStatistics();
}

MainWindow::~MainWindow()
{
}

// ========================================
// UI 初始化方法
// ========================================

void MainWindow::setupUI()
{
    // 设置窗口标题和大小
    setWindowTitle("个人任务管理器");
    resize(1000, 600);

    // 创建中心控件
    setupCentralWidget();
}

void MainWindow::setupCentralWidget()
{
    // 创建中心控件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // ========================================
    // 筛选区域
    // ========================================

    QGroupBox *filterGroup = new QGroupBox("筛选条件", this);
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);

    // 分类筛选
    filterLayout->addWidget(new QLabel("分类：", this));
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItem("全部", 0);

    // 加载分类列表
    QList<Category> categories = DatabaseManager::instance().getAllCategories();
    for (const Category &category : categories) {
        m_categoryFilter->addItem(category.name(), category.id());
    }

    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCategoryFilterChanged);
    filterLayout->addWidget(m_categoryFilter);

    filterLayout->addSpacing(20);

    // 优先级筛选
    filterLayout->addWidget(new QLabel("优先级：", this));
    m_priorityFilter = new QComboBox(this);
    m_priorityFilter->addItem("全部", 0);
    m_priorityFilter->addItem("高", 1);
    m_priorityFilter->addItem("中", 2);
    m_priorityFilter->addItem("低", 3);
    connect(m_priorityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPriorityFilterChanged);
    filterLayout->addWidget(m_priorityFilter);

    filterLayout->addSpacing(20);

    // 状态筛选
    filterLayout->addWidget(new QLabel("状态：", this));
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem("全部", 0);
    m_statusFilter->addItem("未完成", 1);
    m_statusFilter->addItem("已完成", 2);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onStatusFilterChanged);
    filterLayout->addWidget(m_statusFilter);

    filterLayout->addStretch();

    mainLayout->addWidget(filterGroup);

    // ========================================
    // 任务列表
    // ========================================

    m_taskTable = new QTableWidget(this);
    m_taskTable->setColumnCount(7);
    m_taskTable->setHorizontalHeaderLabels({"ID", "标题", "分类", "优先级", "截止日期", "状态", "创建时间"});

    // 设置表格属性
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_taskTable->horizontalHeader()->setStretchLastSection(true);
    m_taskTable->setAlternatingRowColors(true);
    m_taskTable->setSortingEnabled(true);  // 启用排序

    // 隐藏 ID 列
    m_taskTable->setColumnHidden(0, true);

    // 设置列宽
    m_taskTable->setColumnWidth(1, 300);  // 标题
    m_taskTable->setColumnWidth(2, 100);  // 分类
    m_taskTable->setColumnWidth(3, 80);   // 优先级
    m_taskTable->setColumnWidth(4, 120);  // 截止日期
    m_taskTable->setColumnWidth(5, 80);   // 状态
    m_taskTable->setColumnWidth(6, 150);  // 创建时间

    connect(m_taskTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onTaskSelectionChanged);

    mainLayout->addWidget(m_taskTable);

    // ========================================
    // 按钮区域
    // ========================================

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton("添加任务", this);
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::onAddTask);
    buttonLayout->addWidget(m_addButton);

    m_editButton = new QPushButton("编辑任务", this);
    m_editButton->setEnabled(false);
    connect(m_editButton, &QPushButton::clicked, this, &MainWindow::onEditTask);
    buttonLayout->addWidget(m_editButton);

    m_deleteButton = new QPushButton("删除任务", this);
    m_deleteButton->setEnabled(false);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::onDeleteTask);
    buttonLayout->addWidget(m_deleteButton);

    m_completeButton = new QPushButton("标记完成", this);
    m_completeButton->setEnabled(false);
    connect(m_completeButton, &QPushButton::clicked, this, &MainWindow::onCompleteTask);
    buttonLayout->addWidget(m_completeButton);

    buttonLayout->addStretch();

    m_refreshButton = new QPushButton("刷新", this);
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefresh);
    buttonLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(buttonLayout);
}

void MainWindow::setupMenuBar()
{
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");

    QAction *refreshAction = fileMenu->addAction("刷新(&R)");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefresh);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // 任务菜单
    QMenu *taskMenu = menuBar->addMenu("任务(&T)");

    QAction *addAction = taskMenu->addAction("添加任务(&A)");
    addAction->setShortcut(QKeySequence::New);
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddTask);

    QAction *editAction = taskMenu->addAction("编辑任务(&E)");
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditTask);

    QAction *deleteAction = taskMenu->addAction("删除任务(&D)");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteTask);

    taskMenu->addSeparator();

    QAction *completeAction = taskMenu->addAction("标记完成(&C)");
    connect(completeAction, &QAction::triggered, this, &MainWindow::onCompleteTask);

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");

    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::setupToolBar()
{
    // 创建工具栏
    QToolBar *toolBar = new QToolBar(this);
    addToolBar(toolBar);

    // 添加工具栏按钮
    QAction *addAction = toolBar->addAction("添加");
    connect(addAction, &QAction::triggered, this, &MainWindow::onAddTask);

    QAction *editAction = toolBar->addAction("编辑");
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditTask);

    QAction *deleteAction = toolBar->addAction("删除");
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteTask);

    toolBar->addSeparator();

    QAction *completeAction = toolBar->addAction("完成");
    connect(completeAction, &QAction::triggered, this, &MainWindow::onCompleteTask);

    toolBar->addSeparator();

    QAction *refreshAction = toolBar->addAction("刷新");
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefresh);
}

void MainWindow::setupStatusBar()
{
    // 创建状态栏
    m_statsLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statsLabel);
}

// ========================================
// 数据操作方法
// ========================================

void MainWindow::loadTasks()
{
    // 清空表格
    m_taskTable->setRowCount(0);

    // 获取所有任务
    QList<Task> tasks = DatabaseManager::instance().getAllTasks();

    // 应用筛选
    QList<Task> filteredTasks;
    for (const Task &task : tasks) {
        // 分类筛选
        if (m_currentCategoryFilter != 0 && task.categoryId() != m_currentCategoryFilter) {
            continue;
        }

        // 优先级筛选
        if (m_currentPriorityFilter != 0 && task.priority() != m_currentPriorityFilter) {
            continue;
        }

        // 状态筛选
        if (m_currentStatusFilter == 1 && task.completed()) {
            continue;  // 只显示未完成
        }
        if (m_currentStatusFilter == 2 && !task.completed()) {
            continue;  // 只显示已完成
        }

        filteredTasks.append(task);
    }

    // 填充表格
    for (const Task &task : filteredTasks) {
        int row = m_taskTable->rowCount();
        m_taskTable->insertRow(row);

        // ID（隐藏列）
        m_taskTable->setItem(row, 0, new QTableWidgetItem(QString::number(task.id())));

        // 标题
        m_taskTable->setItem(row, 1, new QTableWidgetItem(task.title()));

        // 分类
        Category category = DatabaseManager::instance().getCategoryById(task.categoryId());
        m_taskTable->setItem(row, 2, new QTableWidgetItem(category.name()));

        // 优先级
        QTableWidgetItem *priorityItem = new QTableWidgetItem(task.priorityText());
        priorityItem->setForeground(QColor(task.priorityColor()));
        m_taskTable->setItem(row, 3, priorityItem);

        // 截止日期
        QString dueDateStr = task.dueDate().isValid() ? task.dueDate().toString("yyyy-MM-dd") : "无";
        QTableWidgetItem *dueDateItem = new QTableWidgetItem(dueDateStr);

        // 逾期标红
        if (task.isOverdue()) {
            dueDateItem->setForeground(QColor("#e74c3c"));
        }
        // 即将到期标橙
        else if (task.isDueSoon()) {
            dueDateItem->setForeground(QColor("#f39c12"));
        }

        m_taskTable->setItem(row, 4, dueDateItem);

        // 状态
        QString statusStr = task.completed() ? "已完成" : "未完成";
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusStr);
        if (task.completed()) {
            statusItem->setForeground(QColor("#2ecc71"));
        }
        m_taskTable->setItem(row, 5, statusItem);

        // 创建时间
        m_taskTable->setItem(row, 6, new QTableWidgetItem(task.createdAt().toString("yyyy-MM-dd hh:mm")));
    }
}

void MainWindow::updateStatistics()
{
    int total = DatabaseManager::instance().getTotalTaskCount();
    int completed = DatabaseManager::instance().getCompletedTaskCount();
    int incomplete = DatabaseManager::instance().getIncompleteTaskCount();
    int overdue = DatabaseManager::instance().getOverdueTaskCount();

    double completionRate = total > 0 ? (completed * 100.0 / total) : 0.0;

    QString statsText = QString("总任务：%1 | 已完成：%2 | 未完成：%3 | 逾期：%4 | 完成率：%5%")
                            .arg(total)
                            .arg(completed)
                            .arg(incomplete)
                            .arg(overdue)
                            .arg(completionRate, 0, 'f', 1);

    m_statsLabel->setText(statsText);
}

void MainWindow::applyFilters()
{
    loadTasks();
    updateStatistics();
}

// ========================================
// 槽函数实现
// ========================================

void MainWindow::onAddTask()
{
    // 创建 TaskDialog（添加模式）
    TaskDialog dialog(this);

    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 获取任务数据
        Task task = dialog.getTask();

        // 添加到数据库
        if (DatabaseManager::instance().addTask(task)) {
            QMessageBox::information(this, "成功", "任务添加成功");
            loadTasks();
            updateStatistics();
        } else {
            QMessageBox::critical(this, "错误", "任务添加失败");
        }
    }
}

void MainWindow::onEditTask()
{
    // 获取选中的任务
    int currentRow = m_taskTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请先选择一个任务");
        return;
    }

    // 获取任务 ID
    int taskId = m_taskTable->item(currentRow, 0)->text().toInt();
    Task task = DatabaseManager::instance().getTaskById(taskId);

    // 创建 TaskDialog（编辑模式）
    TaskDialog dialog(task, this);

    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        // 获取更新后的任务数据
        Task updatedTask = dialog.getTask();
        updatedTask.setId(taskId);

        // 更新数据库
        if (DatabaseManager::instance().updateTask(updatedTask)) {
            QMessageBox::information(this, "成功", "任务更新成功");
            loadTasks();
            updateStatistics();
        } else {
            QMessageBox::critical(this, "错误", "任务更新失败");
        }
    }
}

void MainWindow::onDeleteTask()
{
    // 获取选中的任务
    int currentRow = m_taskTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请先选择一个任务");
        return;
    }

    // 获取任务 ID
    int taskId = m_taskTable->item(currentRow, 0)->text().toInt();
    QString taskTitle = m_taskTable->item(currentRow, 1)->text();

    // 确认删除
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认删除",
                                  QString("确定要删除任务 \"%1\" 吗？").arg(taskTitle),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 删除任务
        if (DatabaseManager::instance().deleteTask(taskId)) {
            QMessageBox::information(this, "成功", "任务删除成功");
            loadTasks();
            updateStatistics();
        } else {
            QMessageBox::critical(this, "错误", "任务删除失败");
        }
    }
}

void MainWindow::onCompleteTask()
{
    // 获取选中的任务
    int currentRow = m_taskTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请先选择一个任务");
        return;
    }

    // 获取任务 ID 和当前状态
    int taskId = m_taskTable->item(currentRow, 0)->text().toInt();
    QString statusStr = m_taskTable->item(currentRow, 5)->text();
    bool currentCompleted = (statusStr == "已完成");

    // 切换完成状态
    bool newCompleted = !currentCompleted;

    if (DatabaseManager::instance().markTaskAsCompleted(taskId, newCompleted)) {
        QString message = newCompleted ? "任务已标记为完成" : "任务已标记为未完成";
        QMessageBox::information(this, "成功", message);
        loadTasks();
        updateStatistics();
    } else {
        QMessageBox::critical(this, "错误", "标记任务失败");
    }
}

void MainWindow::onRefresh()
{
    loadTasks();
    updateStatistics();
    QMessageBox::information(this, "提示", "刷新成功");
}

void MainWindow::onCategoryFilterChanged(int index)
{
    m_currentCategoryFilter = m_categoryFilter->itemData(index).toInt();
    applyFilters();
}

void MainWindow::onPriorityFilterChanged(int index)
{
    m_currentPriorityFilter = m_priorityFilter->itemData(index).toInt();
    applyFilters();
}

void MainWindow::onStatusFilterChanged(int index)
{
    m_currentStatusFilter = m_statusFilter->itemData(index).toInt();
    applyFilters();
}

void MainWindow::onTaskSelectionChanged()
{
    // 根据是否选中任务，启用/禁用按钮
    bool hasSelection = m_taskTable->currentRow() >= 0;

    m_editButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);
    m_completeButton->setEnabled(hasSelection);

    // 更新完成按钮文本
    if (hasSelection) {
        int currentRow = m_taskTable->currentRow();
        QString statusStr = m_taskTable->item(currentRow, 5)->text();
        bool isCompleted = (statusStr == "已完成");

        m_completeButton->setText(isCompleted ? "标记未完成" : "标记完成");
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "关于",
                       "个人任务管理器 v1.0\n\n"
                       "一个基于 Qt 的任务管理应用\n\n"
                       "功能特性：\n"
                       "- 任务增删改查\n"
                       "- 任务分类和优先级\n"
                       "- 截止日期管理\n"
                       "- 任务统计\n\n"
                       "开发者：C++ 学习者");
}

