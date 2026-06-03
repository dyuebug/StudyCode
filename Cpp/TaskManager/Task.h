#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QDate>
#include <QDateTime>

/**
 * @brief Task 类 - 任务数据类
 *
 * 表示一个任务的所有信息
 */
class Task {
public:
    // 默认构造函数
    Task();

    // 完整构造函数
    Task(int id, const QString &title, const QString &description,
         int categoryId, int priority, const QDate &dueDate,
         bool completed, const QDateTime &createdAt, const QDateTime &updatedAt);

    // ========================================
    // Getters（获取器）
    // ========================================

    int id() const { return m_id; }
    QString title() const { return m_title; }
    QString description() const { return m_description; }
    int categoryId() const { return m_categoryId; }
    int priority() const { return m_priority; }
    QDate dueDate() const { return m_dueDate; }
    bool completed() const { return m_completed; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }

    // ========================================
    // Setters（设置器）
    // ========================================

    void setId(int id) { m_id = id; }
    void setTitle(const QString &title) { m_title = title; }
    void setDescription(const QString &description) { m_description = description; }
    void setCategoryId(int categoryId) { m_categoryId = categoryId; }
    void setPriority(int priority) { m_priority = priority; }
    void setDueDate(const QDate &dueDate) { m_dueDate = dueDate; }
    void setCompleted(bool completed) { m_completed = completed; }
    void setUpdatedAt(const QDateTime &updatedAt) { m_updatedAt = updatedAt; }

    // ========================================
    // 辅助方法
    // ========================================

    // 获取优先级文本（"高"、"中"、"低"）
    QString priorityText() const;

    // 获取优先级颜色（用于 UI 显示）
    QString priorityColor() const;

    // 判断是否逾期
    bool isOverdue() const;

    // 判断是否即将到期（3天内）
    bool isDueSoon() const;

private:
    int m_id;                   // 任务ID
    QString m_title;            // 任务标题
    QString m_description;      // 任务描述
    int m_categoryId;           // 分类ID
    int m_priority;             // 优先级（1-高，2-中，3-低）
    QDate m_dueDate;            // 截止日期
    bool m_completed;           // 是否完成
    QDateTime m_createdAt;      // 创建时间
    QDateTime m_updatedAt;      // 更新时间
};

#endif // TASK_H
