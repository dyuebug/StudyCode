# JavaScript 高级特性（web_basics）

> 对应第 47 天学习材料。**材料已生成，但学习尚未开始**（任务勾选 0/10，无日志）。

## 内容

| 文件 | 主题 | 行数 |
|---|---|---|
| `javascript_advanced/01_closure_详解.html` | 闭包、作用域链、私有数据、循环陷阱、防抖节流 | 631 |
| `javascript_advanced/02_prototype_详解.html` | 原型链、继承模式、`instanceof` | 552 |
| `javascript_advanced/03_this_binding_详解.html` | 四种绑定规则、箭头函数、`call/apply/bind` | 572 |
| `javascript_advanced/04_es6_modules_详解/` | ES Module、import/export、模块拆分 | 目录 |

## 打开方式

前三个文件可直接双击打开。

`04_es6_modules_详解` 使用 ES Module，必须通过 HTTP 访问：

```bash
python -m http.server 8080 --directory web_basics/javascript_advanced/04_es6_modules_详解
```

访问 <http://127.0.0.1:8080>。

## 学习路径

1. 闭包 → 2. 原型链 → 3. `this` 绑定 → 4. ES6 模块。
2. 每份材料约 45 分钟，先读概念，再运行示例，最后做练习。
3. 完成后在 `daily_logs/` 写第 47 天日志，记录实际时长与问题。

## 验收

- [ ] 能解释闭包与内存泄漏的关系。
- [ ] 能画出原型链并说明查找过程。
- [ ] 能判断任意场景下 `this` 的指向。
- [ ] 能用 `import/export` 重构 Todo App 的部分代码。
