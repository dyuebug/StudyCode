/*
 * Qt 数据库实战：联系人管理应用
 * 日期：2026-06-05（第45天）
 * 难度：⭐⭐ 中级
 *
 * 综合运用：
 * - QSqlDatabase + QSqlQuery（数据库操作）
 * - QSqlTableModel + QTableView（数据展示）
 * - QDialog（添加/编辑联系人）
 * - QSortFilterProxyModel（搜索过滤）
 * - 数据在程序重启后仍然保留
 *
 * 编译前必须在 .pro 文件添加：QT += sql
 */

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>
#include <QStatusBar>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDateTime>

// ============================================
// 添加/编辑联系人对话框
// ============================================
class ContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContactDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("添加联系人");
        setMinimumWidth(320);

        QFormLayout *form = new QFormLayout();

        m_nameEdit  = new QLineEdit();
        m_phoneEdit = new QLineEdit();
        m_emailEdit = new QLineEdit();
        m_noteEdit  = new QLineEdit();

        m_nameEdit->setPlaceholderText("必填");
        m_phoneEdit->setPlaceholderText("如：13800138000");
        m_emailEdit->setPlaceholderText("如：user@example.com");
        m_noteEdit->setPlaceholderText("可选备注");

        form->addRow("姓名 *：",  m_nameEdit);
        form->addRow("电话：",    m_phoneEdit);
        form->addRow("邮箱：",    m_emailEdit);
        form->addRow("备注：",    m_noteEdit);

        QDialogButtonBox *box = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(box, &QDialogButtonBox::accepted, this, &ContactDialog::onAccept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addLayout(form);
        layout->addWidget(box);
    }

    // 设置编辑模式（填入已有数据）
    void setEditMode(const QString &name, const QString &phone,
                     const QString &email, const QString &note)
    {
        setWindowTitle("编辑联系人");
        m_nameEdit->setText(name);
        m_phoneEdit->setText(phone);
        m_emailEdit->setText(email);
        m_noteEdit->setText(note);
    }

    QString name()  const { return m_nameEdit->text().trimmed(); }
    QString phone() const { return m_phoneEdit->text().trimmed(); }
    QString email() const { return m_emailEdit->text().trimmed(); }
    QString note()  const { return m_noteEdit->text().trimmed(); }

private slots:
    void onAccept()
    {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "姓名不能为空！");
            m_nameEdit->setFocus();
            return;
        }
        accept();
    }

private:
    QLineEdit *m_nameEdit;
    QLineEdit *m_phoneEdit;
    QLineEdit *m_emailEdit;
    QLineEdit *m_noteEdit;
};

// ============================================
// 联系人管理主窗口
// ============================================
class ContactManager : public QMainWindow
{
    Q_OBJECT

public:
    ContactManager(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("联系人管理");
        resize(750, 500);

        // 初始化数据库
        initDb();

        // 中央控件
        QWidget *central = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(central);
        layout->setSpacing(10);
        layout->setContentsMargins(16, 16, 16, 16);
        setCentralWidget(central);

        // 标题 + 搜索栏
        QHBoxLayout *topLayout = new QHBoxLayout();
        QLabel *title = new QLabel("📋 联系人管理");
        title->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
        topLayout->addWidget(title);
        topLayout->addStretch();

        m_searchEdit = new QLineEdit();
        m_searchEdit->setPlaceholderText("🔍 搜索姓名或电话...");
        m_searchEdit->setFixedWidth(200);
        topLayout->addWidget(m_searchEdit);
        layout->addLayout(topLayout);

        // 表格视图
        m_tableView = new QTableView();
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setAlternatingRowColors(true);
        m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止直接编辑，通过对话框编辑
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->verticalHeader()->setVisible(false);
        m_tableView->setSortingEnabled(true);
        m_tableView->setStyleSheet(R"(
            QTableView {
                border: 1px solid #dee2e6;
                border-radius: 6px;
                font-size: 13px;
                gridline-color: #f0f0f0;
            }
            QTableView::item { padding: 6px; }
            QTableView::item:selected { background: #cfe2ff; color: #000; }
            QHeaderView::section {
                background: #f8f9fa;
                border: none;
                border-bottom: 2px solid #dee2e6;
                padding: 8px;
                font-weight: bold;
                color: #495057;
            }
        )");
        layout->addWidget(m_tableView);

        // 创建模型
        m_model = new QSqlTableModel(this);
        m_model->setTable("contacts");
        m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        m_model->select();
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "姓名");
        m_model->setHeaderData(2, Qt::Horizontal, "电话");
        m_model->setHeaderData(3, Qt::Horizontal, "邮箱");
        m_model->setHeaderData(4, Qt::Horizontal, "备注");
        m_model->setHeaderData(5, Qt::Horizontal, "添加时间");

        // 代理模型（搜索过滤）
        m_proxy = new QSortFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        m_proxy->setFilterKeyColumn(-1);  // -1 = 搜索所有列
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxy);
        m_tableView->setColumnHidden(0, true);  // 隐藏 ID 列
        m_tableView->setColumnWidth(1, 120);
        m_tableView->setColumnWidth(2, 140);
        m_tableView->setColumnWidth(3, 180);
        m_tableView->setColumnWidth(4, 120);

        // 操作按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();

        auto makeBtn = [](const QString &text, const QString &bg) {
            QPushButton *btn = new QPushButton(text);
            btn->setStyleSheet(
                QString("QPushButton{background:%1;color:white;border:none;"
                        "border-radius:5px;padding:8px 16px;font-size:13px;}"
                        "QPushButton:hover{opacity:0.85;}").arg(bg)
            );
            return btn;
        };

        QPushButton *addBtn    = makeBtn("➕ 添加联系人", "#27ae60");
        QPushButton *editBtn   = makeBtn("✏️ 编辑",       "#3498db");
        QPushButton *deleteBtn = makeBtn("🗑 删除",        "#e74c3c");

        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(editBtn);
        btnLayout->addWidget(deleteBtn);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        // 连接信号
        connect(addBtn,    &QPushButton::clicked, this, &ContactManager::addContact);
        connect(editBtn,   &QPushButton::clicked, this, &ContactManager::editContact);
        connect(deleteBtn, &QPushButton::clicked, this, &ContactManager::deleteContact);
        connect(m_tableView, &QTableView::doubleClicked, this, &ContactManager::editContact);
        connect(m_searchEdit, &QLineEdit::textChanged,
                m_proxy, &QSortFilterProxyModel::setFilterFixedString);

        updateStatusBar();
    }

private slots:

    void addContact()
    {
        ContactDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;

        QSqlQuery q;
        q.prepare(
            "INSERT INTO contacts (name, phone, email, note, created) "
            "VALUES (:name, :phone, :email, :note, :created)"
        );
        q.bindValue(":name",    dlg.name());
        q.bindValue(":phone",   dlg.phone());
        q.bindValue(":email",   dlg.email());
        q.bindValue(":note",    dlg.note());
        q.bindValue(":created", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));

        if (q.exec()) {
            m_model->select();  // 刷新模型
            updateStatusBar();
            statusBar()->showMessage(
                QString("✅ 已添加联系人：%1").arg(dlg.name()), 3000);
        } else {
            QMessageBox::critical(this, "错误", "添加失败：" + q.lastError().text());
        }
    }

    void editContact()
    {
        QModelIndex idx = m_tableView->currentIndex();
        if (!idx.isValid()) {
            QMessageBox::information(this, "提示", "请先选中要编辑的联系人！");
            return;
        }

        // 代理索引映射回源模型索引
        QModelIndex srcIdx = m_proxy->mapToSource(idx);
        int row = srcIdx.row();

        // 获取当前行数据
        int    id    = m_model->data(m_model->index(row, 0)).toInt();
        QString name  = m_model->data(m_model->index(row, 1)).toString();
        QString phone = m_model->data(m_model->index(row, 2)).toString();
        QString email = m_model->data(m_model->index(row, 3)).toString();
        QString note  = m_model->data(m_model->index(row, 4)).toString();

        ContactDialog dlg(this);
        dlg.setEditMode(name, phone, email, note);
        if (dlg.exec() != QDialog::Accepted) return;

        QSqlQuery q;
        q.prepare(
            "UPDATE contacts SET name=:name, phone=:phone, email=:email, note=:note "
            "WHERE id=:id"
        );
        q.bindValue(":name",  dlg.name());
        q.bindValue(":phone", dlg.phone());
        q.bindValue(":email", dlg.email());
        q.bindValue(":note",  dlg.note());
        q.bindValue(":id",    id);

        if (q.exec()) {
            m_model->select();
            statusBar()->showMessage(
                QString("✅ 已更新联系人：%1").arg(dlg.name()), 3000);
        } else {
            QMessageBox::critical(this, "错误", "更新失败：" + q.lastError().text());
        }
    }

    void deleteContact()
    {
        QModelIndex idx = m_tableView->currentIndex();
        if (!idx.isValid()) {
            QMessageBox::information(this, "提示", "请先选中要删除的联系人！");
            return;
        }

        QModelIndex srcIdx = m_proxy->mapToSource(idx);
        int row = srcIdx.row();
        int id  = m_model->data(m_model->index(row, 0)).toInt();
        QString name = m_model->data(m_model->index(row, 1)).toString();

        auto reply = QMessageBox::question(this, "确认删除",
            QString("确定要删除联系人"%1"吗？").arg(name),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        QSqlQuery q;
        q.prepare("DELETE FROM contacts WHERE id=:id");
        q.bindValue(":id", id);

        if (q.exec()) {
            m_model->select();
            updateStatusBar();
            statusBar()->showMessage(
                QString("🗑 已删除联系人：%1").arg(name), 3000);
        } else {
            QMessageBox::critical(this, "错误", "删除失败：" + q.lastError().text());
        }
    }

private:

    void initDb()
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("contacts.db");
        if (!db.open()) {
            QMessageBox::critical(this, "错误", "无法打开数据库！");
            return;
        }

        QSqlQuery q;
        q.exec(
            "CREATE TABLE IF NOT EXISTS contacts ("
            "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  name    TEXT NOT NULL,"
            "  phone   TEXT DEFAULT '',"
            "  email   TEXT DEFAULT '',"
            "  note    TEXT DEFAULT '',"
            "  created TEXT DEFAULT ''"
            ")"
        );
    }

    void updateStatusBar()
    {
        QSqlQuery q;
        q.exec("SELECT COUNT(*) FROM contacts");
        int total = q.next() ? q.value(0).toInt() : 0;
        statusBar()->showMessage(QString("共 %1 位联系人").arg(total));
    }

    QTableView            *m_tableView;
    QSqlTableModel        *m_model;
    QSortFilterProxyModel *m_proxy;
    QLineEdit             *m_searchEdit;
};

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QSqlTableModel::setFilter()】

语法：void setFilter(const QString &filter)
参数：SQL WHERE 子句（不含 WHERE 关键字）
作用：过滤显示的数据行

示例：
m_model->setFilter("name LIKE '%张%'");   // 名字含"张"
m_model->setFilter("");                   // 清空过滤，显示全部

────────────────────────────────────────────────────────────

【函数卡片：QSortFilterProxyModel】

作用：在 Model 和 View 之间插入排序/过滤层，不修改原始数据

常用方法：
- setSourceModel(model)：设置原始 model
- setFilterKeyColumn(col)：按哪列过滤，-1 表示所有列
- setFilterCaseSensitivity(Qt::CaseInsensitive)：不区分大小写
- setFilterFixedString(str) / setFilterRegularExpression(re)：设置过滤词

示例：
auto *proxy = new QSortFilterProxyModel(this);
proxy->setSourceModel(m_model);
proxy->setFilterKeyColumn(-1);
proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
tableView->setModel(proxy);

// 搜索时：
connect(searchEdit, &QLineEdit::textChanged, proxy,
        &QSortFilterProxyModel::setFilterFixedString);
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】QSqlTableModel 修改后忘记 submitAll()

❌ 错误代码：
m_model->setData(index, "新值");
// 忘记 submitAll()，数据只在内存中，关闭程序后丢失

✅ 正确代码：
m_model->setData(index, "新值");
m_model->submitAll();  // 立即写入数据库

// 或设置自动提交模式：
m_model->setEditStrategy(QSqlTableModel::OnFieldChange);  // 每次字段变化立即提交

预防措施：明确选择提交策略，OnManualSubmit 需要手动调用 submitAll()。

────────────────────────────────────────────────────────────

【错误2】通过 proxy model 获取原始行索引时未转换

❌ 错误代码：
int row = tableView->currentIndex().row();
m_model->removeRow(row);  // 使用的是 proxy 的行号，不是 model 的行号！

✅ 正确代码：
QModelIndex proxyIndex = tableView->currentIndex();
QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
m_model->removeRow(sourceIndex.row());

预防措施：使用 proxy model 时，操作原始 model 前必须调用 mapToSource()。

────────────────────────────────────────────────────────────

【错误3】删除行后未刷新 model

❌ 错误代码：
m_model->removeRow(row);
// 界面没有立即更新，需要触发 select()

✅ 正确代码：
m_model->removeRow(row);
m_model->select();  // 重新从数据库查询，刷新界面

预防措施：增删改后调用 select() 或 submitAll() + select()。

────────────────────────────────────────────────────────────

【错误4】SQL 注入风险：直接拼接用户输入到 SQL 字符串

❌ 错误代码：
QString name = searchEdit->text();
q.exec("SELECT * FROM contacts WHERE name = '" + name + "'");
// 用户输入 "' OR '1'='1" 可绕过查询

✅ 正确代码：
QSqlQuery q;
q.prepare("SELECT * FROM contacts WHERE name LIKE :name");
q.bindValue(":name", "%" + searchEdit->text() + "%");
q.exec();

预防措施：用户输入必须用参数化查询，不直接拼接 SQL。

────────────────────────────────────────────────────────────

【错误5】QTableView 列宽未设置，默认列宽过窄

❌ 错误代码：
// 不设置列宽，默认所有列等宽，内容被截断

✅ 正确代码：
// 方式1：自动调整到内容宽度
tableView->resizeColumnsToContents();

// 方式2：指定列的伸缩模式
tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);        // 固定宽度
tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);      // 自动填充剩余空间
tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // 按内容

预防措施：setupUI 时总是设置合理的列宽策略。
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加排序功能
   - 点击列头时，按该列升序/降序排序
   - 提示：tableView->setSortingEnabled(true) + proxy->sort()

2. 导出为 CSV
   - 点击"导出"按钮，将当前显示的所有联系人导出为 CSV 文件
   - 用 QFileDialog::getSaveFileName() 让用户选择保存路径
   - 提示：遍历 m_model->rowCount()，用 QFile + QTextStream 写入

3. 批量导入
   - 选择一个 CSV 文件，解析每行并插入数据库
   - 跳过格式不正确的行，统计成功导入的数量
   - 提示：QTextStream::readLine() 逐行读取

4. 联系人详情对话框
   - 双击某行时，弹出自定义 QDialog 显示该联系人的完整信息
   - 对话框中可以编辑，点"保存"后更新数据库
   - 提示：连接 tableView->doubleClicked 信号
*/

#include "22_sql_practice.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ContactManager window;
    window.show();
    return app.exec();
}
