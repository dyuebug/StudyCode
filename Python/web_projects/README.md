# Web 项目（web_projects）

## Modern Todo App

目录：`todo_app/`

| 文件 | 作用 |
|---|---|
| `index.html` | 页面结构 |
| `style.css` | 响应式样式与动画 |
| `storage.js` | LocalStorage 读写、校验、异常处理 |
| `app.js` | 应用逻辑、事件委托、渲染与统计 |
| `PLAN.md` | 需求与实现计划 |
| `README.md` | 项目说明 |
| `SUMMARY.md` | 项目复盘 |
| `TEST.md` | 手工测试用例 |

### 运行

```bash
# 直接打开
# 或在仓库根目录启动静态服务器
python -m http.server 8080 --directory web_projects/todo_app
```

访问 <http://127.0.0.1:8080>。

### 功能

添加、删除、切换完成状态、筛选（全部/进行中/已完成）、统计、进度条、LocalStorage 持久化、响应式布局、基础 XSS 防护（文本以 `textContent` 写入）。

### 已知边界

- 数据仅保存在当前浏览器的 LocalStorage，不同步、不跨设备。
- `storage.js` 有 10 项 Node 单元测试，`app.js` 有 9 项 jsdom DOM 交互测试；共 19 项，`npm test` 全部通过。浏览器 E2E 测试尚未建立。
- 单文件 `app.js` 约 462 行，后续可拆分为状态层与视图层。

### 文档口径

截至 2026-10-29，目录内共 **12 个文件**（不含 `node_modules/`）：原有 8 个文件，加上 `package.json`、`package-lock.json`、`tests/storage.test.mjs`、`tests/app.test.mjs`。历史文档曾写“7 个文件”，以实际目录为准（见 `../docs/资料异常记录.md` A-005）。
