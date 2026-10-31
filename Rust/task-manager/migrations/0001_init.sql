-- ============================================
-- task-manager 初始迁移
-- 日期：2026-11-01（第37天）
-- ============================================
--
-- 【为什么先写 SQL，再写 Rust 结构体】
--
-- 表结构是整个系统里**最晚能改**的东西：
-- 代码重构是分钟级，方向改了是小时级，表结构改了是"数据要迁移"级别。
-- 所以它必须先定下来，而且要以"数据库能接受的语法"定，
-- 而不是先用 Rust 结构体描述一遍再翻译——翻译过程中的每一次"应该差不多"
-- 都会在第一次跑迁移时变成错误。
--
-- 【五张表的形状】
--
--   users ──< projects ──< tasks >── users（assignee，可为空）
--                            │
--                            └──< task_tags >── tags
--
--   一对多的用外键 + ON DELETE；多对多的用中间表 + 复合主键。

-- --------------------------------------------
-- 用户
-- --------------------------------------------
CREATE TABLE users (
    id           INTEGER PRIMARY KEY,
    username     TEXT    NOT NULL UNIQUE,
    display_name TEXT    NOT NULL,
    created_at   TEXT    NOT NULL DEFAULT (datetime('now'))
);

-- --------------------------------------------
-- 项目（属于某个用户）
-- --------------------------------------------
CREATE TABLE projects (
    id          INTEGER PRIMARY KEY,
    -- ON DELETE CASCADE：删用户 → 他的项目一起没了。
    -- 选 CASCADE 还是 RESTRICT 是业务决策，不是技术决策：
    -- 任务系统里"用户注销"是删除账号还是保留数据，要由产品来说。
    -- 这里按"个人任务管理"的直觉选 CASCADE。
    owner_id    INTEGER NOT NULL REFERENCES users (id) ON DELETE CASCADE,
    name        TEXT    NOT NULL,
    description TEXT    NOT NULL DEFAULT '',
    created_at  TEXT    NOT NULL DEFAULT (datetime('now')),

    -- 同一用户下项目名不能重复。放在数据库而不是应用层，
    -- 是因为**并发下应用层的"先查再插"必然有窗口**。
    UNIQUE (owner_id, name)
);

CREATE INDEX idx_projects_owner ON projects (owner_id);

-- --------------------------------------------
-- 任务
-- --------------------------------------------
CREATE TABLE tasks (
    id          INTEGER PRIMARY KEY,
    project_id  INTEGER NOT NULL REFERENCES projects (id) ON DELETE CASCADE,
    title       TEXT    NOT NULL,

    -- 状态用 TEXT + CHECK，而不是 INTEGER 枚举。
    -- 理由：可直接读（SELECT 出来就是 todo/doing/done），
    -- 且加值只需改 CHECK，不用担心"0 到底是 todo 还是 backlog"。
    -- 代价：比整数占空间、排序要自己定义顺序。任务表这个量级无所谓。
    status      TEXT    NOT NULL DEFAULT 'todo'
                        CHECK (status IN ('todo', 'doing', 'done')),

    -- 1=最高 4=最低。用 BETWEEN 约束住，避免出现 priority=99
    priority    INTEGER NOT NULL DEFAULT 2
                        CHECK (priority BETWEEN 1 AND 4),

    -- 指派给谁。ON DELETE SET NULL：人走了，任务留下但变成"未指派"。
    -- 这比 CASCADE（连任务一起删）安全得多——任务不应该因为人离职而消失。
    assignee_id INTEGER REFERENCES users (id) ON DELETE SET NULL,

    -- 截止时间，可空。存 ISO8601 文本（SQLite 没有原生日期类型）。
    due_at      TEXT,

    created_at  TEXT    NOT NULL DEFAULT (datetime('now')),
    updated_at  TEXT    NOT NULL DEFAULT (datetime('now'))
);

-- 按项目列任务：最常见的查询，必须有索引
CREATE INDEX idx_tasks_project ON tasks (project_id);
-- 按"某项目下的某状态"筛选：复合索引的第二列才用得上
CREATE INDEX idx_tasks_project_status ON tasks (project_id, status);

-- --------------------------------------------
-- 标签（多对多）
-- --------------------------------------------
CREATE TABLE tags (
    id   INTEGER PRIMARY KEY,
    name TEXT    NOT NULL UNIQUE
);

-- 中间表：复合主键天然去重（同一个 task 不会被贴两次同一个 tag）
CREATE TABLE task_tags (
    task_id INTEGER NOT NULL REFERENCES tasks (id) ON DELETE CASCADE,
    tag_id  INTEGER NOT NULL REFERENCES tags (id) ON DELETE CASCADE,
    PRIMARY KEY (task_id, tag_id)
);

-- 反向查询"这个标签下有哪些任务"需要单独索引：
-- 复合主键的前缀是 task_id，查 tag_id 用不上它。
CREATE INDEX idx_task_tags_tag ON task_tags (tag_id);

-- --------------------------------------------
-- 种子数据（第 37 天只为了让链路能跑通）
-- --------------------------------------------
INSERT INTO users (id, username, display_name) VALUES
    (1, 'alice', 'Alice'),
    (2, 'bob',   'Bob');

INSERT INTO projects (id, owner_id, name, description) VALUES
    (1, 1, '学习 Rust', '16 周路线图的执行记录');

INSERT INTO tasks (id, project_id, title, status, priority, assignee_id) VALUES
    (1, 1, '第 37 天：项目骨架',   'done',  1, 1),
    (2, 1, '第 38 天：完整 CRUD',  'doing', 1, 1),
    (3, 1, '第 39 天：中间件与收尾', 'todo',  2, 1);

INSERT INTO tags (id, name) VALUES (1, 'rust'), (2, 'web');

INSERT INTO task_tags (task_id, tag_id) VALUES (1, 1), (2, 1), (2, 2);
