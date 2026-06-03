/*
 * Qt 数据库进阶：事务、关联查询、聚合函数
 * 日期：2026-06-06（第46天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. 事务（transaction / commit / rollback）
 * 2. JOIN 关联查询（INNER JOIN、LEFT JOIN）
 * 3. 聚合函数（COUNT、SUM、AVG、MAX、MIN）
 * 4. GROUP BY 分组统计
 * 5. QSqlQueryModel（只读查询结果模型）
 *
 * 编译前必须在 .pro 文件添加：QT += sql
 */

// ============================================
// 【核心概念】数据库事务
// ============================================
/*
事务（Transaction）的四个特性（ACID）：
  A - 原子性：事务内所有操作要么全部成功，要么全部回滚
  C - 一致性：事务前后数据库状态保持一致
  I - 隔离性：并发事务互不干扰
  D - 持久性：提交后数据永久保存

为什么需要事务？
  场景：银行转账，A 账户 -100，B 账户 +100
  如果 A 扣款成功但 B 加款失败，数据就不一致了！
  用事务包裹，失败时回滚，保证两个操作要么都成功，要么都不执行。

Qt 中的事务：
  db.transaction()  — 开始事务
  db.commit()       — 提交（所有操作生效）
  db.rollback()     — 回滚（所有操作撤销）
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTableView>
#include <QHeaderView>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QMessageBox>
#include <QTabWidget>
#include <QDateTime>

// ============================================
// 初始化演示数据库（两张表：departments + employees）
// ============================================
bool setupAdvancedDb()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("company.db");
    if (!db.open()) return false;

    QSqlQuery q;

    // 部门表
    q.exec(
        "CREATE TABLE IF NOT EXISTS departments ("
        "  id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  city TEXT NOT NULL"
        ")"
    );

    // 员工表（dept_id 是外键，关联 departments.id）
    q.exec(
        "CREATE TABLE IF NOT EXISTS employees ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name    TEXT    NOT NULL,"
        "  dept_id INTEGER NOT NULL,"
        "  salary  REAL    NOT NULL,"
        "  hire_date TEXT  NOT NULL,"
        "  FOREIGN KEY (dept_id) REFERENCES departments(id)"
        ")"
    );

    // 插入示例数据（如果表为空）
    q.exec("SELECT COUNT(*) FROM departments");
    if (q.next() && q.value(0).toInt() == 0) {
        // 插入部门
        q.exec("INSERT INTO departments (name, city) VALUES ('研发部', '北京')");
        q.exec("INSERT INTO departments (name, city) VALUES ('市场部', '上海')");
        q.exec("INSERT INTO departments (name, city) VALUES ('财务部', '广州')");

        // 插入员工（使用事务批量插入）
        QList<QVariantList> employees = {
            {"张三", 1, 15000, "2022-01-15"},
            {"李四", 1, 18000, "2021-06-01"},
            {"王五", 2, 12000, "2023-03-20"},
            {"赵六", 2, 13500, "2022-09-10"},
            {"钱七", 3, 11000, "2023-01-05"},
            {"孙八", 1, 20000, "2020-11-30"},
            {"周九", 3, 10500, "2023-07-15"},
        };

        db.transaction();
        q.prepare("INSERT INTO employees (name, dept_id, salary, hire_date) "
                  "VALUES (:name, :dept, :salary, :date)");
        for (auto &e : employees) {
            q.bindValue(":name",   e[0]);
            q.bindValue(":dept",   e[1]);
            q.bindValue(":salary", e[2]);
            q.bindValue(":date",   e[3]);
            q.exec();
        }
        db.commit();
    }
    return true;
}

// ============================================
// 主界面
// ============================================
class SqlAdvancedDemo : public QWidget
{
    Q_OBJECT

public:
    SqlAdvancedDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt SQL 进阶演示");
        resize(800, 600);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(10);

        QLabel *title = new QLabel("Qt SQL 进阶 — 事务、JOIN、聚合函数");
        title->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        // 标签页
        QTabWidget *tabs = new QTabWidget();
        mainLayout->addWidget(tabs);

        // ----------------------------------------
        // Tab1：事务演示
        // ----------------------------------------
        QWidget *txTab = new QWidget();
        QVBoxLayout *txLayout = new QVBoxLayout(txTab);

        QLabel *txDesc = new QLabel(
            "事务演示：批量插入员工数据\n"
            "• 成功场景：所有插入成功 → commit()\n"
            "• 失败场景：模拟中途失败 → rollback()，所有插入撤销"
        );
        txDesc->setStyleSheet("color: #555; background: #f8f9fa; padding: 8px; border-radius: 4px;");
        txDesc->setWordWrap(true);
        txLayout->addWidget(txDesc);

        QHBoxLayout *txBtnLayout = new QHBoxLayout();
        QPushButton *txSuccessBtn = new QPushButton("✅ 事务成功演示（批量插入3条）");
        QPushButton *txFailBtn    = new QPushButton("❌ 事务失败演示（中途回滚）");
        txSuccessBtn->setStyleSheet("QPushButton{background:#27ae60;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#219a52;}");
        txFailBtn->setStyleSheet("QPushButton{background:#e74c3c;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#c0392b;}");
        txBtnLayout->addWidget(txSuccessBtn);
        txBtnLayout->addWidget(txFailBtn);
        txBtnLayout->addStretch();
        txLayout->addLayout(txBtnLayout);

        m_txLog = new QTextEdit();
        m_txLog->setReadOnly(true);
        m_txLog->setFont(QFont("Consolas", 11));
        m_txLog->setStyleSheet("background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:8px;");
        txLayout->addWidget(m_txLog);

        connect(txSuccessBtn, &QPushButton::clicked, this, &SqlAdvancedDemo::demoTransactionSuccess);
        connect(txFailBtn,    &QPushButton::clicked, this, &SqlAdvancedDemo::demoTransactionFail);
        tabs->addTab(txTab, "事务（Transaction）");

        // ----------------------------------------
        // Tab2：JOIN 关联查询
        // ----------------------------------------
        QWidget *joinTab = new QWidget();
        QVBoxLayout *joinLayout = new QVBoxLayout(joinTab);

        QHBoxLayout *joinBtnLayout = new QHBoxLayout();
        QPushButton *innerJoinBtn = new QPushButton("INNER JOIN（员工+部门）");
        QPushButton *leftJoinBtn  = new QPushButton("LEFT JOIN（含无部门员工）");
        QPushButton *groupByBtn   = new QPushButton("GROUP BY（各部门统计）");
        innerJoinBtn->setStyleSheet("QPushButton{background:#3498db;color:white;border:none;border-radius:4px;padding:8px 12px;}QPushButton:hover{background:#2980b9;}");
        leftJoinBtn->setStyleSheet("QPushButton{background:#9b59b6;color:white;border:none;border-radius:4px;padding:8px 12px;}QPushButton:hover{background:#8e44ad;}");
        groupByBtn->setStyleSheet("QPushButton{background:#e67e22;color:white;border:none;border-radius:4px;padding:8px 12px;}QPushButton:hover{background:#d35400;}");
        joinBtnLayout->addWidget(innerJoinBtn);
        joinBtnLayout->addWidget(leftJoinBtn);
        joinBtnLayout->addWidget(groupByBtn);
        joinBtnLayout->addStretch();
        joinLayout->addLayout(joinBtnLayout);

        m_joinView = new QTableView();
        m_joinView->setAlternatingRowColors(true);
        m_joinView->horizontalHeader()->setStretchLastSection(true);
        m_joinView->setStyleSheet(
            "QTableView{border:1px solid #dee2e6;border-radius:4px;font-size:13px;}"
            "QHeaderView::section{background:#f8f9fa;border:none;border-bottom:2px solid #dee2e6;padding:6px;font-weight:bold;}"
        );

        m_joinModel = new QSqlQueryModel(this);
        m_joinView->setModel(m_joinModel);
        joinLayout->addWidget(m_joinView);

        connect(innerJoinBtn, &QPushButton::clicked, this, &SqlAdvancedDemo::doInnerJoin);
        connect(leftJoinBtn,  &QPushButton::clicked, this, &SqlAdvancedDemo::doLeftJoin);
        connect(groupByBtn,   &QPushButton::clicked, this, &SqlAdvancedDemo::doGroupBy);
        tabs->addTab(joinTab, "JOIN 关联查询");

        // 默认显示 INNER JOIN
        doInnerJoin();
    }

private slots:

    // ============================================
    // 事务成功演示
    // ============================================
    void demoTransactionSuccess()
    {
        m_txLog->clear();
        txLog("=== 事务成功演示 ===");
        txLog("开始事务...");

        QSqlDatabase db = QSqlDatabase::database();

        // --------------------------------------------
        // db.transaction() — 开始事务
        // --------------------------------------------
        db.transaction();

        QSqlQuery q;
        QList<QVariantList> newEmployees = {
            {"测试员工A", 1, 9000, "2024-01-01"},
            {"测试员工B", 2, 9500, "2024-01-02"},
            {"测试员工C", 3, 8800, "2024-01-03"},
        };

        bool allOk = true;
        for (auto &e : newEmployees) {
            q.prepare("INSERT INTO employees (name, dept_id, salary, hire_date) "
                      "VALUES (:name, :dept, :salary, :date)");
            q.bindValue(":name",   e[0]);
            q.bindValue(":dept",   e[1]);
            q.bindValue(":salary", e[2]);
            q.bindValue(":date",   e[3]);

            if (q.exec()) {
                txLog(QString("  ✅ 插入成功：%1（id=%2）")
                    .arg(e[0].toString()).arg(q.lastInsertId().toInt()));
            } else {
                txLog("  ❌ 插入失败：" + q.lastError().text());
                allOk = false;
                break;
            }
        }

        if (allOk) {
            // --------------------------------------------
            // db.commit() — 提交事务，所有操作生效
            // --------------------------------------------
            db.commit();
            txLog("✅ 事务提交成功！3条记录已写入数据库");
        } else {
            // --------------------------------------------
            // db.rollback() — 回滚事务，所有操作撤销
            // --------------------------------------------
            db.rollback();
            txLog("❌ 事务回滚！所有插入操作已撤销");
        }

        // 显示当前员工总数
        QSqlQuery countQ;
        countQ.exec("SELECT COUNT(*) FROM employees");
        if (countQ.next())
            txLog(QString("当前员工总数：%1").arg(countQ.value(0).toInt()));
    }

    // ============================================
    // 事务失败演示（模拟中途失败）
    // ============================================
    void demoTransactionFail()
    {
        m_txLog->clear();
        txLog("=== 事务失败演示（模拟中途失败）===");

        // 先记录当前员工数
        QSqlQuery countQ;
        countQ.exec("SELECT COUNT(*) FROM employees");
        int beforeCount = countQ.next() ? countQ.value(0).toInt() : 0;
        txLog(QString("事务前员工数：%1").arg(beforeCount));

        QSqlDatabase db = QSqlDatabase::database();
        db.transaction();
        txLog("开始事务...");

        QSqlQuery q;

        // 第1条：正常插入
        q.prepare("INSERT INTO employees (name, dept_id, salary, hire_date) "
                  "VALUES (:name, :dept, :salary, :date)");
        q.bindValue(":name", "临时员工X");
        q.bindValue(":dept", 1);
        q.bindValue(":salary", 5000);
        q.bindValue(":date", "2024-06-01");
        q.exec();
        txLog("  ✅ 第1条插入成功（临时员工X）");

        // 第2条：模拟失败（dept_id=999 不存在，但 SQLite 默认不强制外键）
        // 改用插入 NULL name 触发 NOT NULL 约束失败
        q.prepare("INSERT INTO employees (name, dept_id, salary, hire_date) "
                  "VALUES (NULL, :dept, :salary, :date)");  // name=NULL 违反 NOT NULL
        q.bindValue(":dept", 1);
        q.bindValue(":salary", 5000);
        q.bindValue(":date", "2024-06-01");

        if (!q.exec()) {
            txLog("  ❌ 第2条插入失败（name 为 NULL，违反约束）：" + q.lastError().text());
            db.rollback();
            txLog("❌ 事务回滚！第1条插入也被撤销");
        } else {
            db.commit();
        }

        // 验证回滚效果
        countQ.exec("SELECT COUNT(*) FROM employees");
        int afterCount = countQ.next() ? countQ.value(0).toInt() : 0;
        txLog(QString("事务后员工数：%1（与事务前相同=%2，回滚成功）")
            .arg(afterCount).arg(beforeCount == afterCount ? "✅" : "❌"));
    }

    // ============================================
    // INNER JOIN — 只返回两表都有匹配的行
    // ============================================
    void doInnerJoin()
    {
        /*
        INNER JOIN：
        - 只返回两张表中都有匹配记录的行
        - 如果员工没有对应部门，该员工不会出现在结果中

        语法：
          SELECT 列名
          FROM 表A
          INNER JOIN 表B ON 表A.外键 = 表B.主键
        */
        m_joinModel->setQuery(
            "SELECT e.id, e.name AS 姓名, d.name AS 部门, d.city AS 城市, "
            "       e.salary AS 薪资, e.hire_date AS 入职日期 "
            "FROM employees e "
            "INNER JOIN departments d ON e.dept_id = d.id "
            "ORDER BY e.id"
        );
        m_joinModel->setHeaderData(0, Qt::Horizontal, "ID");
        m_joinModel->setHeaderData(1, Qt::Horizontal, "姓名");
        m_joinModel->setHeaderData(2, Qt::Horizontal, "部门");
        m_joinModel->setHeaderData(3, Qt::Horizontal, "城市");
        m_joinModel->setHeaderData(4, Qt::Horizontal, "薪资");
        m_joinModel->setHeaderData(5, Qt::Horizontal, "入职日期");
        m_joinView->resizeColumnsToContents();
    }

    // ============================================
    // LEFT JOIN — 返回左表所有行，右表无匹配则为 NULL
    // ============================================
    void doLeftJoin()
    {
        /*
        LEFT JOIN：
        - 返回左表（employees）的所有行
        - 右表（departments）无匹配时，对应列为 NULL
        - 适合查找"没有部门的员工"等场景
        */
        m_joinModel->setQuery(
            "SELECT e.id, e.name AS 姓名, "
            "       COALESCE(d.name, '（无部门）') AS 部门, "
            "       e.salary AS 薪资 "
            "FROM employees e "
            "LEFT JOIN departments d ON e.dept_id = d.id "
            "ORDER BY e.id"
        );
        m_joinModel->setHeaderData(0, Qt::Horizontal, "ID");
        m_joinModel->setHeaderData(1, Qt::Horizontal, "姓名");
        m_joinModel->setHeaderData(2, Qt::Horizontal, "部门");
        m_joinModel->setHeaderData(3, Qt::Horizontal, "薪资");
        m_joinView->resizeColumnsToContents();
    }

    // ============================================
    // GROUP BY + 聚合函数
    // ============================================
    void doGroupBy()
    {
        /*
        GROUP BY：按指定列分组，配合聚合函数统计
        常用聚合函数：
          COUNT(*)     — 行数
          SUM(列)      — 求和
          AVG(列)      — 平均值
          MAX(列)      — 最大值
          MIN(列)      — 最小值
          ROUND(值, n) — 四舍五入到 n 位小数
        */
        m_joinModel->setQuery(
            "SELECT d.name AS 部门, "
            "       COUNT(e.id) AS 人数, "
            "       ROUND(AVG(e.salary), 0) AS 平均薪资, "
            "       MAX(e.salary) AS 最高薪资, "
            "       MIN(e.salary) AS 最低薪资, "
            "       SUM(e.salary) AS 薪资总额 "
            "FROM departments d "
            "LEFT JOIN employees e ON e.dept_id = d.id "
            "GROUP BY d.id, d.name "
            "ORDER BY 平均薪资 DESC"
        );
        m_joinModel->setHeaderData(0, Qt::Horizontal, "部门");
        m_joinModel->setHeaderData(1, Qt::Horizontal, "人数");
        m_joinModel->setHeaderData(2, Qt::Horizontal, "平均薪资");
        m_joinModel->setHeaderData(3, Qt::Horizontal, "最高薪资");
        m_joinModel->setHeaderData(4, Qt::Horizontal, "最低薪资");
        m_joinModel->setHeaderData(5, Qt::Horizontal, "薪资总额");
        m_joinView->resizeColumnsToContents();
    }

private:
    void txLog(const QString &msg) { m_txLog->append(msg); }

    QTextEdit      *m_txLog;
    QTableView     *m_joinView;
    QSqlQueryModel *m_joinModel;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：事务中途失败没有 rollback
    db.transaction();
    query1.exec(...);  // 成功
    query2.exec(...);  // 失败，但没有 rollback！
    db.commit();       // 只有 query1 的数据被提交，数据不一致！

✅ 正确：失败时立即 rollback
    db.transaction();
    if (!query1.exec() || !query2.exec()) {
        db.rollback();
        return;
    }
    db.commit();

❌ 错误2：QSqlQueryModel 的查询结果不能直接编辑
    m_model->setQuery("SELECT ...");
    // QSqlQueryModel 是只读的！不能通过它修改数据

✅ 正确：只读显示用 QSqlQueryModel，需要编辑用 QSqlTableModel

❌ 错误3：JOIN 查询中列名冲突
    SELECT * FROM employees e JOIN departments d ON e.dept_id = d.id
    // 两张表都有 id 列，结果中会有歧义！

✅ 正确：明确指定列名和别名
    SELECT e.id, e.name AS emp_name, d.name AS dept_name FROM ...

❌ 错误4：忘记 SQLite 默认不强制外键约束
    // SQLite 默认不检查外键！需要手动开启：
    query.exec("PRAGMA foreign_keys = ON");
    // 每次连接都需要执行（不是持久设置）
*/

// ============================================
// 练习题
// ============================================
/*
1. 查询薪资高于平均薪资的员工：
   SELECT name, salary FROM employees
   WHERE salary > (SELECT AVG(salary) FROM employees)
   提示：子查询（Subquery）

2. 查询每个部门薪资最高的员工：
   SELECT e.name, e.salary, d.name AS dept
   FROM employees e
   JOIN departments d ON e.dept_id = d.id
   WHERE e.salary = (
       SELECT MAX(salary) FROM employees WHERE dept_id = e.dept_id
   )

3. 用事务实现"转岗"功能：
   - 将员工从一个部门转到另一个部门
   - 同时在 transfer_log 表记录转岗历史
   - 两个操作必须同时成功或同时失败
*/

#include "23_sql_advanced.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (!setupAdvancedDb()) {
        QMessageBox::critical(nullptr, "错误", "无法连接数据库！");
        return 1;
    }
    SqlAdvancedDemo window;
    window.show();
    return app.exec();
}
