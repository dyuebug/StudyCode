-- todo-api 第一版表结构（第 33 天迁移自 JSON 文件）
--
-- 【为什么没有多余索引】
-- 当前 API 只有：全量列表、按主键查、按主键改、按主键删。
-- 主键查询自带 B 树索引，其余条件没有过滤列，因此**一个额外索引都不建**。
-- 建索引的唯一依据是"某条慢查询需要它"，不是"这个字段看起来重要"。
-- 等将来加了 ?completed= 过滤或分页排序，再按真实执行计划补索引。

CREATE TABLE todos (
    -- INTEGER PRIMARY KEY 在 SQLite 中即 rowid 的别名，无需 AUTOINCREMENT。
    -- 只有"绝不复用已删除 id"时才需要 AUTOINCREMENT（代价是多一张 sqlite_sequence 表）。
    -- 实测（第 33 天）：插入 → id=3，删除 id=3，再插入 → 又拿到 id=3。
    -- 这是预期的 rowid 语义（max(id)+1），不是 bug；
    -- 但**不要**用 id 大小做大小的判断（比如"id 更大的就是更新的记录"）。
    id         INTEGER PRIMARY KEY,
    title      TEXT    NOT NULL,
    completed  INTEGER NOT NULL DEFAULT 0,
    created_at TEXT    NOT NULL,   -- UTC，ISO 8601
    updated_at TEXT    NOT NULL    -- 每次 UPDATE 必须显式更新（SQLite 无 ON UPDATE）
);
