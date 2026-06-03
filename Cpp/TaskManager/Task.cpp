#include "Task.h"

// ========================================
// 构造函数
// ========================================

// 默认构造函数
Task::Task()
    : m_id(0)
    , m_categoryId(0)
    , m_priority(2)  // 默认中优先级
    , m_completed(false)
{
}

// 完整构造函数
Task::Task(int id, const QString &title, const QString &description,
           int categoryId, int priority, const QDate &dueDate,
           bool completed, const QDateTime &createdAt, const QDateTime &updatedAt)
    : m_id(id)
    , m_title(title)
    , m_description(description)
    , m_categoryId(categoryId)
    , m_priority(priority)
    , m_dueDate(dueDate)
    , m_completed(completed)
    , m_createdAt(createdAt)
    , m_updatedAt(updatedAt)
{
}

// ========================================
// 辅助方法
// ========================================

// 获取优先级文本
QString Task::priorityText() const
{
    switch (m_priority) {
    case 1:
        return "高";
    case 2:
        return "中";
    case 3:
        return "低";
    default:
        return "未知";
    }
}

// 获取优先级颜色
QString Task::priorityColor() const
{
    switch (m_priority) {
    case 1:
        return "#e74c3c";  // 红色（高优先级）
    case 2:
        return "#f39c12";  // 橙色（中优先级）
    case 3:
        return "#3498db";  // 蓝色（低优先级）
    default:
        return "#95a5a6";  // 灰色（未知）
    }
}

// 判断是否逾期
bool Task::isOverdue() const
{
    // 如果已完成，不算逾期
    if (m_completed) {
        return false;
    }

    // 如果没有截止日期，不算逾期
    if (!m_dueDate.isValid()) {
        return false;
    }

    // 判断截止日期是否早于今天
    return m_dueDate < QDate::currentDate();
}

// 判断是否即将到期（3天内）
bool Task::isDueSoon() const
{
    // 如果已完成，不算即将到期
    if (m_completed) {
        return false;
    }

    // 如果没有截止日期，不算即将到期
    if (!m_dueDate.isValid()) {
        return false;
    }

    // 判断截止日期是否在3天内
    QDate today = QDate::currentDate();
    int daysTo = today.daysTo(m_dueDate);

    return daysTo >= 0 && daysTo <= 3;
}
