# 学习论坛（Forum Project）

阶段 4 的论坛系统教学项目，用于串联 Flask 路由、Jinja2 模板、SQLite 数据持久化、用户认证与 pytest 接口测试。

## 功能

- 用户注册、登录、登出（密码哈希存储）
- 帖子列表、分类筛选、关键词搜索、分页
- 发帖、删帖（仅作者本人）、回复
- 只读 JSON API：`GET /api/posts`
- 表单 CSRF 校验、参数长度校验、开放重定向防护

## 目录结构

```
forum_project/
├── __init__.py          # 导出应用工厂
├── app.py               # 路由、校验、安全钩子
├── db.py                # SQLite 连接与初始化
├── schema.sql           # 表结构与初始分类
├── run.py               # 开发环境启动入口
├── templates/           # Jinja2 模板
├── static/              # CSS 与 JavaScript
├── tests/               # pytest 测试
├── requirements.txt     # 本项目依赖
├── README.md
└── DESIGN.md
```

## 运行

在仓库根目录执行：

```bash
python_basics/.venv/Scripts/python.exe -m forum_project.run
```

访问 <http://127.0.0.1:5006>。

也可以使用 Flask CLI：

```bash
python_basics/.venv/Scripts/python.exe -m flask --app forum_project run --debug
```

数据库文件默认生成在 `forum_project/forum.db`。删除该文件后重启会重新建表，不会自动恢复已删除数据。

## 测试

```bash
python_basics/.venv/Scripts/python.exe -m pytest forum_project/tests -q
```

当前结果：**16 passed**。覆盖注册、登录、发帖、回复、删除权限、搜索、分页、JSON API、CSRF 和 404。

测试使用 `tmp_path` 下的独立数据库，不会污染开发数据库。

## JSON API

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/api/posts?page=1&per_page=10` | 分页返回帖子列表 |

响应示例：

```json
{
  "status": "success",
  "page": 1,
  "per_page": 10,
  "items": [
    {"id": 1, "title": "标题", "created_at": "2026-10-29 10:00:00", "username": "alice", "category": "python-basics"}
  ]
}
```

## 参数约束

| 字段 | 约束 |
|---|---|
| 用户名 | 3～20 位中文、字母、数字或下划线 |
| 密码 | 8～128 位 |
| 帖子标题 | 3～100 个字符 |
| 帖子正文 | 1～5000 个字符 |
| 回复 | 1～2000 个字符 |

## 已知边界

- 教学项目未实现管理员角色、帖子编辑、附件上传和全文索引。
- 默认 `SECRET_KEY` 仅用于本地开发，部署前必须通过环境变量覆盖。
- SQLite 适合单机与小规模使用；并发写入场景需要迁移到 PostgreSQL 或 MySQL。
- 未实现速率限制，公网部署前需在反向代理层补充。

## 相关文档

- 设计说明：[`DESIGN.md`](DESIGN.md)
- 部署步骤：[`../deployment/README.md`](../deployment/README.md)
- 资料异常与统计口径：[`../docs/资料异常记录.md`](../docs/资料异常记录.md)
