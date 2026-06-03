#include "TaskDialog.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>
#include <QCheckBox>

// ========================================
// 构造函数
// ========================================

// 添加模式构造函数
TaskDialog::TaskDialog(QWidget *parent)
    : QDialog(parent)
    , m_isEditMode(false)
{
    setWindowTitle("添加任务");
    setupUI();
    loadCategories();
}

// 编辑模式构造函数
TaskDialog::TaskDialog(const Task &task, QWidget *parent)
    : QDialog(parent)
    , m_isEditMode(true)
{
    setWindowTitle("编辑任务");
    setupUI();
    loadCategories();
    loadTaskData(task);
}

// ========================================
// UI 初始化方法
// ========================================

void TaskDialog::setupUI()
{
    // 设置对话框大小
    resize(500, 450);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ========================================
    // 基本信息组
    // ========================================

    QGroupBox *basicGroup = new QGroupBox("基本信息", this);
    QFormLayout *basicLayout = new QFormLayout(basicGroup);

    // 标题输入框
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText("请输入任务标题（必填）");
    basicLayout->addRow("标题：", m_titleEdit);

    // 描述输入框
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText("请输入任务描述（可选）");
    m_descriptionEdit->setMaximumHeight(100);
    basicLayout->addRow("描述：", m_descriptionEdit);

    mainLayout->addWidget(basicGroup);

    // ========================================
    // 分类和优先级组
    // ========================================

    QGroupBox *categoryGroup = new QGroupBox("分类和优先级", this);
    QFormLayout *categoryLayout = new QFormLayout(categoryGroup);

    // 分类下拉框
    m_categoryCombo = new QComboBox(this);
    categoryLayout->addRow("分类：", m_categoryCombo);

    // 优先级下拉框
    m_priorityCombo = new QComboBox(this);
    m_priorityCombo->addItem("高", 1);
    m_priorityCombo->addItem("中", 2);
    m_priorityCombo->addItem("低", 3);
    m_priorityCombo->setCurrentIndex(1);  // 默认选择"中"
    categoryLayout->addRow("优先级：", m_priorityCombo);

    mainLayout->addWidget(categoryGroup);

    // ========================================
    // 截止日期组
    // ========================================

    QGroupBox *dateGroup = new QGroupBox("截止日期", this);
    QVBoxLayout *dateLayout = new QVBoxLayout(dateGroup);

    // 截止日期选择器
    QHBoxLayout *dateEditLayout = new QHBoxLayout();
    m_dueDateEdit = new QDateEdit(this);
    m_dueDateEdit->setCalendarPopup(true);  // 启用日历弹出
    m_dueDateEdit->setDate(QDate::currentDate().addDays(7));  // 默认7天后
    m_dueDateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEditLayout->addWidget(new QLabel("日期：", this));
    dateEditLayout->addWidget(m_dueDateEdit);
    dateEditLayout->addStretch();

    // 清除日期按钮
    QPushButton *clearDateButton = new QPushButton("清除日期", this);
    connect(clearDateButton, &QPushButton::clicked, this, [this]() {
        m_dueDateEdit->setDate(QDate());
        m_dueDateEdit->setSpecialValueText("无截止日期");
    });
    dateEditLayout->addWidget(clearDateButton);

    dateLayout->addLayout(dateEditLayout);

    // 提示信息
    QLabel *dateHint = new QLabel("提示：可以清除日期表示无截止日期", this);
    dateHint->setStyleSheet("color: #7f8c8d; font-size: 11px;");
    dateLayout->addWidget(dateHint);

    mainLayout->addWidget(dateGroup);

    // ========================================
    // 按钮框
    // ========================================

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText("确定");
    m_buttonBox->button(QDialogButtonBox::Cancel)->setText("取消");

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &TaskDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &TaskDialog::onRejected);

    mainLayout->addWidget(m_buttonBox);

    // 添加弹簧
    mainLayout->addStretch();
}

void TaskDialog::loadCategories()
{
    // 清空下拉框
    m_categoryCombo->clear();

    // 加载分类列表
    QList<Category> categories = DatabaseManager::instance().getAllCategories();

    for (const Category &category : categories) {
        m_categoryCombo->addItem(category.name(), category.id());
    }

    // 默认选择第一个分类
    if (m_categoryCombo->count() > 0) {
        m_categoryCombo->setCurrentIndex(0);
    }
}

void TaskDialog::loadTaskData(const Task &task)
{
    // 填充任务数据（编辑模式）
    m_titleEdit->setText(task.title());
    m_descriptionEdit->setPlainText(task.description());

    // 设置分类
    for (int i = 0; i < m_categoryCombo->count(); ++i) {
        if (m_categoryCombo->itemData(i).toInt() == task.categoryId()) {
            m_categoryCombo->setCurrentIndex(i);
            break;
        }
    }

    // 设置优先级
    for (int i = 0; i < m_priorityCombo->count(); ++i) {
        if (m_priorityCombo->itemData(i).toInt() == task.priority()) {
            m_priorityCombo->setCurrentIndex(i);
            break;
        }
    }

    // 设置截止日期
    if (task.dueDate().isValid()) {
        m_dueDateEdit->setDate(task.dueDate());
    } else {
        m_dueDateEdit->setDate(QDate());
        m_dueDateEdit->setSpecialValueText("无截止日期");
    }
}

// ========================================
// 数据验证方法
// ========================================

bool TaskDialog::validateInput()
{
    // 验证标题
    if (m_titleEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "任务标题不能为空！");
        m_titleEdit->setFocus();
        return false;
    }

    // 验证标题长度
    if (m_titleEdit->text().length() > 100) {
        QMessageBox::warning(this, "验证失败", "任务标题不能超过100个字符！");
        m_titleEdit->setFocus();
        return false;
    }

    // 验证描述长度
    if (m_descriptionEdit->toPlainText().length() > 500) {
        QMessageBox::warning(this, "验证失败", "任务描述不能超过500个字符！");
        m_descriptionEdit->setFocus();
        return false;
    }

    return true;
}

// ========================================
// 槽函数
// ========================================

void TaskDialog::onAccepted()
{
    // 验证输入
    if (!validateInput()) {
        return;  // 验证失败，不关闭对话框
    }

    // 验证通过，接受对话框
    accept();
}

void TaskDialog::onRejected()
{
    // 取消对话框
    reject();
}

// ========================================
// 获取任务数据
// ========================================

Task TaskDialog::getTask() const
{
    Task task;

    // 设置任务数据
    task.setTitle(m_titleEdit->text().trimmed());
    task.setDescription(m_descriptionEdit->toPlainText().trimmed());
    task.setCategoryId(m_categoryCombo->currentData().toInt());
    task.setPriority(m_priorityCombo->currentData().toInt());

    // 设置截止日期
    if (m_dueDateEdit->date().isValid()) {
        task.setDueDate(m_dueDateEdit->date());
    } else {
        task.setDueDate(QDate());  // 无截止日期
    }

    // 默认未完成
    task.setCompleted(false);

    return task;
}

