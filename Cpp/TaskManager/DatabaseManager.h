#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QMap>
#include "Task.h"
#include "Category.h"

/**
 * @brief DatabaseManager 类 - 数据库管理类（单例模式）
 *
 * 负责所有数据库操作，包括任务和分类的增删改查
 */
class DatabaseManager : public QObject {
    Q_OBJECT

public:
    // ========================================
    // 单例模式
    // ========================================

    // 获取单例实例
    static DatabaseManager& instance();

    // ========================================
    // 数据库初始化
    // ========================================

    // 初始化数据库（创建表、插入预设数据）
    bool initialize();

    // ========================================
    // 任务操作
    // ========================================

    // 添加任务
    bool addTask(const Task &task);

    // 更新任务
    bool updateTask(const Task &task);

    // 删除任务
    bool deleteTask(int taskId);

    // 获取所有任务
    QList<Task> getAllTasks();

    // 根据分类获取任务
    QList<Task> getTasksByCategory(int categoryId);

    // 根据ID获取任务
    Task getTaskById(int taskId);

    // 标记任务为已完成
    bool markTaskAsCompleted(int taskId, bool completed);

    // ========================================
    // 分类操作
    // ========================================

    // 添加分类
    bool addCategory(const Category &category);

    // 更新分类
    bool updateCategory(const Category &category);

    // 删除分类
    bool deleteCategory(int categoryId);

    // 获取所有分类
    QList<Category> getAllCategories();

    // 根据ID获取分类
    Category getCategoryById(int categoryId);

    // ========================================
    // 统计功能
    // ========================================

    // 获取总任务数
    int getTotalTaskCount();

    // 获取已完成任务数
    int getCompletedTaskCount();

    // 获取未完成任务数
    int getIncompleteTaskCount();

    // 获取每个分类的任务数
    QMap<int, int> getTaskCountByCategory();

    // 获取每个优先级的任务数
    QMap<int, int> getTaskCountByPriority();

    // 获取逾期任务数
    int getOverdueTaskCount();

signals:
    // 数据库错误信号
    void databaseError(const QString &error);

private:
    // ========================================
    // 单例模式（私有构造函数）
    // ========================================

    DatabaseManager();
    ~DatabaseManager();

    // 禁止拷贝和赋值
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // ========================================
    // 私有辅助方法
    // ========================================

    // 创建数据库表
    bool createTables();

    // 插入预设分类数据
    bool insertDefaultCategories();

    // 执行 SQL 查询（带错误处理）
    bool executeQuery(const QString &queryStr);

    // ========================================
    // 成员变量
    // ========================================

    QSqlDatabase m_database;  // 数据库连接对象
};

#endif // DATABASEMANAGER_H
