/*
 * Qt 数据库编程基础：QSqlDatabase + QSqlQuery
 * 日期：2026-06-05（第45天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. 连接 SQLite 数据库
 * 2. 创建表（CREATE TABLE）
 * 3. 插入数据（INSERT）
 * 4. 查询数据（SELECT）
 * 5. 更新数据（UPDATE）
 * 6. 删除数据（DELETE）
 * 7. 参数化查询（防 SQL 注入）
 * 8. 错误处理
 *
 * 编译前必须在 .pro 文件添加：QT += sql
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】Qt SQL 模块
// ============================================
/*
是什么？
- Qt SQL 模块提供了数据库访问的统一接口
- 支持多种数据库：SQLite、MySQL、PostgreSQL、ODBC 等
- SQLite 是内置支持的，无需安装额外驱动

为什么用 SQLite？
- 无服务器：数据库就是一个 .db 文件，随程序一起分发
- 零配置：不需要安装、配置数据库服务
- 适合桌面应用的本地数据持久化

核心类：
- QSqlDatabase：管理数据库连接
- QSqlQuery：执行 SQL 语句，遍历结果集
- QSqlError：描述数据库错误信息

⚠️ 重要：.pro 文件必须添加 QT += sql
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDateTime>

// ============================================
// 数据库辅助函数
// ============================================

// 初始化数据库连接，返回是否成功
bool initDatabase()
{
    // --------------------------------------------
    // QSqlDatabase::addDatabase — 添加数据库连接
    // --------------------------------------------
    /*
    【函数卡片】QSqlDatabase::addDatabase()

    语法：
      QSqlDatabase addDatabase(const QString &type,
                               const QString &connectionName = QLatin1String(defaultConnection))

    参数：
      type：数据库驱动类型
        "QSQLITE"   — SQLite（内置，推荐桌面应用）
        "QMYSQL"    — MySQL
        "QPSQL"     — PostgreSQL
        "QODBC"     — ODBC（Windows）
      connectionName：连接名称（多连接时区分用）

    返回值：QSqlDatabase 对象（引用计数管理）
    */
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    // --------------------------------------------
    // setDatabaseName — 设置数据库文件路径
    // --------------------------------------------
    /*
    SQLite 特殊值：
      ":memory:"  — 内存数据库（程序退出后数据消失，适合测试）
      "myapp.db"  — 当前目录下的文件（程序退出后数据保留）
      "/path/to/myapp.db" — 绝对路径
    */
    db.setDatabaseName("students.db");  // 在当前目录创建 students.db

    // --------------------------------------------
    // open — 打开数据库连接
    // --------------------------------------------
    if (!db.open()) {
        qDebug() << "数据库连接失败：" << db.lastError().text();
        return false;
    }

    qDebug() << "数据库连接成功：students.db";
    return true;
}

// 创建表（如果不存在）
bool createTable()
{
    QSqlQuery query;

    // --------------------------------------------
    // QSqlQuery::exec — 执行 SQL 语句
    // --------------------------------------------
    /*
    【函数卡片】QSqlQuery::exec()

    语法：bool exec(const QString &query)
    返回值：true = 执行成功，false = 执行失败
    错误信息：query.lastError().text()

    CREATE TABLE IF NOT EXISTS：
      - IF NOT EXISTS：表已存在时不报错，直接跳过
      - INTEGER PRIMARY KEY：整数主键，SQLite 自动自增
      - TEXT：字符串类型
      - INTEGER：整数类型
      - REAL：浮点数类型
      - NOT NULL：非空约束
      - DEFAULT：默认值
    */
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS students ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name    TEXT    NOT NULL,"
        "  age     INTEGER NOT NULL,"
        "  score   REAL    DEFAULT 0.0,"
        "  created TEXT    DEFAULT ''"
        ")"
    );

    if (!ok) {
        qDebug() << "创建表失败：" << query.lastError().text();
        return false;
    }

    qDebug() << "表创建成功（或已存在）";
    return true;
}

// ============================================
// 主界面
// ============================================
class SqlBasicsDemo : public QWidget
{
    Q_OBJECT

public:
    SqlBasicsDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt SQL 基础演示");
        resize(700, 600);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        // 标题
        QLabel *title = new QLabel("Qt SQL 基础 — SQLite 增删改查演示");
        title->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        // ----------------------------------------
        // 输入表单
        // ----------------------------------------
        QFormLayout *form = new QFormLayout();

        m_nameEdit = new QLineEdit();
        m_nameEdit->setPlaceholderText("输入学生姓名");
        form->addRow("姓名：", m_nameEdit);

        m_ageSpinBox = new QSpinBox();
        m_ageSpinBox->setRange(1, 100);
        m_ageSpinBox->setValue(20);
        form->addRow("年龄：", m_ageSpinBox);

        m_scoreEdit = new QLineEdit();
        m_scoreEdit->setPlaceholderText("输入成绩（0-100）");
        form->addRow("成绩：", m_scoreEdit);

        m_idEdit = new QLineEdit();
        m_idEdit->setPlaceholderText("输入 ID（更新/删除时使用）");
        form->addRow("ID：", m_idEdit);

        mainLayout->addLayout(form);

        // ----------------------------------------
        // 操作按钮
        // ----------------------------------------
        QHBoxLayout *btnLayout = new QHBoxLayout();

        auto makeBtn = [](const QString &text, const QString &color) {
            QPushButton *btn = new QPushButton(text);
            btn->setStyleSheet(
                QString("QPushButton{background:%1;color:white;border:none;"
                        "border-radius:4px;padding:8px 16px;font-weight:bold;}"
                        "QPushButton:hover{opacity:0.8;}").arg(color)
            );
            return btn;
        };

        QPushButton *insertBtn = makeBtn("INSERT 插入", "#27ae60");
        QPushButton *selectBtn = makeBtn("SELECT 查询全部", "#3498db");
        QPushButton *updateBtn = makeBtn("UPDATE 更新", "#e67e22");
        QPushButton *deleteBtn = makeBtn("DELETE 删除", "#e74c3c");
        QPushButton *clearBtn  = makeBtn("清空日志", "#95a5a6");

        btnLayout->addWidget(insertBtn);
        btnLayout->addWidget(selectBtn);
        btnLayout->addWidget(updateBtn);
        btnLayout->addWidget(deleteBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(clearBtn);
        mainLayout->addLayout(btnLayout);

        // ----------------------------------------
        // 日志输出区域
        // ----------------------------------------
        m_log = new QTextEdit();
        m_log->setReadOnly(true);
        m_log->setFont(QFont("Consolas", 11));
        m_log->setStyleSheet(
            "QTextEdit{background:#1e1e2e;color:#cdd6f4;border:none;"
            "border-radius:6px;padding:8px;}"
        );
        mainLayout->addWidget(m_log);

        // 连接信号
        connect(insertBtn, &QPushButton::clicked, this, &SqlBasicsDemo::doInsert);
        connect(selectBtn, &QPushButton::clicked, this, &SqlBasicsDemo::doSelect);
        connect(updateBtn, &QPushButton::clicked, this, &SqlBasicsDemo::doUpdate);
        connect(deleteBtn, &QPushButton::clicked, this, &SqlBasicsDemo::doDelete);
        connect(clearBtn,  &QPushButton::clicked, m_log, &QTextEdit::clear);

        // 启动时查询一次
        log("=== Qt SQL 基础演示启动 ===");
        log("数据库文件：students.db（当前目录）");
        log("");
        doSelect();
    }

private slots:

    // ============================================
    // INSERT — 插入数据
    // ============================================
    void doInsert()
    {
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入姓名！");
            return;
        }

        int age = m_ageSpinBox->value();
        double score = m_scoreEdit->text().toDouble();
        QString created = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        // --------------------------------------------
        // 参数化查询（防止 SQL 注入）⭐⭐⭐⭐⭐
        // --------------------------------------------
        /*
        ❌ 危险做法（字符串拼接，有 SQL 注入风险）：
          query.exec("INSERT INTO students (name) VALUES ('" + name + "')");
          如果 name = "'; DROP TABLE students; --"，会删除整张表！

        ✅ 安全做法（参数化查询）：
          query.prepare("INSERT INTO students (name) VALUES (:name)");
          query.bindValue(":name", name);
          query.exec();
          Qt 会自动转义特殊字符，防止注入攻击。

        【函数卡片】QSqlQuery::prepare() + bindValue()

        prepare()：预编译 SQL 语句，用 :参数名 作为占位符
        bindValue(":参数名", 值)：绑定参数值
        exec()：执行预编译的语句
        */
        QSqlQuery query;
        query.prepare(
            "INSERT INTO students (name, age, score, created) "
            "VALUES (:name, :age, :score, :created)"
        );
        query.bindValue(":name",    name);
        query.bindValue(":age",     age);
        query.bindValue(":score",   score);
        query.bindValue(":created", created);

        if (query.exec()) {
            // lastInsertId()：获取刚插入行的自增 ID
            int newId = query.lastInsertId().toInt();
            log(QString("✅ INSERT 成功：id=%1, name=%2, age=%3, score=%4")
                .arg(newId).arg(name).arg(age).arg(score));
            m_nameEdit->clear();
            m_scoreEdit->clear();
            doSelect();  // 刷新显示
        } else {
            log("❌ INSERT 失败：" + query.lastError().text());
        }
    }

    // ============================================
    // SELECT — 查询数据
    // ============================================
    void doSelect()
    {
        QSqlQuery query;

        // 执行查询
        if (!query.exec("SELECT id, name, age, score, created FROM students ORDER BY id")) {
            log("❌ SELECT 失败：" + query.lastError().text());
            return;
        }

        // --------------------------------------------
        // 遍历结果集
        // --------------------------------------------
        /*
        【函数卡片】QSqlQuery 结果集遍历

        query.next()：移动到下一行，返回 false 表示没有更多行
        query.value(列索引)：获取当前行指定列的值（返回 QVariant）
        query.value("列名")：按列名获取（更清晰，推荐）

        QVariant 转换：
          .toInt()     — 转整数
          .toString()  — 转字符串
          .toDouble()  — 转浮点数
          .toBool()    — 转布尔
        */
        log("--- SELECT 结果 ---");
        int count = 0;
        while (query.next()) {
            int    id      = query.value("id").toInt();
            QString name   = query.value("name").toString();
            int    age     = query.value("age").toInt();
            double score   = query.value("score").toDouble();
            QString created = query.value("created").toString();

            log(QString("  [%1] %2 | 年龄:%3 | 成绩:%4 | 创建:%5")
                .arg(id).arg(name).arg(age).arg(score, 0, 'f', 1).arg(created));
            count++;
        }

        if (count == 0) {
            log("  （暂无数据）");
        }
        log(QString("共 %1 条记录").arg(count));
        log("");
    }

    // ============================================
    // UPDATE — 更新数据
    // ============================================
    void doUpdate()
    {
        QString idStr = m_idEdit->text().trimmed();
        if (idStr.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入要更新的 ID！");
            return;
        }

        QString name = m_nameEdit->text().trimmed();
        int age = m_ageSpinBox->value();
        double score = m_scoreEdit->text().toDouble();

        QSqlQuery query;
        query.prepare(
            "UPDATE students SET name=:name, age=:age, score=:score WHERE id=:id"
        );
        query.bindValue(":name",  name.isEmpty() ? QVariant() : name);
        query.bindValue(":age",   age);
        query.bindValue(":score", score);
        query.bindValue(":id",    idStr.toInt());

        if (query.exec()) {
            // numRowsAffected()：受影响的行数
            int affected = query.numRowsAffected();
            if (affected > 0) {
                log(QString("✅ UPDATE 成功：id=%1，更新了 %2 行").arg(idStr).arg(affected));
                doSelect();
            } else {
                log(QString("⚠️ UPDATE：id=%1 不存在").arg(idStr));
            }
        } else {
            log("❌ UPDATE 失败：" + query.lastError().text());
        }
    }

    // ============================================
    // DELETE — 删除数据
    // ============================================
    void doDelete()
    {
        QString idStr = m_idEdit->text().trimmed();
        if (idStr.isEmpty()) {
            QMessageBox::warning(this, "提示", "请输入要删除的 ID！");
            return;
        }

        auto reply = QMessageBox::question(this, "确认删除",
            QString("确定要删除 id=%1 的记录吗？").arg(idStr),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        QSqlQuery query;
        query.prepare("DELETE FROM students WHERE id=:id");
        query.bindValue(":id", idStr.toInt());

        if (query.exec()) {
            int affected = query.numRowsAffected();
            if (affected > 0) {
                log(QString("✅ DELETE 成功：id=%1").arg(idStr));
                m_idEdit->clear();
                doSelect();
            } else {
                log(QString("⚠️ DELETE：id=%1 不存在").arg(idStr));
            }
        } else {
            log("❌ DELETE 失败：" + query.lastError().text());
        }
    }

private:
    void log(const QString &msg)
    {
        m_log->append(msg);
    }

    QLineEdit *m_nameEdit;
    QSpinBox  *m_ageSpinBox;
    QLineEdit *m_scoreEdit;
    QLineEdit *m_idEdit;
    QTextEdit *m_log;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：忘记在 .pro 文件添加 QT += sql
    编译报错：QSqlDatabase: No such file or directory

✅ 正确：在 .pro 文件中添加
    QT += core gui sql widgets

❌ 错误2：字符串拼接 SQL（SQL 注入漏洞）
    query.exec("SELECT * FROM users WHERE name='" + userInput + "'");
    // 如果 userInput = "' OR '1'='1"，会返回所有用户！

✅ 正确：参数化查询
    query.prepare("SELECT * FROM users WHERE name=:name");
    query.bindValue(":name", userInput);
    query.exec();

❌ 错误3：不检查 exec() 返回值
    query.exec("INSERT INTO ...");
    // 插入失败了也不知道！

✅ 正确：
    if (!query.exec("INSERT INTO ...")) {
        qDebug() << query.lastError().text();
    }

❌ 错误4：在 open() 之前执行查询
    QSqlQuery query;
    query.exec("SELECT ...");  // 数据库还没打开！

✅ 正确：先 open()，再执行查询

❌ 错误5：忘记关闭数据库（通常不需要手动关闭）
    // Qt 会在 QSqlDatabase 对象销毁时自动关闭
    // 但如果需要手动关闭：
    QSqlDatabase::database().close();
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加"按姓名搜索"功能：
   - 输入姓名关键词，查询包含该关键词的所有学生
   - 提示：SELECT * FROM students WHERE name LIKE :pattern
   - bindValue(":pattern", "%" + keyword + "%")

2. 添加"成绩统计"功能：
   - 显示：最高分、最低分、平均分、总人数
   - 提示：SELECT MAX(score), MIN(score), AVG(score), COUNT(*) FROM students

3. 添加"事务"支持：
   - 批量插入10条记录，要么全部成功，要么全部失败
   - 提示：
     QSqlDatabase::database().transaction();
     // 执行多条 INSERT
     QSqlDatabase::database().commit();    // 全部成功
     QSqlDatabase::database().rollback();  // 有失败则回滚
*/

#include "20_sql_basics.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 初始化数据库
    if (!initDatabase()) {
        QMessageBox::critical(nullptr, "错误", "无法连接数据库！");
        return 1;
    }

    // 创建表
    if (!createTable()) {
        QMessageBox::critical(nullptr, "错误", "无法创建数据库表！");
        return 1;
    }

    SqlBasicsDemo window;
    window.show();
    return app.exec();
}
