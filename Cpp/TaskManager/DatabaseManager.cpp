#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDir>

// ========================================
// 单例模式实现
// ========================================

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

// ========================================
// 构造函数和析构函数
// ========================================

DatabaseManager::DatabaseManager()
{
    // 构造函数中不做任何操作
    // 数据库初始化在 initialize() 中完成
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

// ========================================
// 数据库初始化
// ========================================

bool DatabaseManager::initialize()
{
    // 添加 SQLite 数据库
    m_database = QSqlDatabase::addDatabase("QSQLITE");

    // 设置数据库文件路径（当前目录下的 tasks.db）
    m_database.setDatabaseName("tasks.db");

    // 打开数据库
    if (!m_database.open()) {
        qDebug() << "数据库打开失败：" << m_database.lastError().text();
        emit databaseError("数据库打开失败：" + m_database.lastError().text());
        return false;
    }

    qDebug() << "数据库打开成功";

    // 创建表
    if (!createTables()) {
        qDebug() << "创建表失败";
        return false;
    }

    // 插入预设分类数据
    if (!insertDefaultCategories()) {
        qDebug() << "插入预设分类失败";
        return false;
    }

    qDebug() << "数据库初始化成功";
    return true;
}

// ========================================
// 私有辅助方法
// ========================================

bool DatabaseManager::createTables()
{
    QSqlQuery query;

    // 创建 categories 表
    QString createCategoriesTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            color TEXT DEFAULT '#3498db'
        )
    )";

    if (!query.exec(createCategoriesTable)) {
        qDebug() << "创建 categories 表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "categories 表创建成功";

    // 创建 tasks 表
    QString createTasksTable = R"(
        CREATE TABLE IF NOT EXISTS tasks (
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
        )
    )";

    if (!query.exec(createTasksTable)) {
        qDebug() << "创建 tasks 表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "tasks 表创建成功";

    return true;
}

bool DatabaseManager::insertDefaultCategories()
{
    QSqlQuery query;

    // 检查是否已有分类数据
    query.exec("SELECT COUNT(*) FROM categories");
    if (query.next() && query.value(0).toInt() > 0) {
        qDebug() << "分类数据已存在，跳过插入";
        return true;
    }

    // 插入预设分类
    QStringList categories = {
        "('工作', '#e74c3c')",
        "('学习', '#3498db')",
        "('生活', '#2ecc71')",
        "('其他', '#95a5a6')"
    };

    for (const QString &category : categories) {
        QString insertQuery = "INSERT INTO categories (name, color) VALUES " + category;
        if (!query.exec(insertQuery)) {
            qDebug() << "插入分类失败：" << query.lastError().text();
            return false;
        }
    }

    qDebug() << "预设分类插入成功";
    return true;
}

bool DatabaseManager::executeQuery(const QString &queryStr)
{
    QSqlQuery query;
    if (!query.exec(queryStr)) {
        qDebug() << "SQL 执行失败：" << query.lastError().text();
        qDebug() << "SQL：" << queryStr;
        emit databaseError("SQL 执行失败：" + query.lastError().text());
        return false;
    }
    return true;
}

// ========================================
// 任务操作
// ========================================

bool DatabaseManager::addTask(const Task &task)
{
    QSqlQuery query;

    // 准备插入语句（使用参数化查询防止 SQL 注入）
    query.prepare("INSERT INTO tasks (title, description, category_id, priority, due_date, completed) "
                  "VALUES (:title, :description, :category_id, :priority, :due_date, :completed)");

    query.bindValue(":title", task.title());
    query.bindValue(":description", task.description());
    query.bindValue(":category_id", task.categoryId());
    query.bindValue(":priority", task.priority());
    query.bindValue(":due_date", task.dueDate().toString("yyyy-MM-dd"));
    query.bindValue(":completed", task.completed() ? 1 : 0);

    if (!query.exec()) {
        qDebug() << "添加任务失败：" << query.lastError().text();
        emit databaseError("添加任务失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "任务添加成功，ID：" << query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateTask(const Task &task)
{
    QSqlQuery query;

    // 准备更新语句
    query.prepare("UPDATE tasks SET title = :title, description = :description, "
                  "category_id = :category_id, priority = :priority, due_date = :due_date, "
                  "completed = :completed, updated_at = CURRENT_TIMESTAMP "
                  "WHERE id = :id");

    query.bindValue(":title", task.title());
    query.bindValue(":description", task.description());
    query.bindValue(":category_id", task.categoryId());
    query.bindValue(":priority", task.priority());
    query.bindValue(":due_date", task.dueDate().toString("yyyy-MM-dd"));
    query.bindValue(":completed", task.completed() ? 1 : 0);
    query.bindValue(":id", task.id());

    if (!query.exec()) {
        qDebug() << "更新任务失败：" << query.lastError().text();
        emit databaseError("更新任务失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "任务更新成功，ID：" << task.id();
    return true;
}

bool DatabaseManager::deleteTask(int taskId)
{
    QSqlQuery query;

    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        qDebug() << "删除任务失败：" << query.lastError().text();
        emit databaseError("删除任务失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "任务删除成功，ID：" << taskId;
    return true;
}

QList<Task> DatabaseManager::getAllTasks()
{
    QList<Task> tasks;
    QSqlQuery query("SELECT id, title, description, category_id, priority, due_date, "
                    "completed, created_at, updated_at FROM tasks ORDER BY created_at DESC");

    while (query.next()) {
        Task task;
        task.setId(query.value(0).toInt());
        task.setTitle(query.value(1).toString());
        task.setDescription(query.value(2).toString());
        task.setCategoryId(query.value(3).toInt());
        task.setPriority(query.value(4).toInt());
        task.setDueDate(QDate::fromString(query.value(5).toString(), "yyyy-MM-dd"));
        task.setCompleted(query.value(6).toInt() == 1);
        task.setCreatedAt(QDateTime::fromString(query.value(7).toString(), Qt::ISODate));
        task.setUpdatedAt(QDateTime::fromString(query.value(8).toString(), Qt::ISODate));

        tasks.append(task);
    }

    return tasks;
}

QList<Task> DatabaseManager::getTasksByCategory(int categoryId)
{
    QList<Task> tasks;
    QSqlQuery query;

    query.prepare("SELECT id, title, description, category_id, priority, due_date, "
                  "completed, created_at, updated_at FROM tasks WHERE category_id = :category_id "
                  "ORDER BY created_at DESC");
    query.bindValue(":category_id", categoryId);

    if (!query.exec()) {
        qDebug() << "查询任务失败：" << query.lastError().text();
        return tasks;
    }

    while (query.next()) {
        Task task;
        task.setId(query.value(0).toInt());
        task.setTitle(query.value(1).toString());
        task.setDescription(query.value(2).toString());
        task.setCategoryId(query.value(3).toInt());
        task.setPriority(query.value(4).toInt());
        task.setDueDate(QDate::fromString(query.value(5).toString(), "yyyy-MM-dd"));
        task.setCompleted(query.value(6).toInt() == 1);
        task.setCreatedAt(QDateTime::fromString(query.value(7).toString(), Qt::ISODate));
        task.setUpdatedAt(QDateTime::fromString(query.value(8).toString(), Qt::ISODate));

        tasks.append(task);
    }

    return tasks;
}

Task DatabaseManager::getTaskById(int taskId)
{
    Task task;
    QSqlQuery query;

    query.prepare("SELECT id, title, description, category_id, priority, due_date, "
                  "completed, created_at, updated_at FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        qDebug() << "查询任务失败：" << query.lastError().text();
        return task;
    }

    if (query.next()) {
        task.setId(query.value(0).toInt());
        task.setTitle(query.value(1).toString());
        task.setDescription(query.value(2).toString());
        task.setCategoryId(query.value(3).toInt());
        task.setPriority(query.value(4).toInt());
        task.setDueDate(QDate::fromString(query.value(5).toString(), "yyyy-MM-dd"));
        task.setCompleted(query.value(6).toInt() == 1);
        task.setCreatedAt(QDateTime::fromString(query.value(7).toString(), Qt::ISODate));
        task.setUpdatedAt(QDateTime::fromString(query.value(8).toString(), Qt::ISODate));
    }

    return task;
}

bool DatabaseManager::markTaskAsCompleted(int taskId, bool completed)
{
    QSqlQuery query;

    query.prepare("UPDATE tasks SET completed = :completed, updated_at = CURRENT_TIMESTAMP "
                  "WHERE id = :id");
    query.bindValue(":completed", completed ? 1 : 0);
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        qDebug() << "标记任务失败：" << query.lastError().text();
        emit databaseError("标记任务失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "任务标记成功，ID：" << taskId << "，完成状态：" << completed;
    return true;
}

// ========================================
// 分类操作
// ========================================

bool DatabaseManager::addCategory(const Category &category)
{
    QSqlQuery query;

    query.prepare("INSERT INTO categories (name, color) VALUES (:name, :color)");
    query.bindValue(":name", category.name());
    query.bindValue(":color", category.color());

    if (!query.exec()) {
        qDebug() << "添加分类失败：" << query.lastError().text();
        emit databaseError("添加分类失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "分类添加成功，ID：" << query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateCategory(const Category &category)
{
    QSqlQuery query;

    query.prepare("UPDATE categories SET name = :name, color = :color WHERE id = :id");
    query.bindValue(":name", category.name());
    query.bindValue(":color", category.color());
    query.bindValue(":id", category.id());

    if (!query.exec()) {
        qDebug() << "更新分类失败：" << query.lastError().text();
        emit databaseError("更新分类失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "分类更新成功，ID：" << category.id();
    return true;
}

bool DatabaseManager::deleteCategory(int categoryId)
{
    QSqlQuery query;

    query.prepare("DELETE FROM categories WHERE id = :id");
    query.bindValue(":id", categoryId);

    if (!query.exec()) {
        qDebug() << "删除分类失败：" << query.lastError().text();
        emit databaseError("删除分类失败：" + query.lastError().text());
        return false;
    }

    qDebug() << "分类删除成功，ID：" << categoryId;
    return true;
}

QList<Category> DatabaseManager::getAllCategories()
{
    QList<Category> categories;
    QSqlQuery query("SELECT id, name, color FROM categories ORDER BY id");

    while (query.next()) {
        Category category;
        category.setId(query.value(0).toInt());
        category.setName(query.value(1).toString());
        category.setColor(query.value(2).toString());

        categories.append(category);
    }

    return categories;
}

Category DatabaseManager::getCategoryById(int categoryId)
{
    Category category;
    QSqlQuery query;

    query.prepare("SELECT id, name, color FROM categories WHERE id = :id");
    query.bindValue(":id", categoryId);

    if (!query.exec()) {
        qDebug() << "查询分类失败：" << query.lastError().text();
        return category;
    }

    if (query.next()) {
        category.setId(query.value(0).toInt());
        category.setName(query.value(1).toString());
        category.setColor(query.value(2).toString());
    }

    return category;
}

// ========================================
// 统计功能
// ========================================

int DatabaseManager::getTotalTaskCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tasks");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getCompletedTaskCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tasks WHERE completed = 1");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int DatabaseManager::getIncompleteTaskCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tasks WHERE completed = 0");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QMap<int, int> DatabaseManager::getTaskCountByCategory()
{
    QMap<int, int> result;
    QSqlQuery query("SELECT category_id, COUNT(*) FROM tasks GROUP BY category_id");

    while (query.next()) {
        int categoryId = query.value(0).toInt();
        int count = query.value(1).toInt();
        result[categoryId] = count;
    }

    return result;
}

QMap<int, int> DatabaseManager::getTaskCountByPriority()
{
    QMap<int, int> result;
    QSqlQuery query("SELECT priority, COUNT(*) FROM tasks GROUP BY priority");

    while (query.next()) {
        int priority = query.value(0).toInt();
        int count = query.value(1).toInt();
        result[priority] = count;
    }

    return result;
}

int DatabaseManager::getOverdueTaskCount()
{
    QSqlQuery query;
    QString today = QDate::currentDate().toString("yyyy-MM-dd");

    query.prepare("SELECT COUNT(*) FROM tasks WHERE completed = 0 AND due_date < :today");
    query.bindValue(":today", today);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

