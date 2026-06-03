#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include "Task.h"

/**
 * @brief TaskDialog 类 - 任务对话框
 *
 * 用于添加新任务或编辑现有任务
 */
class TaskDialog : public QDialog {
    Q_OBJECT

public:
    // ========================================
    // 构造函数
    // ========================================

    // 添加模式构造函数
    explicit TaskDialog(QWidget *parent = nullptr);

    // 编辑模式构造函数
    explicit TaskDialog(const Task &task, QWidget *parent = nullptr);

    // ========================================
    // 获取任务数据
    // ========================================

    // 获取对话框中的任务数据
    Task getTask() const;

private slots:
    // ========================================
    // 槽函数
    // ========================================

    void onAccepted();      // 确定按钮点击
    void onRejected();      // 取消按钮点击

private:
    // ========================================
    // UI 初始化方法
    // ========================================

    void setupUI();         // 设置 UI
    void loadCategories();  // 加载分类列表
    void loadTaskData(const Task &task);  // 加载任务数据（编辑模式）

    // ========================================
    // 数据验证方法
    // ========================================

    bool validateInput();   // 验证输入数据

    // ========================================
    // UI 控件
    // ========================================

    QLineEdit *m_titleEdit;         // 标题输入框
    QTextEdit *m_descriptionEdit;   // 描述输入框
    QComboBox *m_categoryCombo;     // 分类下拉框
    QComboBox *m_priorityCombo;     // 优先级下拉框
    QDateEdit *m_dueDateEdit;       // 截止日期选择器
    QDialogButtonBox *m_buttonBox;  // 按钮框

    // ========================================
    // 数据成员
    // ========================================

    bool m_isEditMode;      // 是否为编辑模式
};

#endif // TASKDIALOG_H
