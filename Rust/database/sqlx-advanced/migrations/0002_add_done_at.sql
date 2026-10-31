-- 第二批：新增一列
-- 注意 ALTER TABLE 只能加可空列或带默认值的列，SQLite 不支持随意改列
ALTER TABLE tasks ADD COLUMN done_at TEXT;
