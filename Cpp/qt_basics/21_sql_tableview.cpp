/*
 * Qt 数据库：QSqlTableModel + QTableView
 * 日期：2026-06-05（第45天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. QSqlTableModel — 将数据库表绑定为数据模型
 * 2. QTableView — 将模型显示为可编辑表格
 * 3. 通过模型增删改查，自动同步到数据库
 * 4. 排序和过滤
 *
 * 编译前必须在 .pro 文件添加：QT += sql
 */

// ============================================
// 【核心概念】Model/View 架构
// ============================================
/*
Qt 的 Model/View 架构将数据和显示分离：

  数据库 ←→ QSqlTableModel（模型）←→ QTableView（视图）

优点：
- 模型负责数据，视图负责显示，互不干扰
- 同一个模型可以绑定多个视图
- 修改模型数据，所有视图自动更新

QSqlTableModel 的编辑策略（EditStrategy）：
  OnFieldChange  — 每次字段修改立即写入数据库（实时）
  OnRowChange    — 切换行时写入数据库
  OnManualSubmit — 手动调用 submitAll() 才写入（推荐，支持撤销）
*/

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QDateTime>

// 初始化数据库（复用上一个文件的逻辑）
bool setupDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("students.db");
    if (!db.open()) return false;

    QSqlQuery q;
    q.exec(
        "CREATE TABLE IF NOT EXISTS students ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name    TEXT    NOT NULL,"
        "  age     INTEGER NOT NULL DEFAULT 20,"
        "  score   REAL    DEFAULT 0.0,"
        "  created TEXT    DEFAULT ''"
        ")"
    );

    // 插入一些示例数据（如果表为空）
    q.exec("SELECT COUNT(*) FROM students");
    if (q.next() && q.value(0).toInt() == 0) {
        QStringList names = {"张三", "李四", "王五", "赵六", "钱七"};
        QList<int> ages   = {20, 22, 21, 23, 19};
        QList<double> scores = {85.5, 92.0, 78.5, 88.0, 95.5};
        QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd");

        for (int i = 0; i < names.size(); ++i) {
            q.prepare("INSERT INTO students (name, age, score, created) VALUES (:n,:a,:s,:c)");
            q.bindValue(":n", names[i]);
            q.bindValue(":a", ages[i]);
            q.bindValue(":s", scores[i]);
            q.bindValue(":c", now);
            q.exec();
        }
    }
    return true;
}

// ============================================
// 主界面
// ============================================
class TableModelDemo : public QWidget
{
    Q_OBJECT

public:
    TableModelDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("QSqlTableModel + QTableView 演示");
        resize(800, 550);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        // 标题
        QLabel *title = new QLabel("学生成绩管理 — QSqlTableModel + QTableView");
        title->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        // ----------------------------------------
        // 搜索栏
        // ----------------------------------------
        QHBoxLayout *searchLayout = new QHBoxLayout();
        searchLayout->addWidget(new QLabel("搜索姓名："));
        m_searchEdit = new QLineEdit();
        m_searchEdit->setPlaceholderText("输入姓名关键词过滤...");
        searchLayout->addWidget(m_searchEdit);
        searchLayout->addStretch();
        mainLayout->addLayout(searchLayout);

        // ----------------------------------------
        // QTableView — 表格视图
        // ----------------------------------------
        /*
        【函数卡片】QTableView

        作用：以表格形式显示数据模型
        setModel(model)：绑定数据模型
        horizontalHeader()：获取水平表头
        verticalHeader()：获取垂直表头
        setSelectionBehavior(SelectRows)：整行选中
        setEditTriggers(DoubleClicked)：双击编辑
        resizeColumnsToContents()：列宽自适应内容
        */
        m_tableView = new QTableView();
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setAlternatingRowColors(true);  // 交替行颜色
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
        m_tableView->setSortingEnabled(true);  // 允许点击表头排序
        m_tableView->setStyleSheet(R"(
            QTableView {
                border: 1px solid #dee2e6;
                border-radius: 4px;
                gridline-color: #e9ecef;
                font-size: 13px;
            }
            QTableView::item:selected {
                background: #cfe2ff;
                color: #000;
            }
            QHeaderView::section {
                background: #f8f9fa;
                border: none;
                border-bottom: 2px solid #dee2e6;
                padding: 6px;
                font-weight: bold;
            }
        )");
        mainLayout->addWidget(m_tableView);

        // ----------------------------------------
        // 创建 QSqlTableModel
        // ----------------------------------------
        /*
        【函数卡片】QSqlTableModel

        构造：QSqlTableModel(parent, db)
          parent：父对象
          db：数据库连接（默认使用默认连接）

        setTable(tableName)：指定数据库表名
        setEditStrategy(strategy)：设置编辑策略
        select()：从数据库加载数据
        setHeaderData(col, Qt::Horizontal, label)：设置列标题
        */
        m_model = new QSqlTableModel(this);
        m_model->setTable("students");
        m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        m_model->select();  // 加载数据

        // 设置列标题（更友好的中文名）
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "姓名");
        m_model->setHeaderData(2, Qt::Horizontal, "年龄");
        m_model->setHeaderData(3, Qt::Horizontal, "成绩");
        m_model->setHeaderData(4, Qt::Horizontal, "创建日期");

        // ----------------------------------------
        // 使用代理模型实现搜索过滤
        // ----------------------------------------
        /*
        QSortFilterProxyModel：
        - 包装原始模型，提供排序和过滤功能
        - 不修改原始数据，只改变显示
        - setFilterKeyColumn(col)：指定过滤的列
        - setFilterCaseSensitivity：大小写敏感性
        - setFilterRegularExpression(pattern)：设置过滤规则
        */
        m_proxyModel = new QSortFilterProxyModel(this);
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterKeyColumn(1);  // 按第1列（姓名）过滤
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        m_tableView->setModel(m_proxyModel);
        m_tableView->resizeColumnsToContents();
        m_tableView->setColumnWidth(1, 120);

        // ----------------------------------------
        // 操作按钮
        // ----------------------------------------
        QHBoxLayout *btnLayout = new QHBoxLayout();

        auto makeBtn = [](const QString &text, const QString &bg, const QString &fg = "white") {
            QPushButton *btn = new QPushButton(text);
            btn->setStyleSheet(
                QString("QPushButton{background:%1;color:%2;border:none;"
                        "border-radius:4px;padding:7px 14px;}"
                        "QPushButton:hover{opacity:0.85;}").arg(bg).arg(fg)
            );
            return btn;
        };

        QPushButton *addBtn    = makeBtn("➕ 添加行",   "#27ae60");
        QPushButton *deleteBtn = makeBtn("🗑 删除选中", "#e74c3c");
        QPushButton *submitBtn = makeBtn("💾 提交更改", "#3498db");
        QPushButton *revertBtn = makeBtn("↩ 撤销更改", "#95a5a6");
        QPushButton *refreshBtn= makeBtn("🔄 刷新",    "#6c757d");

        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(deleteBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(submitBtn);
        btnLayout->addWidget(revertBtn);
        btnLayout->addWidget(refreshBtn);
        mainLayout->addLayout(btnLayout);

        // 状态标签
        m_statusLabel = new QLabel("双击单元格可直接编辑 | 修改后点击"提交更改"保存到数据库");
        m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
        mainLayout->addWidget(m_statusLabel);

        // 连接信号
        connect(addBtn,    &QPushButton::clicked, this, &TableModelDemo::addRow);
        connect(deleteBtn, &QPushButton::clicked, this, &TableModelDemo::deleteRow);
        connect(submitBtn, &QPushButton::clicked, this, &TableModelDemo::submitChanges);
        connect(revertBtn, &QPushButton::clicked, this, &TableModelDemo::revertChanges);
        connect(refreshBtn,&QPushButton::clicked, this, &TableModelDemo::refreshData);

        // 搜索框输入时实时过滤
        connect(m_searchEdit, &QLineEdit::textChanged,
                m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

        updateStatus();
    }

private slots:

    void addRow()
    {
        // --------------------------------------------
        // 在模型末尾插入新行
        // --------------------------------------------
        /*
        insertRow(row)：在指定位置插入空行
        rowCount()：当前行数（插入到末尾）
        插入后，新行显示在表格中，用户可以双击编辑
        */
        int row = m_model->rowCount();
        m_model->insertRow(row);

        // 设置默认值
        QModelIndex nameIdx = m_model->index(row, 1);
        QModelIndex ageIdx  = m_model->index(row, 2);
        QModelIndex scoreIdx= m_model->index(row, 3);
        QModelIndex dateIdx = m_model->index(row, 4);

        m_model->setData(nameIdx,  "新学生");
        m_model->setData(ageIdx,   20);
        m_model->setData(scoreIdx, 0.0);
        m_model->setData(dateIdx,  QDateTime::currentDateTime().toString("yyyy-MM-dd"));

        // 滚动到新行并选中
        m_tableView->scrollToBottom();
        m_tableView->selectRow(row);

        m_statusLabel->setText("已添加新行，双击编辑后点击"提交更改"保存");
    }

    void deleteRow()
    {
        // --------------------------------------------
        // 删除选中行
        // --------------------------------------------
        QModelIndexList selected = m_tableView->selectionModel()->selectedRows();
        if (selected.isEmpty()) {
            QMessageBox::information(this, "提示", "请先选中要删除的行！");
            return;
        }

        auto reply = QMessageBox::question(this, "确认删除",
            QString("确定要删除选中的 %1 行吗？").arg(selected.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        // 从后往前删除，避免索引偏移
        QList<int> rows;
        for (const QModelIndex &idx : selected) {
            // 代理模型索引需要映射回源模型索引
            rows.append(m_proxyModel->mapToSource(idx).row());
        }
        std::sort(rows.begin(), rows.end(), std::greater<int>());
        for (int row : rows) {
            m_model->removeRow(row);
        }

        m_statusLabel->setText(QString("已标记 %1 行待删除，点击"提交更改"确认").arg(rows.size()));
    }

    void submitChanges()
    {
        // --------------------------------------------
        // 提交所有未保存的更改到数据库
        // --------------------------------------------
        /*
        submitAll()：将所有挂起的更改写入数据库
        返回 true = 成功，false = 失败（可能有约束冲突）
        */
        if (m_model->submitAll()) {
            m_statusLabel->setText("✅ 更改已保存到数据库");
            updateStatus();
        } else {
            QMessageBox::warning(this, "保存失败",
                "保存失败：" + m_model->lastError().text());
            m_statusLabel->setText("❌ 保存失败：" + m_model->lastError().text());
        }
    }

    void revertChanges()
    {
        // --------------------------------------------
        // 撤销所有未提交的更改
        // --------------------------------------------
        /*
        revertAll()：丢弃所有未提交的更改，恢复到数据库中的状态
        */
        m_model->revertAll();
        m_statusLabel->setText("↩ 已撤销所有未保存的更改");
    }

    void refreshData()
    {
        m_model->select();
        m_statusLabel->setText("🔄 数据已刷新");
        updateStatus();
    }

private:
    void updateStatus()
    {
        int total = m_model->rowCount();
        m_statusLabel->setText(
            QString("共 %1 条记录 | 双击单元格编辑 | 修改后点击"提交更改"保存").arg(total)
        );
    }

    QTableView              *m_tableView;
    QSqlTableModel          *m_model;
    QSortFilterProxyModel   *m_proxyModel;
    QLineEdit               *m_searchEdit;
    QLabel                  *m_statusLabel;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：忘记调用 select() 加载数据
    m_model->setTable("students");
    // 没有 select()，表格是空的！

✅ 正确：
    m_model->setTable("students");
    m_model->select();

❌ 错误2：OnFieldChange 策略下无法撤销
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    // 每次修改立即写入数据库，无法撤销

✅ 正确：用 OnManualSubmit，支持 revertAll() 撤销
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

❌ 错误3：删除行时索引偏移
    for (auto &idx : selected) {
        m_model->removeRow(idx.row());  // 删除第一行后，后续行索引变了！
    }

✅ 正确：从后往前删除
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) m_model->removeRow(row);

❌ 错误4：代理模型索引没有映射回源模型
    QModelIndex idx = m_tableView->currentIndex();
    m_model->removeRow(idx.row());  // idx 是代理模型的索引，不是源模型的！

✅ 正确：
    QModelIndex sourceIdx = m_proxyModel->mapToSource(idx);
    m_model->removeRow(sourceIdx.row());

❌ 错误5：submitAll() 失败后没有处理错误
    m_model->submitAll();  // 失败了也不知道

✅ 正确：
    if (!m_model->submitAll()) {
        QMessageBox::warning(this, "错误", m_model->lastError().text());
    }
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加"按成绩排序"按钮：
   - 点击后按成绩从高到低排序
   - 提示：m_model->setSort(3, Qt::DescendingOrder); m_model->select();

2. 添加"成绩统计"标签：
   - 在表格下方显示：最高分、最低分、平均分
   - 每次数据变化时自动更新
   - 提示：遍历模型所有行，或用 QSqlQuery 执行 SELECT AVG(score)

3. 添加"导出 CSV"功能：
   - 将当前表格数据导出为 CSV 文件
   - 提示：遍历模型所有行列，用 QFile + QTextStream 写入
   - 格式：id,name,age,score,created（第一行为表头）
*/

#include "21_sql_tableview.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!setupDatabase()) {
        QMessageBox::critical(nullptr, "错误", "无法连接数据库！");
        return 1;
    }

    TableModelDemo window;
    window.show();
    return app.exec();
}
