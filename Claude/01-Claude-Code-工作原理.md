# Claude Code 工作原理

## 一、概述

Claude Code 是 Anthropic 官方推出的 **Agent 式命令行编程工具**。它运行在终端中，通过自然语言与开发者交互，能够理解代码库、编辑文件、执行命令、管理 Git 工作流，本质上是一个**自主循环的 AI Agent**。

---

## 二、核心架构：Agent Loop（代理循环）

Claude Code 的工作核心是一个 **Agentic Loop（代理循环）** 架构：

```
用户输入 --> 上下文组装 --> Claude API 调用 --> 响应解析
                                                   |
                                           文本回复？--> 输出给用户
                                           工具调用？--> 执行工具 --> 结果注入 --> 继续循环
```

### 2.1 详细流程

```
┌──────────────────────────────────────────────────────────────┐
│                     Claude Code CLI                          │
│                                                              │
│  1. 用户输入                                                  │
│     |                                                        │
│  2. 上下文组装                                                │
│     ├── System Prompt（系统提示词）                            │
│     ├── CLAUDE.md 多层级内容                                  │
│     ├── Output Style（输出风格指令）                           │
│     ├── 对话历史                                              │
│     ├── 可用工具定义（内置 + MCP）                             │
│     └── 权限配置                                              │
│     |                                                        │
│  3. 调用 Claude API（Messages API, stream=true）              │
│     |                                                        │
│  4. 解析流式响应                                              │
│     ├── text block --> 渲染输出给用户                          │
│     └── tool_use block --> 进入工具调度                        │
│         |                                                    │
│  5. 工具调度流程                                              │
│     ├── 权限检查（allow/deny/ask user）                       │
│     ├── PreToolUse Hooks 执行                                │
│     ├── 工具实际执行                                          │
│     ├── PostToolUse Hooks 执行                               │
│     └── tool_result 注入对话                                  │
│         |                                                    │
│  6. 循环判断                                                  │
│     ├── Claude 还有工具调用 --> 回到步骤 3                     │
│     └── Claude 发出 end_turn --> 最终文本输出给用户            │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 关键点

| 方面 | 说明 |
|------|------|
| **流式输出** | Claude Code 使用 SSE 流式 API，实时渲染文字 |
| **多轮工具调用** | 一次用户输入可能触发多轮工具调用循环 |
| **并行工具调用** | Claude 可在单次响应中发出多个 tool_use，Claude Code 并行执行 |
| **自主决策** | Claude 自行决定是否使用工具、使用哪个工具、使用多少次 |

---

## 三、运行模式

Claude Code 支持多种运行模式：

| 模式 | 命令示例 | 说明 |
|------|----------|------|
| **交互模式** | `claude` | 启动 REPL 会话，持续对话 |
| **单次模式** | `claude -m "分析这段代码"` | 发送一条消息后退出 |
| **管道模式** | `echo "内容" \| claude -m "分析"` | 接收 stdin 输入 |
| **CI 模式** | `claude -m "任务" --allowedTools "..." --json` | 无人值守执行 |
| **恢复会话** | `claude --resume <session_id>` | 恢复之前的会话 |
| **继续会话** | `claude --continue` | 继续最近的会话 |

---

## 四、消息模型（与 Claude API 的交互）

Claude Code 底层使用 **Anthropic Messages API**，每次交互构建如下请求结构：

```json
{
  "model": "claude-opus-4-20250514",
  "max_tokens": 16384,
  "stream": true,
  "system": "系统提示词（含 CLAUDE.md、OutputStyle、工具说明等）",
  "tools": [
    {
      "name": "Read",
      "description": "Read a file from the filesystem",
      "input_schema": { "type": "object", "properties": {...} }
    }
  ],
  "messages": [
    { "role": "user", "content": "请帮我分析这个文件" },
    {
      "role": "assistant",
      "content": [
        { "type": "text", "text": "让我读取这个文件。" },
        { "type": "tool_use", "id": "toolu_01ABC", "name": "Read", "input": {"file_path": "/path/to/file.ts"} }
      ]
    },
    {
      "role": "user",
      "content": [
        { "type": "tool_result", "tool_use_id": "toolu_01ABC", "content": "文件内容..." }
      ]
    },
    { "role": "assistant", "content": "分析结果..." }
  ]
}
```

### 角色说明

| 角色 | 内容类型 | 说明 |
|------|----------|------|
| `system` | 系统提示 | CLAUDE.md + OutputStyle + 工具定义 + 行为指令 |
| `user` | 用户消息 / tool_result | 用户输入，或工具执行结果 |
| `assistant` | 文本回复 / tool_use | Claude 的文字回答，或工具调用请求 |

---

## 五、上下文管理

### 5.1 CLAUDE.md 层级系统

Claude Code 使用多层级的 CLAUDE.md 文件提供持久化上下文指令：

```
优先级（从高到低）：

1. ~/.claude/CLAUDE.md              -- 用户全局指令（跨所有项目）
2. <project-root>/CLAUDE.md         -- 项目根级指令
3. <project-root>/.claude/CLAUDE.md -- 项目 .claude 目录指令
4. <subdirectory>/CLAUDE.md         -- 子目录级指令（按需加载）
```

**加载机制：**
- 全局 CLAUDE.md 在每次会话启动时自动加载
- 项目级 CLAUDE.md 在进入项目目录时加载
- 子目录级 CLAUDE.md 在操作该目录文件时按需加载
- 所有内容注入为系统提示词的一部分

### 5.2 上下文窗口管理

Claude 模型有上下文窗口限制（如 Opus 4 为 200K tokens）。管理策略：

| 策略 | 说明 |
|------|------|
| **自动压缩** | 对话接近上下文限制时自动触发摘要压缩 |
| **手动压缩** | 用户执行 `/compact` 或 `/compact [自定义总结指令]` |
| **子代理隔离** | Task 子代理有独立上下文，只返回最终结果给主会话 |
| **工具输出截断** | Read 默认最多 2000 行，超长行截断 |
| **选择性加载** | 只在需要时读取文件内容 |

### 5.3 会话持久化

```bash
claude --resume <session_id>    # 恢复指定会话
claude --continue               # 继续最近会话
```

会话数据存储在本地，包含完整对话历史和工具调用记录。

---

## 六、权限系统

权限采用白名单/黑名单机制，配置在 `settings.json` 中：

```json
{
  "permissions": {
    "allow": ["Bash", "Read", "Write", "Edit", "mcp__context7"],
    "deny": []
  }
}
```

**匹配规则：**
- **精确匹配**：`"Bash"` 匹配 Bash 工具
- **前缀匹配**：`"mcp__context7"` 匹配 context7 服务器的所有工具
- **不在 allow 也不在 deny 中的工具** --> 触发用户确认弹窗

### 权限检查流程

```
Claude 请求使用工具
    |
    v
在 allow 列表中？ --是--> 直接执行
    |否
在 deny 列表中？  --是--> 拒绝执行
    |否
请求用户确认 --> 用户选择允许/拒绝
```

---

## 七、工具系统

### 7.1 内置工具

| 工具名 | 功能 | 典型用途 |
|--------|------|----------|
| `Read` | 读取文件 | 文本、图片、PDF、Jupyter Notebook |
| `Write` | 写入文件 | 创建/覆盖文件 |
| `Edit` | 编辑文件 | 基于搜索/替换的精确编辑 |
| `Bash` | 执行 Shell 命令 | git、npm、docker 等 |
| `Glob` | 文件模式匹配 | 按 glob 模式查找文件 |
| `Grep` | 正则搜索 | 基于 ripgrep 的全文搜索 |
| `Task` | 生成子代理 | 创建独立上下文的子任务 |
| `TaskOutput` | 获取子任务结果 | 等待后台任务完成 |
| `WebFetch` | 获取网页内容 | 抓取 URL 并处理 |
| `WebSearch` | 网络搜索 | 搜索引擎查询 |
| `NotebookEdit` | Jupyter 编辑 | 编辑 .ipynb 文件 |
| `AskUserQuestion` | 询问用户 | 在工具循环中请求用户输入 |

### 7.2 MCP 工具

通过 MCP 协议从外部服务器获取，命名格式：`mcp__<服务器名>__<工具名>`

示例：`mcp__context7__query-docs`、`mcp__Playwright__browser_click`

### 7.3 工具调度完整流程

```
Claude 响应包含 tool_use block
         |
         v
   ┌─────────────┐
   │  权限检查    │ --> allow/deny/ask
   └─────────────┘
         |
         v
   ┌──────────────────┐
   │  PreToolUse      │ --> hooks 检查 (allow/block/modify)
   │  Hooks 执行      │
   └──────────────────┘
         |
         v
   ┌──────────────────┐
   │  工具实际执行     │ --> 各工具的具体实现
   └──────────────────┘
         |
         v
   ┌──────────────────┐
   │  PostToolUse     │ --> hooks 后处理 (日志/格式化/通知)
   │  Hooks 执行      │
   └──────────────────┘
         |
         v
   tool_result 注入对话 --> Claude 继续推理
```

---

## 八、配置体系

### 8.1 配置文件层级

```
优先级（从高到低，后者覆盖前者）：

1. 企业管理策略               -- 管理员强制设定
2. ~/.claude/settings.json    -- 用户全局设置
3. .claude/settings.json      -- 项目级设置
4. 命令行参数                 -- 运行时覆盖
5. 环境变量                   -- 系统级配置
```

### 8.2 settings.json 核心结构

```json
{
  "$schema": "https://json.schemastore.org/claude-code-settings.json",
  "env": {
    "ANTHROPIC_AUTH_TOKEN": "...",
    "ANTHROPIC_BASE_URL": "https://api.anthropic.com",
    "MCP_TIMEOUT": "60000"
  },
  "permissions": {
    "allow": ["Bash", "Read", "Write", "Edit"],
    "deny": []
  },
  "hooks": {},
  "model": "opus",
  "outputStyle": "engineer-professional",
  "includeCoAuthoredBy": false,
  "mcpServers": {}
}
```

### 8.3 关键环境变量

| 变量 | 功能 |
|------|------|
| `ANTHROPIC_BASE_URL` | API 端点地址 |
| `ANTHROPIC_AUTH_TOKEN` | 认证令牌 |
| `MCP_TIMEOUT` | MCP 工具调用超时（毫秒） |
| `CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC` | 禁止非必要网络请求 |
| `DISABLE_TELEMETRY` | 禁用遥测 |

---

## 九、总结架构图

```
┌──────────────────────────────────────────────────────────────────┐
│                        Claude Code CLI                           │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    配置层                                 │   │
│  │  settings.json + CLAUDE.md + OutputStyle + Permissions    │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              |                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    输入层                                 │   │
│  │  REPL / pipe / --message / --resume / slash commands      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              |                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              上下文组装 + Hooks (SessionStart)             │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              |                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                   Agentic Loop                            │   │
│  │                                                           │   │
│  │  Claude API <--> 文本输出 / 工具调用                       │   │
│  │       ^              |                                    │   │
│  │       |      权限检查 -> PreHook -> 执行 -> PostHook      │   │
│  │       |              |                                    │   │
│  │       +--- tool_result 注入 <--+                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              |                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    扩展层                                 │   │
│  │  MCP Servers | SubAgents | Plugins | Skills | Hooks       │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```
