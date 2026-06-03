/*
 * Qt 数据库：QSqlRelationalTableModel + 带分组的联系人管理
 * 日期：2026-06-06（第46天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. QSqlRelationalTableModel — 外键关联自动显示
 * 2. QSqlRelationalDelegate — 外键列显示为下拉框
 * 3. 带分组功能的联系人管理（左侧分组列表 + 右侧联系人列表）
 *
 * 编译前必须在 .pro 文件添加：QT += sql
 */

// ============================================
// 【核心概念】QSqlRelationalTableModel
// ============================================
/*
问题：普通 QSqlTableModel 显示外键时只显示数字 ID，不友好。
  contacts 表中 group_id=1，用户看到的是 "1"，不知道是哪个分组。

解决：QSqlRelationalTableModel 自动将外键 ID 替换为关联表的文字。
  设置关联后，group_id 列自动显示 "家人"、"同事" 等分组名称。

配合 QSqlRelationalDelegate：
  外键列在编辑时自动显示为下拉框，列出所有可选的关联值。
*/

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QDialog>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QTableView>
#include <QHeaderView>
#include <QStatusBar>
#include <QSqlDatabase>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QInputDialog>

// ============================================
// 初始化数据库
// ============================================
bool setupGroupedDb()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("grouped_contacts.db");
    if (!db.open()) return false;

    QSqlQuery q;

    // 分组表
    q.exec(
        "CREATE TABLE IF NOT EXISTS groups ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE"
        ")"
    );

    // 联系人表（group_id 外键关联 groups.id）
    q.exec(
        "CREATE TABLE IF NOT EXISTS contacts ("
        "  id       INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name     TEXT NOT NULL,"
        "  phone    TEXT DEFAULT '',"
        "  email    TEXT DEFAULT '',"
        "  group_id INTEGER NOT NULL DEFAULT 1,"
        "  FOREIGN KEY (group_id) REFERENCES groups(id)"
        ")"
    );

    // 插入默认分组
    q.exec("SELECT COUNT(*) FROM groups");
    if (q.next() && q.value(0).toInt() == 0) {
        q.exec("INSERT INTO groups (name) VALUES ('家人')");
        q.exec("INSERT INTO groups (name) VALUES ('朋友')");
        q.exec("INSERT INTO groups (name) VALUES ('同事')");
        q.exec("INSERT INTO groups (name) VALUES ('其他')");

        // 插入示例联系人
        QList<QVariantList> contacts = {
            {"张爸爸", "13800000001", "dad@home.com",    1},
            {"张妈妈", "13800000002", "mom@home.com",    1},
            {"李明",   "13900000001", "liming@qq.com",   2},
            {"王芳",   "13900000002", "wangfang@qq.com", 2},
            {"陈经理", "13700000001", "chen@work.com",   3},
            {"刘同事", "13700000002", "liu@work.com",    3},
        };
        q.prepare("INSERT INTO contacts (name, phone, email, group_id) "
                  "VALUES (:name, :phone, :email, :gid)");
        for (auto &c : contacts) {
            q.bindValue(":name",  c[0]);
            q.bindValue(":phone", c[1]);
            q.bindValue(":email", c[2]);
            q.bindValue(":gid",   c[3]);
            q.exec();
        }
    }
    return true;
}

// ============================================
// 添加联系人对话框
// ============================================
class AddContactDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddContactDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("添加联系人");
        setMinimumWidth(300);

        QFormLayout *form = new QFormLayout();
        m_nameEdit  = new QLineEdit();
        m_phoneEdit = new QLineEdit();
        m_emailEdit = new QLineEdit();
        m_groupCombo = new QComboBox();

        // 从数据库加载分组列表
        QSqlQuery q("SELECT id, name FROM groups ORDER BY id");
        while (q.next()) {
            m_groupCombo->addItem(q.value(1).toString(), q.value(0).toInt());
        }

        form->addRow("姓名 *：",  m_nameEdit);
        form->addRow("电话：",    m_phoneEdit);
        form->addRow("邮箱：",    m_emailEdit);
        form->addRow("分组：",    m_groupCombo);

        QDialogButtonBox *box = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(box, &QDialogButtonBox::accepted, this, &AddContactDialog::onAccept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addLayout(form);
        layout->addWidget(box);
    }

    QString name()    const { return m_nameEdit->text().trimmed(); }
    QString phone()   const { return m_phoneEdit->text().trimmed(); }
    QString email()   const { return m_emailEdit->text().trimmed(); }
    int     groupId() const { return m_groupCombo->currentData().toInt(); }

private slots:
    void onAccept() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "提示", "姓名不能为空！");
            return;
        }
        accept();
    }

private:
    QLineEdit *m_nameEdit, *m_phoneEdit, *m_emailEdit;
    QComboBox *m_groupCombo;
};

// ============================================
// 带分组的联系人管理主窗口
// ============================================
class GroupedContactManager : public QMainWindow
{
    Q_OBJECT

public:
    GroupedContactManager(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("联系人管理（带分组）");
        resize(850, 550);

        QWidget *central = new QWidget();
        QVBoxLayout *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);
        setCentralWidget(central);

        // 标题
        QLabel *title = new QLabel("📋 联系人管理（QSqlRelationalTableModel 演示）");
        title->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        // 分割器：左侧分组列表 + 右侧联系人表格
        QSplitter *splitter = new QSplitter(Qt::Horizontal);
        mainLayout->addWidget(splitter);

        // ----------------------------------------
        // 左侧：分组列表
        // ----------------------------------------
        QWidget *leftWidget = new QWidget();
        leftWidget->setMaximumWidth(180);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 8, 0);

        QLabel *groupTitle = new QLabel("分组");
        groupTitle->setStyleSheet("font-weight: bold; color: #495057;");
        leftLayout->addWidget(groupTitle);

        m_groupList = new QListWidget();
        m_groupList->setStyleSheet(
            "QListWidget{border:1px solid #dee2e6;border-radius:4px;}"
            "QListWidget::item{padding:8px;}"
            "QListWidget::item:selected{background:#cfe2ff;color:#000;}"
        );
        leftLayout->addWidget(m_groupList);

        QHBoxLayout *groupBtnLayout = new QHBoxLayout();
        QPushButton *addGroupBtn = new QPushButton("+ 分组");
        QPushButton *delGroupBtn = new QPushButton("- 分组");
        addGroupBtn->setStyleSheet("QPushButton{background:#27ae60;color:white;border:none;border-radius:3px;padding:4px 8px;font-size:12px;}");
        delGroupBtn->setStyleSheet("QPushButton{background:#e74c3c;color:white;border:none;border-radius:3px;padding:4px 8px;font-size:12px;}");
        groupBtnLayout->addWidget(addGroupBtn);
        groupBtnLayout->addWidget(delGroupBtn);
        leftLayout->addLayout(groupBtnLayout);

        splitter->addWidget(leftWidget);

        // ----------------------------------------
        // 右侧：联系人表格（QSqlRelationalTableModel）
        // ----------------------------------------
        QWidget *rightWidget = new QWidget();
        QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(0, 0, 0, 0);

        // 搜索栏
        QHBoxLayout *searchLayout = new QHBoxLayout();
        m_searchEdit = new QLineEdit();
        m_searchEdit->setPlaceholderText("🔍 搜索联系人...");
        searchLayout->addWidget(m_searchEdit);
        rightLayout->addLayout(searchLayout);

        // 表格视图
        m_tableView = new QTableView();
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setAlternatingRowColors(true);
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->verticalHeader()->setVisible(false);
        m_tableView->setStyleSheet(
            "QTableView{border:1px solid #dee2e6;border-radius:4px;font-size:13px;}"
            "QTableView::item{padding:5px;}"
            "QTableView::item:selected{background:#cfe2ff;color:#000;}"
            "QHeaderView::section{background:#f8f9fa;border:none;border-bottom:2px solid #dee2e6;padding:6px;font-weight:bold;}"
        );
        rightLayout->addWidget(m_tableView);

        // 操作按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *addBtn = new QPushButton("➕ 添加联系人");
        QPushButton *delBtn = new QPushButton("🗑 删除选中");
        QPushButton *saveBtn = new QPushButton("💾 保存更改");
        addBtn->setStyleSheet("QPushButton{background:#27ae60;color:white;border:none;border-radius:4px;padding:7px 14px;}");
        delBtn->setStyleSheet("QPushButton{background:#e74c3c;color:white;border:none;border-radius:4px;padding:7px 14px;}");
        saveBtn->setStyleSheet("QPushButton{background:#3498db;color:white;border:none;border-radius:4px;padding:7px 14px;}");
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(delBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(saveBtn);
        rightLayout->addLayout(btnLayout);

        splitter->addWidget(rightWidget);
        splitter->setSizes({160, 640});

        // ----------------------------------------
        // 创建 QSqlRelationalTableModel
        // ----------------------------------------
        /*
        【函数卡片】QSqlRelationalTableModel

        继承自 QSqlTableModel，额外支持外键关联显示。

        setRelation(column, QSqlRelation(tableName, indexColumn, displayColumn))
          column：要设置关联的列索引
          tableName：关联的外表名
          indexColumn：外表中作为主键/索引的列名
          displayColumn：外表中要显示的列名

        效果：
          原来显示 group_id=1，设置关联后显示 "家人"
        */
        m_model = new QSqlRelationalTableModel(this);
        m_model->setTable("contacts");
        m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

        // 设置 group_id 列（第4列，索引从0）关联到 groups 表
        // 显示 groups.name，而不是 groups.id
        m_model->setRelation(4, QSqlRelation("groups", "id", "name"));
        m_model->select();

        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "姓名");
        m_model->setHeaderData(2, Qt::Horizontal, "电话");
        m_model->setHeaderData(3, Qt::Horizontal, "邮箱");
        m_model->setHeaderData(4, Qt::Horizontal, "分组");

        // 代理模型（搜索过滤）
        m_proxy = new QSortFilterProxyModel(this);
        m_proxy->setSourceModel(m_model);
        m_proxy->setFilterKeyColumn(-1);
        m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxy);
        m_tableView->setColumnHidden(0, true);  // 隐藏 ID 列

        // ⭐ 关键：使用 QSqlRelationalDelegate
        // 让外键列（分组）在编辑时显示为下拉框
        m_tableView->setItemDelegate(new QSqlRelationalDelegate(m_tableView));

        // 加载分组列表
        loadGroups();

        // 连接信号
        connect(addBtn,  &QPushButton::clicked, this, &GroupedContactManager::addContact);
        connect(delBtn,  &QPushButton::clicked, this, &GroupedContactManager::deleteContact);
        connect(saveBtn, &QPushButton::clicked, this, &GroupedContactManager::saveChanges);
        connect(addGroupBtn, &QPushButton::clicked, this, &GroupedContactManager::addGroup);
        connect(delGroupBtn, &QPushButton::clicked, this, &GroupedContactManager::deleteGroup);
        connect(m_groupList, &QListWidget::currentRowChanged,
                this, &GroupedContactManager::onGroupSelected);
        connect(m_searchEdit, &QLineEdit::textChanged,
                m_proxy, &QSortFilterProxyModel::setFilterFixedString);

        updateStatusBar();
    }

private slots:

    void loadGroups()
    {
        m_groupList->clear();
        m_groupList->addItem("📋 全部联系人");

        QSqlQuery q("SELECT id, name FROM groups ORDER BY id");
        while (q.next()) {
            QListWidgetItem *item = new QListWidgetItem(q.value(1).toString());
            item->setData(Qt::UserRole, q.value(0).toInt());  // 存储 group_id
            m_groupList->addItem(item);
        }
        m_groupList->setCurrentRow(0);
    }

    void onGroupSelected(int row)
    {
        if (row <= 0) {
            // 显示全部
            m_model->setFilter("");
        } else {
            QListWidgetItem *item = m_groupList->item(row);
            int groupId = item->data(Qt::UserRole).toInt();
            m_model->setFilter(QString("group_id = %1").arg(groupId));
        }
        m_model->select();
        updateStatusBar();
    }

    void addContact()
    {
        AddContactDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted) return;

        QSqlQuery q;
        q.prepare("INSERT INTO contacts (name, phone, email, group_id) "
                  "VALUES (:name, :phone, :email, :gid)");
        q.bindValue(":name",  dlg.name());
        q.bindValue(":phone", dlg.phone());
        q.bindValue(":email", dlg.email());
        q.bindValue(":gid",   dlg.groupId());

        if (q.exec()) {
            m_model->select();
            updateStatusBar();
            statusBar()->showMessage("✅ 已添加：" + dlg.name(), 3000);
        } else {
            QMessageBox::critical(this, "错误", q.lastError().text());
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

        if (QMessageBox::question(this, "确认", QString("删除"%1"？").arg(name),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        QSqlQuery q;
        q.prepare("DELETE FROM contacts WHERE id=:id");
        q.bindValue(":id", id);
        if (q.exec()) {
            m_model->select();
            updateStatusBar();
            statusBar()->showMessage("🗑 已删除：" + name, 3000);
        }
    }

    void saveChanges()
    {
        if (m_model->submitAll()) {
            statusBar()->showMessage("✅ 更改已保存", 3000);
        } else {
            QMessageBox::warning(this, "错误", m_model->lastError().text());
        }
    }

    void addGroup()
    {
        bool ok;
        QString name = QInputDialog::getText(this, "添加分组", "分组名称：",
                                             QLineEdit::Normal, "", &ok);
        if (!ok || name.trimmed().isEmpty()) return;

        QSqlQuery q;
        q.prepare("INSERT INTO groups (name) VALUES (:name)");
        q.bindValue(":name", name.trimmed());
        if (q.exec()) {
            loadGroups();
            statusBar()->showMessage("✅ 已添加分组：" + name, 3000);
        } else {
            QMessageBox::warning(this, "错误", "分组名称已存在！");
        }
    }

    void deleteGroup()
    {
        QListWidgetItem *item = m_groupList->currentItem();
        if (!item || !item->data(Qt::UserRole).isValid()) {
            QMessageBox::information(this, "提示", "请选中要删除的分组！");
            return;
        }
        int groupId = item->data(Qt::UserRole).toInt();
        QString groupName = item->text();

        // 检查分组下是否有联系人
        QSqlQuery check;
        check.prepare("SELECT COUNT(*) FROM contacts WHERE group_id=:id");
        check.bindValue(":id", groupId);
        check.exec();
        int count = check.next() ? check.value(0).toInt() : 0;

        if (count > 0) {
            QMessageBox::warning(this, "无法删除",
                QString("分组"%1"下还有 %2 位联系人，请先移动或删除这些联系人！")
                    .arg(groupName).arg(count));
            return;
        }

        if (QMessageBox::question(this, "确认", QString("删除分组"%1"？").arg(groupName),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;

        QSqlQuery q;
        q.prepare("DELETE FROM groups WHERE id=:id");
        q.bindValue(":id", groupId);
        if (q.exec()) {
            loadGroups();
            m_model->select();
            statusBar()->showMessage("🗑 已删除分组：" + groupName, 3000);
        }
    }

private:
    void updateStatusBar()
    {
        int shown = m_model->rowCount();
        QSqlQuery q("SELECT COUNT(*) FROM contacts");
        int total = q.next() ? q.value(0).toInt() : 0;
        statusBar()->showMessage(
            QString("显示 %1 位 / 共 %2 位联系人 | 双击单元格可直接编辑分组").arg(shown).arg(total)
        );
    }

    QListWidget               *m_groupList;
    QTableView                *m_tableView;
    QSqlRelationalTableModel  *m_model;
    QSortFilterProxyModel     *m_proxy;
    QLineEdit                 *m_searchEdit;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】QSqlRelationalTableModel 外键列直接 setData 写 ID，不写名称

❌ 错误代码：
// 外键列应该显示关联表的名称，但直接 setData 写了 ID 数字
model->setData(model->index(row, 2), 3);  // 写入 group_id=3
// 界面上该列还是显示数字，不是分组名

✅ 正确代码：
// 外键列必须通过 QSqlRelationalDelegate 来编辑，UI 自动显示关联名称
tableView->setItemDelegate(new QSqlRelationalDelegate(tableView));
// 设置关联：
model->setRelation(2, QSqlRelation("groups", "id", "name"));

预防措施：使用 QSqlRelationalTableModel 时，必须搭配 QSqlRelationalDelegate。

────────────────────────────────────────────────────────────

【错误2】关联表数据改变后，relational model 未刷新

❌ 错误代码：
// 添加新分组后，联系人的分组下拉框没有更新
groupModel->insertRow(...);
groupModel->submitAll();
// contactModel 的关联数据不会自动更新

✅ 正确代码：
groupModel->submitAll();
contactModel->select();  // 重新查询，关联数据自动刷新

预防措施：修改关联表后，主表 model 需要重新 select()。

────────────────────────────────────────────────────────────

【错误3】DELETE 时违反外键约束导致失败

❌ 错误代码：
// 删除分组时，分组下还有联系人
q.exec("DELETE FROM groups WHERE id = 1");
// 如果启用了外键约束，会失败；否则联系人的 group_id 变成悬空值

✅ 正确代码：
// 方式1：先检查是否有关联联系人
QSqlQuery check;
check.prepare("SELECT COUNT(*) FROM contacts WHERE group_id = :id");
check.bindValue(":id", groupId);
check.exec();
if (check.next() && check.value(0).toInt() > 0) {
    QMessageBox::warning(this, "无法删除", "该分组下还有联系人，请先移除");
    return;
}

// 方式2：级联删除（在建表时指定 ON DELETE CASCADE）

预防措施：删除父表记录前，先检查子表是否有关联数据。

────────────────────────────────────────────────────────────

【错误4】JOIN 查询结果列名冲突

❌ 错误代码：
q.exec("SELECT contacts.id, groups.id FROM contacts JOIN groups ON ...");
// 两个 id 列名相同，q.value("id") 取到哪个？未定义

✅ 正确代码：
q.exec("SELECT contacts.id AS contact_id, groups.id AS group_id, "
       "contacts.name, groups.name AS group_name FROM contacts JOIN groups ON ...");
// 用 AS 为冲突列取别名

预防措施：JOIN 查询中有同名列时，必须用 AS 取别名。

────────────────────────────────────────────────────────────

【错误5】QSortFilterProxyModel 过滤后行号偏移，删除错误行

❌ 错误代码：
// 同 22_sql_practice 的错误2：proxy 行号 ≠ source 行号
int row = tableView->currentIndex().row();
m_model->removeRow(row);  // 删除了错误的行！

✅ 正确代码：
auto sourceIdx = m_proxy->mapToSource(tableView->currentIndex());
m_model->removeRow(sourceIdx.row());
m_model->select();

预防措施：有 proxy model 时，所有对 source model 的行操作都需要 mapToSource()。
*/

// ============================================
// 练习题
// ============================================
/*
1. 分组重命名
   - 双击分组列表中的某项，允许用户修改分组名
   - 修改后同步更新 groups 表和联系人显示
   - 提示：QListWidget::itemDoubleClicked 信号 + QInputDialog::getText()

2. 联系人移动分组
   - 选中一个联系人，右键菜单"移动到分组"
   - 弹出分组列表，选择后更新该联系人的 group_id
   - 提示：QMenu + QAction，更新后 m_model->select()

3. 分组统计视图
   - 在分组列表旁显示每个分组的联系人数量（如"朋友 (5)"）
   - 提示：GROUP BY 查询统计各分组人数，刷新 QListWidget

4. 数据导出增强
   - 导出时按分组组织 CSV 文件：每个分组一个 Section
   - 格式：
     # 朋友
     张三,13800138000
     李四,13900139000
     # 同事
     王五,13700137000
*/

#include "24_25_sql_relational.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (!setupGroupedDb()) {
        QMessageBox::critical(nullptr, "错误", "无法连接数据库！");
        return 1;
    }
    GroupedContactManager window;
    window.show();
    return app.exec();
}
