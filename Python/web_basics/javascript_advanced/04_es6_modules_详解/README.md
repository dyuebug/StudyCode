# 📦 ES6模块化深入理解

> 第47天学习材料 - JavaScript高级特性

## 📚 学习目标

- 理解模块化的概念和价值
- 掌握ES6 import/export语法
- 理解命名导出vs默认导出
- 学会模块化重构实际项目
- 对比ES6模块和CommonJS

---

## 📂 文件结构

```
04_es6_modules_详解/
├── index.html           # 主页面（查看说明）
├── main.js              # 主入口文件（演示导入）
├── modules/
│   ├── math.js         # 数学工具（命名导出）
│   ├── string.js       # 字符串工具（默认导出）
│   ├── user.js         # 用户模块（混合导出）
│   └── config.js       # 配置模块
└── README.md           # 本文件
```

---

## 🚀 如何运行

### ⚠️ 重要提示

ES6模块**必须在服务器环境**运行，不能直接用`file://`协议打开。

### 方法1：VS Code Live Server（推荐）

1. 安装VS Code扩展：**Live Server**
2. 右键点击`index.html` → "Open with Live Server"
3. 浏览器自动打开
4. 按**F12**打开控制台查看输出

### 方法2：Python HTTP服务器

```bash
# 在当前目录运行
python -m http.server 8000

# 浏览器访问
http://localhost:8000/index.html
```

### 方法3：Node.js http-server

```bash
# 安装（只需一次）
npm install -g http-server

# 启动服务器
http-server

# 浏览器访问
http://localhost:8080/index.html
```

---

## 📖 核心知识点

### 1. 什么是模块化？

**模块化**是将大型程序拆分成独立的、可复用的模块。

**优点：**
- ✅ 代码组织清晰
- ✅ 避免全局变量污染
- ✅ 依赖关系明确
- ✅ 代码可复用
- ✅ 按需加载

### 2. 导出方式（Export）

#### 2.1 命名导出（可以有多个）

```javascript
// math.js
export const PI = 3.14;
export function add(a, b) {
    return a + b;
}
export class Calculator {}

// 或者统一导出
const PI = 3.14;
function add(a, b) { return a + b; }
export { PI, add };
```

#### 2.2 默认导出（只能有一个）

```javascript
// string.js
const StringUtils = {
    capitalize(str) { /* ... */ },
    reverse(str) { /* ... */ }
};

export default StringUtils;
```

#### 2.3 混合导出

```javascript
// user.js
export const USER_ROLES = { /* ... */ };  // 命名导出
export default class User { /* ... */ }    // 默认导出
```

### 3. 导入方式（Import）

#### 3.1 导入命名导出

```javascript
// 导入单个
import { add } from './math.js';

// 导入多个
import { add, subtract, multiply } from './math.js';

// 重命名
import { add as mathAdd } from './math.js';
```

#### 3.2 导入默认导出

```javascript
// 可以自定义名称
import StringUtils from './string.js';
import MyStringUtils from './string.js';  // 都可以
```

#### 3.3 混合导入

```javascript
// 同时导入默认和命名
import User, { USER_ROLES, USER_STATUS } from './user.js';
//     ↑默认         ↑命名导出
```

#### 3.4 全部导入

```javascript
// 导入所有导出到命名空间
import * as MathModule from './math.js';

// 使用
MathModule.add(1, 2);
MathModule.PI;
```

### 4. 重要规则

| 规则 | 说明 |
|------|------|
| **文件扩展名** | 必须写`.js`（不能省略） |
| **路径** | 相对路径必须有`./`或`../` |
| **type="module"** | HTML中必须指定`<script type="module">` |
| **严格模式** | 模块代码自动运行在严格模式 |
| **作用域** | 每个模块有独立作用域 |
| **this** | 模块顶层的`this`是`undefined` |
| **运行环境** | 必须在服务器环境（不能`file://`） |

---

## 💡 ES6模块 vs CommonJS

| 特性 | ES6模块 | CommonJS |
|------|---------|----------|
| **语法** | `import/export` | `require/module.exports` |
| **环境** | 浏览器 + Node.js | Node.js |
| **加载** | 静态（编译时） | 动态（运行时） |
| **this** | `undefined` | `module` |
| **循环依赖** | 支持 | 支持但有限制 |
| **Tree Shaking** | 支持 | 不支持 |

---

## 🎯 学习路径

### 第一步：理解基础概念
1. 阅读`index.html`了解模块化概念
2. 查看`modules/math.js`学习命名导出
3. 查看`modules/string.js`学习默认导出
4. 查看`modules/user.js`学习混合导出

### 第二步：学习导入方式
5. 阅读`main.js`了解各种导入方式
6. 在浏览器中运行，查看控制台输出
7. 修改代码，实验不同的导入方式

### 第三步：实战练习
8. 尝试创建自己的模块
9. 重构之前的Todo应用（模块化）
10. 完成练习题

---

## 📝 练习题

### 练习1：创建工具模块

创建`modules/array.js`，导出数组工具函数：
- `shuffle(arr)` - 打乱数组
- `unique(arr)` - 数组去重
- `flatten(arr)` - 数组扁平化
- `chunk(arr, size)` - 数组分组

在`main.js`中导入并测试。

### 练习2：模块化重构Todo应用

将之前的Todo应用重构为模块化结构：

```
todo_app_modular/
├── index.html
├── main.js
├── modules/
│   ├── storage.js      # 数据持久化
│   ├── todoApp.js      # 应用逻辑
│   └── utils.js        # 工具函数
└── style.css
```

**要求：**
- `storage.js` - 导出Storage类（默认导出）
- `todoApp.js` - 导出TodoApp类（默认导出）
- `utils.js` - 导出工具函数（命名导出）
- `main.js` - 导入所有模块并初始化应用

### 练习3：创建API模块

创建`modules/api.js`，封装HTTP请求：

```javascript
// api.js
export async function get(url) { /* ... */ }
export async function post(url, data) { /* ... */ }
export async function put(url, data) { /* ... */ }
export async function del(url) { /* ... */ }

// 或者默认导出
export default {
    get, post, put, del
};
```

---

## ⚠️ 常见错误

### 错误1：忘记写文件扩展名

```javascript
❌ import { add } from './math';
✅ import { add } from './math.js';
```

### 错误2：忘记写相对路径

```javascript
❌ import { add } from 'math.js';
✅ import { add } from './math.js';
```

### 错误3：没有使用服务器环境

```
❌ file:///C:/Users/.../index.html
   报错：CORS error

✅ http://localhost:8000/index.html
   正常运行
```

### 错误4：HTML中忘记type="module"

```html
❌ <script src="main.js"></script>
✅ <script type="module" src="main.js"></script>
```

### 错误5：混淆命名导出和默认导出

```javascript
// math.js
export const add = (a, b) => a + b;

❌ import add from './math.js';        // 错误：add是命名导出
✅ import { add } from './math.js';    // 正确
```

---

## 🎓 进阶知识

### 1. 动态导入

```javascript
// 异步加载模块
const module = await import('./math.js');
module.add(1, 2);

// 或者
import('./math.js').then(module => {
    module.add(1, 2);
});
```

### 2. 重导出

```javascript
// utils/index.js
export { add, subtract } from './math.js';
export { capitalize } from './string.js';
export * from './array.js';
```

### 3. 只执行模块（不导入）

```javascript
// 只执行模块代码，不导入任何内容
import './config.js';
```

---

## 📚 参考资源

- [MDN - JavaScript Modules](https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Guide/Modules)
- [ES6 入门教程 - Module](https://es6.ruanyifeng.com/#docs/module)

---

## ✅ 学习检查清单

完成以下检查项，确保掌握ES6模块化：

- [ ] 能解释模块化的概念和优点
- [ ] 能写出命名导出的代码
- [ ] 能写出默认导出的代码
- [ ] 能写出混合导出的代码
- [ ] 能正确导入命名导出
- [ ] 能正确导入默认导出
- [ ] 能同时导入命名和默认导出
- [ ] 能使用import * as语法
- [ ] 能使用as关键字重命名
- [ ] 理解模块必须在服务器环境运行
- [ ] 能成功启动本地服务器
- [ ] 完成练习1（创建工具模块）
- [ ] 完成练习2（重构Todo应用）
- [ ] 理解ES6模块和CommonJS的区别

---

**学习时间**：2026-06-22
**难度**：⭐⭐⭐⭐ 中高级
**预计时长**：45分钟
