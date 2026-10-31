# 阶段 3：JavaScript 进阶知识笔记

> **整理日期**：2026-10-29
> **数据来源**：第 43～47 天学习材料与日志；`web_basics/javascript_advanced/` 下的四个专题页面；`web_projects/todo_app/` 现有代码。
> **状态说明**：第 47 天材料已生成，但任务勾选为 0/10、无完成日志。本笔记属于**学习材料**，不是“已掌握”的证明。文中示例可在浏览器控制台直接运行验证。

---

## 1. 闭包（Closure）

### 1.1 是什么

闭包是**函数与其词法环境的组合**。内部函数引用了外部函数的变量，即使外部函数已经返回，这些变量仍然保留在内存中。

```js
function createCounter() {
  let count = 0;                 // 被闭包捕获
  return {
    increment() { count += 1; return count; },
    current() { return count; },
  };
}

const counter = createCounter();
counter.increment(); // 1
counter.increment(); // 2
counter.current();   // 2
```

关键点：

- 捕获的是**变量本身**，不是创建闭包时的值。
- 每次调用 `createCounter()` 都会产生一套独立的作用域，互不影响。
- 闭包延长了变量的生命周期，因此滥用会增加内存占用。

### 1.2 常见用途

| 用途 | 说明 | 对应示例 |
|---|---|---|
| 私有状态 | 外部无法直接改 `count`，只能通过方法访问 | `createCounter` |
| 函数工厂 | 用参数生成行为不同但结构相同的函数 | `makeMultiplier(n)` |
| 防抖 / 节流 | 用闭包保存定时器或上次执行时间 | 搜索框输入、滚动监听 |
| 缓存（memoize） | 闭包保存 `Map`，避免重复计算 | 斐波那契、昂贵格式化 |
| 模块模式 | IIFE 内定义变量，只导出公开接口 | 老式模块封装 |

防抖最小实现：

```js
function debounce(fn, delay) {
  let timerId = null;
  return function (...args) {
    clearTimeout(timerId);
    timerId = setTimeout(() => fn.apply(this, args), delay);
  };
}
```

注意 `fn.apply(this, args)`：保留调用时的 `this`，否则在对象方法上使用会丢失上下文。

### 1.3 经典陷阱

**循环中的 `var`**：

```js
for (var i = 0; i < 3; i++) {
  setTimeout(() => console.log(i), 0); // 3 3 3
}
```

原因：`var` 是函数作用域，三个回调共享同一个 `i`，循环结束时 `i === 3`。

修复方式：

```js
for (let i = 0; i < 3; i++) {
  setTimeout(() => console.log(i), 0); // 0 1 2
}
```

`let` 为每次迭代创建新的绑定。

**内存泄漏**：闭包持有大对象或 DOM 引用时，只要闭包存活，对象就无法回收。处理方式：不再需要时解除引用，或把缓存限制在有限范围内。

**`this` 丢失**：把对象方法作为回调传递时，方法内的 `this` 不再指向原对象。可用箭头函数、`bind` 或外层保存 `const self = this` 解决。

### 1.4 验证方式

打开 `web_basics/javascript_advanced/01_closure_详解.html`，在控制台依次运行计数器、循环、防抖示例，观察输出顺序。

---

## 2. 原型链（Prototype Chain）

### 2.1 三个核心对象

```js
function Animal(name) {
  this.name = name;
}
Animal.prototype.speak = function () {
  return `${this.name} makes a sound`;
};

const cat = new Animal('Tom');
```

| 表达式 | 结果 | 含义 |
|---|---|---|
| `cat.__proto__` | `Animal.prototype` | 实例的原型（旧访问器） |
| `Object.getPrototypeOf(cat)` | `Animal.prototype` | 推荐写法 |
| `Animal.prototype.constructor` | `Animal` | 原型指回构造函数 |
| `cat instanceof Animal` | `true` | 沿原型链查找 |

### 2.2 属性查找过程

访问 `cat.speak` 时：

1. 查 `cat` 自身属性 → 没有。
2. 查 `Object.getPrototypeOf(cat)`，即 `Animal.prototype` → 找到 `speak`，返回。
3. 如果还没找到，继续查 `Animal.prototype` 的原型，直到 `Object.prototype`。
4. 到 `null` 仍没有 → `undefined`。

原型链的终点是 `Object.prototype`，它的原型是 `null`。

### 2.3 继承方式

**ES5 组合继承**：

```js
function Dog(name) {
  Animal.call(this, name); // 继承实例属性
}
Dog.prototype = Object.create(Animal.prototype);
Dog.prototype.constructor = Dog;
Dog.prototype.bark = function () { return 'woof'; };
```

**ES6 class**：

```js
class Dog extends Animal {
  constructor(name) {
    super(name);
  }
  bark() { return 'woof'; }
}
```

`extends` 的本质仍是原型链，`super()` 调用父类构造函数。class 不会提升，必须先定义后使用。

### 2.4 手写 `instanceof`

```js
function myInstanceof(obj, Ctor) {
  let proto = Object.getPrototypeOf(obj);
  while (proto) {
    if (proto === Ctor.prototype) return true;
    proto = Object.getPrototypeOf(proto);
  }
  return false;
}
```

### 2.5 常见误区

- `Animal.prototype = { ... }` 会**整体替换**原型对象，导致 `constructor` 丢失；需要手动补回。
- 在构造函数内定义方法（`this.speak = function(){}`）会让每个实例各有一份，浪费内存；应放到 `prototype` 上。
- `__proto__` 是访问器，`prototype` 是函数才有的属性，二者不是一回事。

### 2.6 验证方式

打开 `web_basics/javascript_advanced/02_prototype_详解.html`，用页面提供的链路可视化工具逐层打印 `Object.getPrototypeOf`。

---

## 3. `this` 绑定

### 3.1 四条规则与优先级

从高到低：

1. **`new` 绑定**：`new Foo()` 中，`this` 指向新创建的对象。
2. **显式绑定**：`fn.call(obj)`、`fn.apply(obj)`、`fn.bind(obj)`。
3. **隐式绑定**：`obj.method()` 中，`this` 指向 `obj`。
4. **默认绑定**：普通函数调用 `fn()`，非严格模式指向全局对象，严格模式为 `undefined`。

箭头函数不适用以上规则，它的 `this` 来自**定义时所在的词法作用域**，且无法被 `call/apply/bind` 改变。

```js
const obj = {
  name: 'obj',
  regular() { return this.name; },
  arrow: () => this?.name,
};
console.log(obj.regular()); // 'obj'
console.log(obj.arrow());   // undefined（模块/严格模式下）
```

### 3.2 常见丢失场景

```js
const user = {
  name: 'Alice',
  greet() { return `Hi, ${this.name}`; },
};

const greet = user.greet;
greet();              // 报错或 undefined
setTimeout(user.greet, 0); // this 丢失

const bound = user.greet.bind(user);
bound();              // 'Hi, Alice'
```

修复方式：

- `bind`：生成固定 `this` 的新函数。
- 箭头函数包装：`setTimeout(() => user.greet(), 0)`。
- 类字段箭头函数：`greet = () => this.name`（每次实例化创建一份）。

### 3.3 `call` / `apply` / `bind` 区别

| 方法 | 是否立即执行 | 参数形式 | 返回值 |
|---|---|---|---|
| `call` | 是 | 逐个传参 | 函数返回值 |
| `apply` | 是 | 数组传参 | 函数返回值 |
| `bind` | 否 | 逐个传参 | 绑定后的新函数 |

`bind` 还可以预置参数（偏函数）：

```js
function add(a, b) { return a + b; }
const add10 = add.bind(null, 10);
add10(5); // 15
```

### 3.4 验证方式

打开 `web_basics/javascript_advanced/03_this_binding_详解.html`，按页面中的优先级实验逐项运行，先预测输出再执行。

---

## 4. ES6 模块（ES Module）

### 4.1 基本语法

```js
// math.js
export const PI = 3.14159;
export function add(a, b) { return a + b; }
export default function multiply(a, b) { return a * b; }
```

```js
// main.js
import multiply, { PI, add } from './math.js';
import * as math from './math.js';

console.log(add(PI, 1), multiply(2, 3), math.PI);
```

规则：

- 一个模块只能有一个 `default` 导出。
- 命名导出可以有多个，导入时名字必须匹配（可用 `as` 重命名）。
- `import` 必须写在模块顶层，不能放在条件语句中。
- 静态导入会被提升，模块只执行一次。

### 4.2 浏览器运行条件

```html
<script type="module" src="./main.js"></script>
```

必须通过 HTTP 访问，不能直接双击 `file://` 打开，否则会因 CORS/协议限制报错。

```bash
python -m http.server 8080 --directory web_basics/javascript_advanced/04_es6_modules_详解
```

### 4.3 与 CommonJS 的差异

| 维度 | ES Module | CommonJS |
|---|---|---|
| 语法 | `import` / `export` | `require` / `module.exports` |
| 加载时机 | 静态分析，编译期确定 | 运行时同步加载 |
| 导出值 | 实时绑定（live binding） | 值的拷贝 |
| 使用环境 | 浏览器、现代 Node | 传统 Node |
| 顶层 `this` | `undefined` | `module.exports` |

### 4.4 模块化重构 Todo App 的切入点

当前 `web_projects/todo_app/storage.js` 与 `app.js` 都是顶层 `class`，通过普通 `<script>` 依次加载，依赖全局顺序。可拆为：

```
todo_app/
├── src/
│   ├── storage.js     export class Storage
│   ├── todo-app.js    export class TodoApp
│   └── main.js        import + 初始化
└── index.html         <script type="module" src="./src/main.js">
```

验收标准：页面功能不变，控制台无 `ReferenceError`，LocalStorage 数据仍能读取。

---

## 5. 自测题（不提供答案）

1. 下面的输出是什么？为什么？
   ```js
   function outer() {
     var result = [];
     for (var i = 0; i < 3; i++) result.push(() => i);
     return result;
   }
   console.log(outer().map(fn => fn()));
   ```
2. 手写 `myInstanceof`，并说明它为什么不能识别 `null`。
3. 判断以下代码的 `this`：
   ```js
   const obj = { name: 'A', fn() { return this.name; } };
   const f = obj.fn;
   console.log(f.call({ name: 'B' }));
   ```
4. 为什么 `import` 必须写在模块顶层？动态导入用什么语法？
5. 把 `debounce` 改造成支持 `immediate` 首次立即执行的版本。

---

## 6. 对应文件

| 主题 | 文件 |
|---|---|
| 闭包 | `web_basics/javascript_advanced/01_closure_详解.html` |
| 原型链 | `web_basics/javascript_advanced/02_prototype_详解.html` |
| `this` 绑定 | `web_basics/javascript_advanced/03_this_binding_详解.html` |
| ES6 模块 | `web_basics/javascript_advanced/04_es6_modules_详解/` |
| 现有前端项目 | `web_projects/todo_app/` |
| 第 47 天任务 | `daily_tasks/2026-06-22_第47天_任务.md` |

---

## 7. 待验证内容

- 本文示例未逐条写入自动化测试；第 47 天学习时应至少在控制台运行一遍。
- Todo App 的模块化改造属于建议方案，尚未实施。
