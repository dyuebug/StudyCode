# Qt 图形界面编程 — 阶段7知识点速查表

> 日期：2026-06-04（第44天）  
> 阶段：阶段7 - Qt 图形界面编程  
> 用途：快速查阅 Qt 常用 API，无需翻文档

---

## 一、基础控件速查

### QLabel（标签）
```cpp
QLabel *label = new QLabel("文字");
label->setText("新文字");           // 设置文字
label->setPixmap(QPixmap("a.png")); // 显示图片
label->setAlignment(Qt::AlignCenter); // 居中对齐
label->setWordWrap(true);           // 自动换行
label->setStyleSheet("color: red; font-size: 14px;");
```

### QLineEdit（单行输入框）
```cpp
QLineEdit *edit = new QLineEdit();
edit->setPlaceholderText("提示文字");
edit->setText("内容");
edit->text();                        // 获取文字
edit->setEchoMode(QLineEdit::Password); // 密码模式
edit->setReadOnly(true);             // 只读
edit->setMaxLength(20);              // 最大长度
// 信号
connect(edit, &QLineEdit::textChanged, ...);   // 文字改变
connect(edit, &QLineEdit::returnPressed, ...); // 按回车
```

### QPushButton（按钮）
```cpp
QPushButton *btn = new QPushButton("点击");
btn->setEnabled(false);   // 禁用
btn->setCheckable(true);  // 可切换状态
btn->isChecked();         // 是否选中
btn->setDefault(true);    // 设为默认按钮（回车触发）
// 信号
connect(btn, &QPushButton::clicked, this, &MyClass::onClicked);
```

### QTextEdit（多行文本编辑器）
```cpp
QTextEdit *edit = new QTextEdit();
edit->setPlainText("纯文本");
edit->toPlainText();              // 获取纯文本
edit->setReadOnly(true);          // 只读
edit->setFont(QFont("Arial", 12));
edit->setTextColor(Qt::red);      // 设置文字颜色
edit->find("关键词");             // 查找（高亮选中）
edit->document()->isModified();   // 是否有未保存修改
edit->document()->setModified(false); // 清除修改标记
// 内置槽
edit->undo(); edit->redo();
edit->cut(); edit->copy(); edit->paste();
edit->selectAll();
// 信号
connect(edit, &QTextEdit::textChanged, ...);
connect(edit->document(), &QTextDocument::undoAvailable, ...);
```

### QComboBox（下拉框）
```cpp
QComboBox *combo = new QComboBox();
combo->addItem("选项1");
combo->addItems({"A", "B", "C"});
combo->currentText();             // 当前文字
combo->currentIndex();            // 当前索引
combo->setCurrentIndex(0);        // 设置选中项
combo->count();                   // 选项总数
// 信号
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), ...);
```

### QCheckBox（复选框）
```cpp
QCheckBox *cb = new QCheckBox("选项");
cb->setChecked(true);
cb->isChecked();                  // 是否选中
// 信号
connect(cb, &QCheckBox::stateChanged, ...);
connect(cb, &QCheckBox::toggled, ...);
```

### QRadioButton（单选按钮）
```cpp
QRadioButton *rb = new QRadioButton("选项");
rb->setChecked(true);
rb->isChecked();
// 互斥分组
QButtonGroup *group = new QButtonGroup(this);
group->addButton(rb1);
group->addButton(rb2);
// 信号
connect(rb, &QRadioButton::toggled, ...);
```

### QSpinBox（整数输入框）
```cpp
QSpinBox *spin = new QSpinBox();
spin->setRange(0, 100);
spin->setValue(25);
spin->value();
spin->setSuffix(" 岁");
spin->setSingleStep(1);
// 信号
connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), ...);
```

### QSlider（滑块）
```cpp
QSlider *slider = new QSlider(Qt::Horizontal);
slider->setRange(0, 100);
slider->setValue(50);
slider->value();
// 信号
connect(slider, &QSlider::valueChanged, ...);
```

### QListWidget（列表控件）
```cpp
QListWidget *list = new QListWidget();
list->addItem("条目1");
list->addItems({"A", "B", "C"});
list->currentItem()->text();      // 当前选中项文字
list->currentRow();               // 当前行索引
list->takeItem(row);              // 移除并返回条目
list->clear();                    // 清空
// 信号
connect(list, &QListWidget::itemClicked, ...);
connect(list, &QListWidget::currentRowChanged, ...);
```

---

## 二、布局管理速查

```cpp
// QVBoxLayout — 垂直排列
QVBoxLayout *vbox = new QVBoxLayout(parent);
vbox->addWidget(widget);
vbox->addLayout(subLayout);
vbox->addStretch();               // 弹性空间
vbox->addSpacing(10);             // 固定间距
vbox->setSpacing(8);              // 控件间距
vbox->setContentsMargins(10,10,10,10); // 边距

// QHBoxLayout — 水平排列（用法同上）

// QGridLayout — 网格排列
QGridLayout *grid = new QGridLayout();
grid->addWidget(w, row, col);             // 指定位置
grid->addWidget(w, row, col, rowSpan, colSpan); // 跨行列
grid->setSpacing(8);

// QFormLayout — 表单排列
QFormLayout *form = new QFormLayout();
form->addRow("标签：", widget);
form->addRow(labelWidget, inputWidget);
form->setLabelAlignment(Qt::AlignRight);

// 布局选择原则
// 简单列表 → QVBoxLayout
// 工具栏   → QHBoxLayout
// 计算器   → QGridLayout
// 表单     → QFormLayout
// 复杂界面 → 嵌套布局
```

---

## 三、对话框速查

### QMessageBox
```cpp
// 四种类型（静态方法，一行调用）
QMessageBox::information(parent, "标题", "内容");
QMessageBox::warning(parent, "标题", "内容");
QMessageBox::critical(parent, "标题", "内容");

// 询问（必须检查返回值！）
auto r = QMessageBox::question(parent, "标题", "内容",
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
if (r == QMessageBox::Yes) { ... }

// 三选项（保存/放弃/取消）
auto r = QMessageBox::question(parent, "标题", "内容",
    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
```

### QInputDialog
```cpp
bool ok;
QString s = QInputDialog::getText(parent, "标题", "提示", QLineEdit::Normal, "默认值", &ok);
int n     = QInputDialog::getInt(parent, "标题", "提示", 默认值, 最小, 最大, 步长, &ok);
double d  = QInputDialog::getDouble(parent, "标题", "提示", 默认值, 最小, 最大, 小数位, &ok);
QString item = QInputDialog::getItem(parent, "标题", "提示", QStringList列表, 默认索引, 可编辑, &ok);
// 必须检查 ok！
```

### QFileDialog
```cpp
// 打开单文件（取消返回空字符串，必须检查！）
QString f = QFileDialog::getOpenFileName(parent, "标题", 初始目录, "过滤器 (*.txt)");
// 打开多文件
QStringList fs = QFileDialog::getOpenFileNames(parent, "标题", 目录, 过滤器);
// 保存文件
QString f = QFileDialog::getSaveFileName(parent, "标题", 默认路径, 过滤器);
// 选择目录
QString d = QFileDialog::getExistingDirectory(parent, "标题", 初始目录);
// 过滤器格式："图片 (*.png *.jpg);;文本 (*.txt);;所有 (*.*)"
```

### QColorDialog / QFontDialog
```cpp
QColor c = QColorDialog::getColor(初始颜色, parent, "标题");
if (c.isValid()) { ... }  // 必须检查！

bool ok;
QFont f = QFontDialog::getFont(&ok, 初始字体, parent, "标题");
if (ok) { ... }
```

### 自定义 QDialog
```cpp
class MyDialog : public QDialog {
    Q_OBJECT
public:
    explicit MyDialog(QWidget *parent = nullptr) : QDialog(parent) {
        // 创建控件和布局
        QDialogButtonBox *box = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(box, &QDialogButtonBox::accepted, this, &MyDialog::onAccept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    QString getData() const { return m_edit->text(); }
private slots:
    void onAccept() {
        if (验证失败) return;  // 不关闭
        accept();              // 关闭，返回 Accepted
    }
};

// 使用
MyDialog dlg(this);
if (dlg.exec() == QDialog::Accepted) {
    QString data = dlg.getData();
}

// 非模态（堆上创建 + WA_DeleteOnClose）
MyDialog *dlg = new MyDialog(this);
dlg->setAttribute(Qt::WA_DeleteOnClose);
dlg->show();
```

---

## 四、Qt Style Sheets 速查

```css
/* 选择器 */
QPushButton { }           /* 类型选择器 */
#myButton { }             /* ID选择器（setObjectName） */
QPushButton:hover { }     /* 伪状态 */
QPushButton:pressed { }
QPushButton:disabled { }
QCheckBox:checked { }
QLineEdit:focus { }
QComboBox::drop-down { }  /* 子控件（双冒号） */
QCheckBox::indicator { }

/* 常用属性 */
color: #333;
background-color: #fff;
background: linear-gradient(#fff, #eee);
border: 1px solid #ccc;
border-radius: 5px;
border-top: 2px solid blue;
padding: 8px 16px;        /* 上下 左右 */
margin: 4px;
font-size: 14px;
font-weight: bold;
font-family: "Arial";
min-width: 80px;
min-height: 30px;

/* 设置方式 */
widget->setStyleSheet("...");           // 单个控件
qApp->setStyleSheet("...");             // 全局
// 外部文件
QFile f("style.qss");
f.open(QFile::ReadOnly);
qApp->setStyleSheet(f.readAll());
```

---

## 五、QMainWindow 速查

```cpp
class MyWindow : public QMainWindow {
public:
    MyWindow() {
        setCentralWidget(editor);      // 设置中央控件

        // 菜单栏
        QMenu *menu = menuBar()->addMenu("文件(&F)");
        menu->addAction(action);
        menu->addSeparator();

        // 工具栏
        QToolBar *tb = addToolBar("工具栏");
        tb->addAction(action);         // 复用 QAction
        tb->setMovable(false);

        // 状态栏
        statusBar()->showMessage("就绪", 3000);  // 3秒后消失
        statusBar()->addPermanentWidget(label);  // 永久显示
    }

    void closeEvent(QCloseEvent *event) override {
        if (可以关闭) event->accept();
        else event->ignore();
    }
};

// QAction
QAction *act = new QAction("保存(&S)", this);
act->setShortcut(QKeySequence::Save);  // 标准快捷键
act->setToolTip("保存文件");
act->setEnabled(false);
connect(act, &QAction::triggered, this, &MyWindow::onSave);
// 标准快捷键：New/Open/Save/SaveAs/Quit/Undo/Redo/Cut/Copy/Paste/SelectAll
```

---

## 六、文件读写速查

```cpp
// 读文件
QFile file("path/to/file.txt");
if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();
}

// 写文件
QFile file("path/to/file.txt");
if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    file.close();
}

// 文件信息
QFileInfo info("path/to/file.txt");
info.fileName();      // 文件名（不含路径）
info.baseName();      // 文件名（不含扩展名）
info.suffix();        // 扩展名
info.absolutePath();  // 绝对路径
info.size();          // 文件大小（字节）
info.exists();        // 是否存在
```

---

## 七、信号与槽常用模式速查

```cpp
// 1. 成员函数槽
connect(btn, &QPushButton::clicked, this, &MyClass::onClicked);

// 2. Lambda 槽（捕获变量）
connect(btn, &QPushButton::clicked, this, [this]() {
    doSomething();
});

// 3. Lambda 捕获循环变量（必须值捕获！）
QString text = item.text;  // 先复制
connect(btn, &QPushButton::clicked, this, [this, text]() {
    process(text);
});

// 4. 直接连接到另一个控件的槽
connect(undoAction, &QAction::triggered, editor, &QTextEdit::undo);

// 5. 带参数的信号
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MyClass::onIndexChanged);

// 6. 跨线程信号（Qt::QueuedConnection）
connect(worker, &Worker::finished, this, &MyClass::onFinished,
        Qt::QueuedConnection);
```

---

## 八、阶段7完成项目清单

| 项目 | 文件 | 核心技术 |
|------|------|---------|
| 控件演示 | 05_more_widgets_demo.cpp | QComboBox/QCheckBox/QRadioButton/QSpinBox |
| 高级布局 | 06_advanced_layouts.cpp | QGridLayout/QFormLayout/布局嵌套 |
| 注册表单 | 07_registration_form.cpp | 表单验证/QMessageBox |
| 样式美化 | 09~11 | Qt Style Sheets/主题切换 |
| 标准对话框 | 12_dialogs_basics.cpp | QMessageBox/QInputDialog/QFileDialog |
| 自定义对话框 | 13_custom_dialog.cpp | QDialog/exec/show/QDialogButtonBox |
| 对话框实战 | 14_dialog_practice.cpp | 综合对话框集成 |
| 简单计算器 | 15_calculator.cpp | 状态机/QGridLayout/Lambda捕获 |
| 记事本 | 16_notepad.cpp | QMainWindow/QAction/QFile/closeEvent |
