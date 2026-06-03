# Qt 更多控件与高级布局 - 学习总结

**日期**：2026-05-30（第39天）  
**学习主题**：Qt 更多控件与高级布局管理  

---

## 📚 今日学习内容回顾

### 第一部分：更多 Qt 控件

#### 1. QComboBox（下拉框）

**核心功能**：
- 提供下拉选项供用户选择
- 节省界面空间
- 限制用户输入范围

**关键方法**：
```cpp
QComboBox *combo = new QComboBox();

// 添加选项
combo->addItem("选项1");
combo->addItems({"选项2", "选项3", "选项4"});

// 获取当前选项
QString text = combo->currentText();
int index = combo->currentIndex();

// 设置当前选项
combo->setCurrentIndex(0);
combo->setCurrentText("选项1");

// 清空所有选项
combo->clear();
```

**常用信号**：
```cpp
// 选项改变时触发
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [=](int index) {
    QString text = combo->itemText(index);
    // 处理选项改变
});
```

---

#### 2. QCheckBox（复选框）

**核心功能**：
- 允许用户多选
- 可以设置选中/未选中状态
- 支持三态（选中、未选中、部分选中）

**关键方法**：
```cpp
QCheckBox *check = new QCheckBox("选项");

// 设置选中状态
check->setChecked(true);

// 获取选中状态
bool isChecked = check->isChecked();

// 设置三态模式
check->setTristate(true);
```

**常用信号**：
```cpp
// 状态改变时触发
connect(check, &QCheckBox::stateChanged, [=](int state) {
    // state: 0=未选中, 1=部分选中, 2=选中
    if (state == Qt::Checked) {
        // 处理选中
    }
});
```

---

#### 3. QRadioButton（单选按钮）

**核心功能**：
- 只能选择一个选项
- 通常配合 QButtonGroup 使用
- 自动实现互斥选择

**关键方法**：
```cpp
QRadioButton *radio1 = new QRadioButton("选项1");
QRadioButton *radio2 = new QRadioButton("选项2");

// 设置默认选中
radio1->setChecked(true);

// 获取选中状态
bool isChecked = radio1->isChecked();

// 创建按钮组（确保互斥）
QButtonGroup *group = new QButtonGroup();
group->addButton(radio1);
group->addButton(radio2);
```

**常用信号**：
```cpp
// 状态切换时触发
connect(radio1, &QRadioButton::toggled, [=](bool checked) {
    if (checked) {
        // 处理选中
    }
});
```

---

#### 4. QSpinBox（数字输入框）

**核心功能**：
- 限制输入为数字
- 可以设置范围和步长
- 支持键盘和鼠标操作

**关键方法**：
```cpp
QSpinBox *spin = new QSpinBox();

// 设置范围
spin->setRange(0, 100);
spin->setMinimum(0);
spin->setMaximum(100);

// 设置步长
spin->setSingleStep(1);

// 设置当前值
spin->setValue(50);

// 获取当前值
int value = spin->value();

// 设置后缀
spin->setSuffix(" 岁");
```

**常用信号**：
```cpp
// 值改变时触发
connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
        [=](int value) {
    // 处理值改变
});
```

---

### 第二部分：高级布局管理

#### 1. QGridLayout（网格布局）

**核心功能**：
- 按行列排列控件
- 支持跨行跨列
- 适合计算器、表格等布局

**关键方法**：
```cpp
QGridLayout *grid = new QGridLayout();

// 添加控件到指定位置
grid->addWidget(widget, row, col);

// 跨行跨列
grid->addWidget(widget, row, col, rowSpan, colSpan);

// 示例：跨4列
grid->addWidget(display, 0, 0, 1, 4);  // 第0行，第0列，跨1行4列

// 设置间距
grid->setSpacing(10);
grid->setHorizontalSpacing(5);
grid->setVerticalSpacing(5);

// 设置边距
grid->setContentsMargins(10, 10, 10, 10);
```

**实际应用**：
```cpp
// 计算器布局
QGridLayout *grid = new QGridLayout();

// 显示屏（跨4列）
grid->addWidget(display, 0, 0, 1, 4);

// 按钮（4x4网格）
grid->addWidget(btn7, 1, 0);
grid->addWidget(btn8, 1, 1);
grid->addWidget(btn9, 1, 2);
grid->addWidget(btnDiv, 1, 3);
// ...
```

---

#### 2. QFormLayout（表单布局）

**核心功能**：
- 专门用于表单布局
- 自动对齐标签和控件
- 标签-控件对的形式

**关键方法**：
```cpp
QFormLayout *form = new QFormLayout();

// 添加行（标签 + 控件）
form->addRow("姓名：", nameEdit);
form->addRow("年龄：", ageSpinBox);

// 添加行（QString + 控件）
form->addRow("邮箱：", new QLineEdit());

// 设置标签对齐
form->setLabelAlignment(Qt::AlignRight);

// 设置字段增长策略
form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
```

**实际应用**：
```cpp
QFormLayout *form = new QFormLayout();
form->addRow("姓名：", new QLineEdit());
form->addRow("年龄：", new QSpinBox());
form->addRow("性别：", new QComboBox());
form->addRow("邮箱：", new QLineEdit());
```

---

#### 3. 布局嵌套技巧

**核心思想**：
- 组合使用多种布局
- 主布局 + 子布局
- 实现复杂的界面结构

**示例1：顶部工具栏 + 中间内容 + 底部按钮**
```cpp
QVBoxLayout *mainLayout = new QVBoxLayout();

// 顶部：水平布局
QHBoxLayout *topLayout = new QHBoxLayout();
topLayout->addWidget(title);
topLayout->addStretch();
topLayout->addWidget(closeBtn);
mainLayout->addLayout(topLayout);

// 中间：表单布局
QFormLayout *formLayout = new QFormLayout();
formLayout->addRow("姓名：", nameEdit);
formLayout->addRow("年龄：", ageSpinBox);
mainLayout->addLayout(formLayout);

// 底部：水平布局
QHBoxLayout *bottomLayout = new QHBoxLayout();
bottomLayout->addStretch();
bottomLayout->addWidget(submitBtn);
bottomLayout->addWidget(cancelBtn);
mainLayout->addLayout(bottomLayout);
```

**示例2：左右分栏布局**
```cpp
QHBoxLayout *mainLayout = new QHBoxLayout();

// 左侧：垂直布局
QVBoxLayout *leftLayout = new QVBoxLayout();
leftLayout->addWidget(menu1);
leftLayout->addWidget(menu2);
leftLayout->addStretch();
mainLayout->addLayout(leftLayout);

// 右侧：内容区域
QVBoxLayout *rightLayout = new QVBoxLayout();
rightLayout->addWidget(content);
mainLayout->addLayout(rightLayout);
```

---

### 第三部分：用户注册表单实战

#### 功能清单

1. **表单字段**：
   - 姓名（QLineEdit）
   - 性别（QRadioButton）
   - 年龄（QSpinBox）
   - 城市（QComboBox）
   - 邮箱（QLineEdit）
   - 电话（QLineEdit）
   - 爱好（QCheckBox）
   - 简介（QTextEdit）

2. **表单验证**：
   - 姓名非空，长度2-20字符
   - 年龄范围18-100
   - 邮箱格式验证（包含@和.）
   - 电话号码11位
   - 至少选择一个爱好
   - 简介长度50-200字

3. **用户反馈**：
   - 验证失败：QMessageBox::warning()
   - 提交成功：QMessageBox::information()
   - 显示收集的数据

#### 核心代码

**验证示例**：
```cpp
void onSubmit()
{
    // 1. 验证姓名
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请输入姓名！");
        m_nameEdit->setFocus();
        return;
    }

    // 2. 验证邮箱
    QString email = m_emailEdit->text().trimmed();
    if (!email.contains("@") || !email.contains(".")) {
        QMessageBox::warning(this, "验证失败", "邮箱格式不正确！");
        return;
    }

    // 3. 验证爱好
    QStringList hobbies;
    if (m_hobby1->isChecked()) hobbies << "读书";
    if (m_hobby2->isChecked()) hobbies << "运动";
    
    if (hobbies.isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请至少选择一个爱好！");
        return;
    }

    // 验证通过，显示信息
    QMessageBox::information(this, "注册成功", "注册信息已提交！");
}
```

---

## 🎯 核心知识点总结

### 1. 控件选择指南

| 需求 | 推荐控件 | 说明 |
|------|---------|------|
| 单选（预定义选项） | QComboBox | 节省空间 |
| 单选（少量选项） | QRadioButton | 直观清晰 |
| 多选 | QCheckBox | 允许多选 |
| 数字输入 | QSpinBox | 限制范围 |
| 文本输入 | QLineEdit | 单行文本 |
| 长文本输入 | QTextEdit | 多行文本 |

### 2. 布局选择指南

| 场景 | 推荐布局 | 说明 |
|------|---------|------|
| 简单列表 | QVBoxLayout | 垂直排列 |
| 工具栏 | QHBoxLayout | 水平排列 |
| 计算器 | QGridLayout | 网格排列 |
| 表单 | QFormLayout | 标签-控件对 |
| 复杂界面 | 嵌套布局 | 组合使用 |

### 3. 常用信号对比

| 控件 | 常用信号 | 参数 |
|-----|---------|------|
| QComboBox | currentIndexChanged(int) | 新索引 |
| QCheckBox | stateChanged(int) | 新状态 |
| QRadioButton | toggled(bool) | 是否选中 |
| QSpinBox | valueChanged(int) | 新值 |

---

## 💡 最佳实践

### 1. 表单验证原则

- **即时反馈**：验证失败立即提示
- **焦点管理**：验证失败后将焦点设置到错误字段
- **清晰提示**：错误信息要具体明确
- **友好体验**：使用 QMessageBox 提供反馈

### 2. 布局设计原则

- **合理间距**：使用 setSpacing() 设置控件间距
- **适当边距**：使用 setContentsMargins() 设置边距
- **弹性空间**：使用 addStretch() 实现灵活布局
- **对齐方式**：使用 setAlignment() 设置对齐

### 3. 代码组织原则

- **成员变量**：控件指针作为成员变量
- **信号槽**：使用 Lambda 表达式简化代码
- **验证逻辑**：独立的验证函数
- **数据收集**：统一的数据收集方法

---

## 📖 学习资源

### 官方文档
- QComboBox：https://doc.qt.io/qt-5/qcombobox.html
- QCheckBox：https://doc.qt.io/qt-5/qcheckbox.html
- QRadioButton：https://doc.qt.io/qt-5/qradiobutton.html
- QSpinBox：https://doc.qt.io/qt-5/qspinbox.html
- QGridLayout：https://doc.qt.io/qt-5/qgridlayout.html
- QFormLayout：https://doc.qt.io/qt-5/qformlayout.html

### 示例代码
- Qt Widgets Examples
- Qt Layouts Examples

---

## ✅ 学习检查清单

完成以下检查，确认今日学习目标达成：

### 控件使用
- [x] 掌握 QComboBox 的使用（添加选项、获取选项、信号）
- [x] 掌握 QCheckBox 的使用（设置状态、获取状态、信号）
- [x] 掌握 QRadioButton 的使用（互斥选择、按钮组）
- [x] 掌握 QSpinBox 的使用（范围、步长、信号）

### 布局管理
- [x] 理解 QGridLayout 的网格布局方式
- [x] 理解 QFormLayout 的表单布局方式
- [x] 掌握布局嵌套技巧

### 实战能力
- [x] 能够创建完整的用户注册表单
- [x] 能够实现表单验证功能
- [x] 能够收集和显示表单数据
- [x] 能够提供用户反馈

---

## 🔜 下一步学习计划

### 明天学习内容（第40天）

**主题**：Qt 样式表与美化

**学习目标**：
1. 学习 Qt Style Sheets（类似 CSS）
2. 美化控件外观
3. 创建主题样式
4. 实现响应式设计

**预计时长**：2小时

---

**今日学习完成！掌握了更多 Qt 控件和高级布局，明天继续学习样式美化！** 🎉
