-- 第一批表结构
CREATE TABLE tasks (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    title      TEXT    NOT NULL,
    completed  INTEGER NOT NULL DEFAULT 0,
    priority   INTEGER NOT NULL DEFAULT 0,
    version    INTEGER NOT NULL DEFAULT 0,
    created_at TEXT    NOT NULL
);

-- 唯一索引：标题不允许重复
CREATE UNIQUE INDEX idx_tasks_title ON tasks (title);

-- 普通索引：按完成状态过滤是高频查询
CREATE INDEX idx_tasks_completed ON tasks (completed);
