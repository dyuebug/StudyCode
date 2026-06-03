#ifndef CATEGORY_H
#define CATEGORY_H

#include <QString>

/**
 * @brief Category 类 - 分类数据类
 *
 * 表示一个任务分类的信息
 */
class Category {
public:
    // 默认构造函数
    Category();

    // 完整构造函数
    Category(int id, const QString &name, const QString &color);

    // ========================================
    // Getters（获取器）
    // ========================================

    int id() const { return m_id; }
    QString name() const { return m_name; }
    QString color() const { return m_color; }

    // ========================================
    // Setters（设置器）
    // ========================================

    void setId(int id) { m_id = id; }
    void setName(const QString &name) { m_name = name; }
    void setColor(const QString &color) { m_color = color; }

private:
    int m_id;           // 分类ID
    QString m_name;     // 分类名称
    QString m_color;    // 分类颜色（十六进制颜色值，如 "#3498db"）
};

#endif // CATEGORY_H
