# Qt 数据库编程 — 阶段8知识点速查表

> 日期：2026-06-07（第47天）  
> 阶段：阶段8 - Qt 数据库编程  
> 用途：快速查阅 Qt SQL 常用 API

---

## 一、QSqlDatabase 连接速查

```cpp
// 连接 SQLite（最常用，无需安装服务器）
QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
db.setDatabaseName("myapp.db");   // 文件路径，":memory:" 为内存库
if (!db.open()) {
    qDebug() << db.lastError().text();
}

// 连接 MySQL
QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
db.setHostName("localhost");
db.setPort(3306);
db.setDatabaseName("mydb");
db.setUserName("root");
db.setPassword("password");
db.open();

// 多连接（命名连接）
QSqlDatabase db1 = QSqlDatabase::addDatabase("QSQLITE", "conn1");
QSqlDatabase db2 = QSqlDatabase::addDatabase("QSQLITE", "conn2");
QSqlDatabase::database("conn1");  // 获取命名连接

// 关闭连接
db.close();
QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
```

---

## 二、QSqlQuery CRUD 速查

```cpp
QSqlQuery q;

// CREATE TABLE
q.exec("CREATE TABLE IF NOT EXISTS users ("
       "  id    INTEGER PRIMARY KEY AUTOINCREMENT,"
       "  name  TEXT NOT NULL,"
       "  age   INTEGER DEFAULT 0"
       ")");

// INSERT（参数化查询，防 SQL 注入）
q.prepare("INSERT INTO users (name, age) VALUES (:name, :age)");
q.bindValue(":name", "张三");
q.bindValue(":age",  25);
q.exec();
int newId = q.lastInsertId().toInt();  // 获取自增 ID

// SELECT + 遍历结果集
q.exec("SELECT id, name, age FROM users ORDER BY id");
while (q.next()) {
    int    id   = q.value("id").toInt();
    QString name = q.value("name").toString();
    int    age  = q.value("age").toInt();
    // 也可以用列索引：q.value(0).toInt()
}

// UPDATE
q.prepare("UPDATE users SET age=:age WHERE id=:id");
q.bindValue(":age", 26);
q.bindValue(":id",  1);
q.exec();
int affected = q.numRowsAffected();  // 受影响行数

// DELETE
q.prepare("DELETE FROM users WHERE id=:id");
q.bindValue(":id", 1);
q.exec();

// 错误检查
if (!q.exec("...")) {
    qDebug() << q.lastError().text();
}
```

---

## 三、事务速查

```cpp
QSqlDatabase db = QSqlDatabase::database();

// 标准事务模板
db.transaction();
bool ok = true;

QSqlQuery q;
for (auto &item : items) {
    q.prepare("INSERT INTO ...");
    q.bindValue(...);
    if (!q.exec()) {
        ok = false;
        break;
    }
}

if (ok) {
    db.commit();    // 全部成功，提交
} else {
    db.rollback();  // 有失败，回滚
}

// 事务的作用：
// 1. 原子性：批量操作要么全成功，要么全撤销
// 2. 性能：批量插入用事务比逐条插入快 10-100 倍
```

---

## 四、常用 SQL 语句速查

```sql
-- JOIN 关联查询
SELECT e.name, d.name AS dept
FROM employees e
INNER JOIN departments d ON e.dept_id = d.id   -- 只返回有匹配的行

SELECT e.name, COALESCE(d.name, '无部门') AS dept
FROM employees e
LEFT JOIN departments d ON e.dept_id = d.id    -- 左表全返回，右表无匹配为 NULL

-- 聚合函数 + GROUP BY
SELECT dept_id,
       COUNT(*)        AS 人数,
       AVG(salary)     AS 平均薪资,
       MAX(salary)     AS 最高薪资,
       MIN(salary)     AS 最低薪资,
       SUM(salary)     AS 薪资总额
FROM employees
GROUP BY dept_id
HAVING COUNT(*) > 2    -- HAVING 过滤分组结果（WHERE 过滤行，HAVING 过滤组）

-- 子查询
SELECT name, salary FROM employees
WHERE salary > (SELECT AVG(salary) FROM employees)

-- 模糊查询
SELECT * FROM users WHERE name LIKE '%张%'      -- 包含"张"
SELECT * FROM users WHERE name LIKE '张%'       -- 以"张"开头

-- 排序 + 分页
SELECT * FROM users ORDER BY age DESC LIMIT 10 OFFSET 20
-- LIMIT 10：取10条；OFFSET 20：跳过前20条（第3页，每页10条）

-- SQLite 开启外键约束（每次连接都需要执行）
PRAGMA foreign_keys = ON;
```

---

## 五、QSqlTableModel 速查

```cpp
QSqlTableModel *model = new QSqlTableModel(this);
model->setTable("users");
model->setEditStrategy(QSqlTableModel::OnManualSubmit);  // 手动提交
model->select();  // 加载数据

// 设置列标题
model->setHeaderData(0, Qt::Horizontal, "ID");
model->setHeaderData(1, Qt::Horizontal, "姓名");

// 过滤（WHERE 子句）
model->setFilter("age > 18");
model->select();

// 排序
model->setSort(2, Qt::DescendingOrder);  // 按第2列降序
model->select();

// 插入行
model->insertRow(model->rowCount());
model->setData(model->index(row, 1), "新用户");

// 提交 / 撤销
model->submitAll();   // 写入数据库
model->revertAll();   // 撤销未提交的更改

// 删除行（从后往前删，避免索引偏移）
model->removeRow(row);
model->submitAll();
```

---

## 六、QSqlRelationalTableModel 速查

```cpp
QSqlRelationalTableModel *model = new QSqlRelationalTableModel(this);
model->setTable("contacts");
model->setEditStrategy(QSqlTableModel::OnManualSubmit);

// 设置外键关联：第4列(group_id) 关联 groups 表，显示 name 列
model->setRelation(4, QSqlRelation("groups", "id", "name"));
model->select();

// 绑定到 QTableView
view->setModel(model);
// 使用关联代理，外键列编辑时显示为下拉框
view->setItemDelegate(new QSqlRelationalDelegate(view));
```

---

## 七、QSqlQueryModel 速查（只读）

```cpp
// 用于显示任意 SQL 查询结果（只读，不能编辑）
QSqlQueryModel *model = new QSqlQueryModel(this);
model->setQuery("SELECT e.name, d.name FROM employees e JOIN departments d ON ...");
model->setHeaderData(0, Qt::Horizontal, "员工");
model->setHeaderData(1, Qt::Horizontal, "部门");

view->setModel(model);
```

---

## 八、三种模型对比

| 模型 | 可编辑 | 外键支持 | 适用场景 |
|------|--------|---------|---------|
| QSqlQueryModel | ❌ 只读 | 手动 JOIN | 复杂查询结果展示 |
| QSqlTableModel | ✅ 可编辑 | ❌ 显示 ID | 单表增删改查 |
| QSqlRelationalTableModel | ✅ 可编辑 | ✅ 自动显示文字 | 有外键的表 |

---

## 九、QSortFilterProxyModel 速查

```cpp
QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
proxy->setSourceModel(model);           // 包装原始模型
proxy->setFilterKeyColumn(1);           // 按第1列过滤（-1 = 所有列）
proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

view->setModel(proxy);
view->setSortingEnabled(true);          // 允许点击表头排序

// 设置过滤关键词
proxy->setFilterFixedString("张");      // 固定字符串
proxy->setFilterRegularExpression("^张"); // 正则表达式

// 代理索引映射回源模型（删除/编辑时必须！）
QModelIndex srcIdx = proxy->mapToSource(proxyIdx);
model->removeRow(srcIdx.row());
```
