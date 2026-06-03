#include "Category.h"

// ========================================
// 构造函数
// ========================================

// 默认构造函数
Category::Category()
    : m_id(0)
    , m_color("#3498db")  // 默认蓝色
{
}

// 完整构造函数
Category::Category(int id, const QString &name, const QString &color)
    : m_id(id)
    , m_name(name)
    , m_color(color)
{
}
