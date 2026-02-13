// ============================================
// JavaScript模板字符串 - 超详细注释版
// 日期：2026-02-12（第1天）
// 难度：初级
// ============================================

/*
【核心概念】模板字符串

是什么？
- 使用反引号（`）包裹的字符串，支持变量插值和多行文本
- ES6新增特性，React开发中大量使用

为什么需要？
- 传统字符串拼接繁琐易错
- 模板字符串让代码更简洁易读
- 支持多行文本，不需要\n

核心特点？
- 使用反引号（`）而不是引号
- 使用${}插入变量和表达式
- 支持多行文本
- 可以嵌套使用
*/

// ============================================
// 第一部分：核心概念详解
// ============================================

/*
【概念1：什么是模板字符串？】

是什么？
- 使用反引号（`）包裹的字符串
- 可以在字符串中插入变量和表达式
- 就像填空题，把变量"填"到字符串中

为什么需要？
- 传统拼接：const msg = "Hello, " + name + "!";（繁琐）
- 模板字符串：const msg = `Hello, ${name}!`;（简洁）

核心特点：
- 使用反引号（键盘ESC下方的键）
- 使用${}插入变量
- 支持多行文本
- 可以插入任何表达式
*/

// --------------------------------------------
// 1.1 模板字符串语法
// --------------------------------------------

/*
【API卡片】

名称：模板字符串（Template Literals）
作用：创建可以插入变量的字符串
类型：ES6语法特性

完整语法：
`文本内容 ${变量或表达式} 文本内容`

参数详解：
1. 反引号（`）
   - 作用：标记模板字符串的开始和结束
   - 位置：键盘ESC下方，数字1左边
   - 注意：不是单引号（'）

2. ${}
   - 作用：插入变量或表达式
   - 内容：可以是变量、表达式、函数调用
   - 结果：自动转为字符串

使用示例：
```jsx
// 基本用法
const name = "张三";
const greeting = `你好，${name}！`;

// 插入表达式
const price = 99;
const message = `总价：${price * 2}元`;

// 插入函数调用
const text = `当前时间：${new Date().toLocaleString()}`;
```

注意事项：
- 反引号不是单引号
- ${}内可以是任何JavaScript表达式
- 结果会自动转为字符串
*/

// --------------------------------------------
// 1.2 传统字符串拼接 vs 模板字符串
// --------------------------------------------

/*
【对比】

传统方式（不推荐）：
```jsx
const name = "李四";
const age = 25;
const message = "我叫" + name + "，今年" + age + "岁";
```

问题：
- 需要多个+号
- 容易漏掉空格
- 代码难读

模板字符串（推荐）：
```jsx
const name = "李四";
const age = 25;
const message = `我叫${name}，今年${age}岁`;
```

优点：
- 代码简洁
- 不容易出错
- 易读易维护
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 基本变量插值
// --------------------------------------------

// 声明变量
const userName = "王五";
const userAge = 30;
const userCity = "北京";

// 使用模板字符串插入变量
// ${}会自动将变量转为字符串并插入
const introduction = `大家好，我叫${userName}，今年${userAge}岁，来自${userCity}。`;

console.log(introduction);
// 输出：大家好，我叫王五，今年30岁，来自北京。

// 对比传统拼接方式（不推荐）
const oldWay = "大家好，我叫" + userName + "，今年" + userAge + "岁，来自" + userCity + "。";
console.log("传统方式：", oldWay);

console.log("=".repeat(60));

// --------------------------------------------
// 2.2 插入表达式
// --------------------------------------------

// 模板字符串中可以插入任何表达式
const price = 99.99;
const quantity = 3;

// 插入数学运算
const total = `总价：${price * quantity}元`;
console.log(total);  // 总价：299.97元

// 插入比较运算
const age = 18;
const status = `状态：${age >= 18 ? "成年" : "未成年"}`;
console.log(status);  // 状态：成年

// 插入函数调用
const upperName = `大写名字：${userName.toUpperCase()}`;
console.log(upperName);  // 大写名字：王五

// 插入对象属性
const user = {
  name: "赵六",
  email: "zhao@example.com"
};
const userInfo = `用户：${user.name}，邮箱：${user.email}`;
console.log(userInfo);

console.log("=".repeat(60));

// --------------------------------------------
// 2.3 多行文本
// --------------------------------------------

// 传统方式：需要使用\n换行（不推荐）
const oldMultiLine = "第一行\n第二行\n第三行";
console.log("传统多行：");
console.log(oldMultiLine);

// 模板字符串：直接换行（推荐）
const newMultiLine = `第一行
第二行
第三行`;
console.log("模板字符串多行：");
console.log(newMultiLine);

// 实际应用：HTML模板
const htmlTemplate = `
  <div class="card">
    <h2>${userName}</h2>
    <p>年龄：${userAge}</p>
    <p>城市：${userCity}</p>
  </div>
`;
console.log("HTML模板：");
console.log(htmlTemplate);

console.log("=".repeat(60));

// --------------------------------------------
// 2.4 嵌套使用
// --------------------------------------------

// 模板字符串可以嵌套
const firstName = "张";
const lastName = "三";

// 在模板字符串中嵌套另一个模板字符串
const fullName = `${firstName}${lastName}`;
const greeting = `欢迎，${`尊敬的${fullName}先生`}！`;
console.log(greeting);  // 欢迎，尊敬的张三先生！

// 实际应用：动态生成消息
const isVIP = true;
const welcomeMsg = `您好，${isVIP ? `VIP会员${fullName}` : fullName}`;
console.log(welcomeMsg);

console.log("=".repeat(60));

// --------------------------------------------
// 2.5 实际应用场景
// --------------------------------------------

// 场景1：用户信息展示
const student = {
  name: "李明",
  grade: 85,
  subject: "数学"
};

const report = `
学生成绩单
-----------
姓名：${student.name}
科目：${student.subject}
成绩：${student.grade}分
等级：${student.grade >= 90 ? "优秀" : student.grade >= 80 ? "良好" : "及格"}
`;
console.log(report);

// 场景2：购物车总价计算
const cart = [
  { name: "苹果", price: 5, quantity: 3 },
  { name: "香蕉", price: 3, quantity: 5 },
  { name: "橙子", price: 4, quantity: 2 }
];

// 计算总价
const totalPrice = cart.reduce((sum, item) => sum + item.price * item.quantity, 0);

const cartSummary = `
购物车清单
-----------
${cart.map(item => `${item.name}：${item.price}元 x ${item.quantity} = ${item.price * item.quantity}元`).join('\n')}
-----------
总计：${totalPrice}元
`;
console.log(cartSummary);

// 场景3：API URL构建
const apiBase = "https://api.example.com";
const userId = 123;
const apiUrl = `${apiBase}/users/${userId}/profile`;
console.log("API地址：", apiUrl);

// 场景4：日志消息
const logLevel = "ERROR";
const errorMsg = "文件未找到";
const timestamp = new Date().toLocaleString();
const logMessage = `[${timestamp}] [${logLevel}] ${errorMsg}`;
console.log(logMessage);

console.log("=".repeat(60));

/*
==============================================
深入理解：模板字符串的原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【深入内容】

1. 模板字符串是如何工作的？
   - JavaScript引擎会解析模板字符串
   - 将${}中的表达式计算出结果
   - 将结果转为字符串
   - 拼接所有部分

   例子：
   ```jsx
   const name = "Tom";
   const age = 25;
   const msg = `Hello, ${name}, you are ${age} years old`;

   // 等价于：
   const msg2 = "Hello, " + name + ", you are " + age + " years old";
   ```

2. 标签模板（Tagged Templates）
   高级用法，可以自定义模板字符串的处理方式

   ```jsx
   function highlight(strings, ...values) {
     return strings.reduce((result, str, i) => {
       return result + str + (values[i] ? `<strong>${values[i]}</strong>` : '');
     }, '');
   }

   const name = "Tom";
   const html = highlight`Hello, ${name}!`;
   // 结果：Hello, <strong>Tom</strong>!
   ```

3. 为什么React中大量使用模板字符串？
   - JSX中动态插入数据
   - 构建className
   - 生成动态样式
   - API请求URL

   例子：
   ```jsx
   // 动态className
   const isActive = true;
   <div className={`button ${isActive ? 'active' : ''}`}>

   // 动态样式
   const color = 'red';
   <div style={{ color: `${color}` }}>

   // API请求
   fetch(`/api/users/${userId}`)
   ```

4. 性能考虑
   - 模板字符串和传统拼接性能相近
   - 现代JavaScript引擎优化很好
   - 优先考虑代码可读性，不用担心性能

==============================================
*/

// ============================================
// 第三部分：常见错误和陷阱
// ============================================

/*
3.1 错误1：使用单引号而不是反引号

错误代码：
```jsx
const name = "Tom";
const msg = 'Hello, ${name}!';  // ❌ 使用了单引号
console.log(msg);  // 输出：Hello, ${name}!（没有替换）
```

结果：变量没有被替换，输出原始字符串

原因：单引号（'）和双引号（"）是普通字符串，不支持变量插值

正确代码：
```jsx
const name = "Tom";
const msg = `Hello, ${name}!`;  // ✅ 使用反引号
console.log(msg);  // 输出：Hello, Tom!
```

记忆技巧：模板字符串必须用反引号（`），不是单引号（'）
*/

/*
3.2 错误2：忘记使用${}

错误代码：
```jsx
const age = 25;
const msg = `I am age years old`;  // ❌ 忘记${}
console.log(msg);  // 输出：I am age years old
```

结果：age被当作普通文本，没有替换为变量值

原因：没有使用${}标记变量

正确代码：
```jsx
const age = 25;
const msg = `I am ${age} years old`;  // ✅ 使用${}
console.log(msg);  // 输出：I am 25 years old
```

记忆技巧：插入变量必须用${}包裹
*/

/*
3.3 错误3：${}中使用语句而不是表达式

错误代码：
```jsx
const age = 25;
const msg = `Status: ${if (age >= 18) { "adult" }}`;  // ❌ 语法错误
```

结果：语法错误，代码无法运行

原因：${}中只能是表达式，不能是语句（if、for等）

正确代码：
```jsx
const age = 25;
const msg = `Status: ${age >= 18 ? "adult" : "minor"}`;  // ✅ 使用三元运算符
```

记忆技巧：${}中用三元运算符，不用if语句
*/

/*
3.4 错误4：多行文本缩进问题

错误代码：
```jsx
const html = `
    <div>
        <h1>Title</h1>
    </div>
`;
console.log(html);
// 输出包含前导空格，可能影响显示
```

问题：缩进的空格也会被包含在字符串中

解决方案1：不缩进（不美观）
```jsx
const html = `
<div>
<h1>Title</h1>
</div>
`;
```

解决方案2：使用trim()去除首尾空白
```jsx
const html = `
    <div>
        <h1>Title</h1>
    </div>
`.trim();
```

记忆技巧：多行文本注意缩进，必要时使用trim()
*/

/*
3.5 错误5：在${}中使用undefined或null

错误代码：
```jsx
let name;  // undefined
const msg = `Hello, ${name}!`;
console.log(msg);  // 输出：Hello, undefined!
```

问题：undefined和null会被转为字符串"undefined"和"null"

正确代码：
```jsx
let name;
const msg = `Hello, ${name || "Guest"}!`;  // 使用默认值
console.log(msg);  // 输出：Hello, Guest!
```

记忆技巧：使用||提供默认值，避免显示undefined
*/

/*
3.6 错误6：忘记转义反引号

错误代码：
```jsx
const code = `const msg = `Hello`;`;  // ❌ 语法错误
```

结果：语法错误，反引号提前结束

原因：反引号没有转义

正确代码：
```jsx
const code = `const msg = \`Hello\`;`;  // ✅ 使用\转义
console.log(code);  // 输出：const msg = `Hello`;
```

记忆技巧：模板字符串中的反引号要用\转义
*/

/*
3.7 错误7：过度嵌套导致难读

错误代码：
```jsx
const msg = `Hello, ${user ? `${user.name ? `Mr. ${user.name}` : 'Guest'}` : 'Anonymous'}`;
// 太复杂，难以理解
```

问题：嵌套太深，代码难读

正确代码：
```jsx
// 先计算中间值
const displayName = user?.name ? `Mr. ${user.name}` : 'Guest';
const finalName = user ? displayName : 'Anonymous';
const msg = `Hello, ${finalName}`;
```

记忆技巧：嵌套超过2层就拆分成多个变量
*/

// ============================================
// 第四部分：模板字符串最佳实践
// ============================================

// 最佳实践1：使用默认值避免undefined
const userName2 = undefined;
const greeting2 = `欢迎，${userName2 || "访客"}！`;
console.log(greeting2);  // 欢迎，访客！

// 最佳实践2：复杂逻辑提取到变量
const score = 85;
const level = score >= 90 ? "优秀" : score >= 80 ? "良好" : "及格";
const result = `成绩：${score}分，等级：${level}`;
console.log(result);

// 最佳实践3：使用trim()处理多行文本
const cleanHtml = `
  <div>
    <h1>标题</h1>
  </div>
`.trim();
console.log(cleanHtml);

// 最佳实践4：构建URL时注意编码
const searchTerm = "React 教程";
const encodedTerm = encodeURIComponent(searchTerm);
const searchUrl = `https://example.com/search?q=${encodedTerm}`;
console.log(searchUrl);

console.log("=".repeat(60));

// ============================================
// 第五部分：模板字符串速查表
// ============================================

/*
【基本语法】
`文本 ${变量} 文本`

【常用场景】
1. 变量插值：`Hello, ${name}!`
2. 表达式计算：`Total: ${price * quantity}`
3. 三元运算：`Status: ${isActive ? 'Active' : 'Inactive'}`
4. 函数调用：`Upper: ${name.toUpperCase()}`
5. 对象属性：`Email: ${user.email}`
6. 多行文本：
   ```
   `第一行
   第二行`
   ```

【注意事项】
- 必须使用反引号（`），不是单引号（'）
- 变量必须用${}包裹
- ${}中只能是表达式，不能是语句
- undefined和null会转为字符串
- 反引号需要转义：\`

【React中的应用】
- 动态className：`button ${isActive ? 'active' : ''}`
- API URL：`/api/users/${userId}`
- 内联样式：`${size}px`
- 条件渲染：`${count > 0 ? count : ''}`
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习1：个人信息卡片
要求：
  - 声明变量：姓名、年龄、职业、城市
  - 使用模板字符串生成个人信息卡片（多行）
  - 格式：
    ```
    ============
    个人信息
    ============
    姓名：XXX
    年龄：XX岁
    职业：XXX
    城市：XXX
    ============
    ```
提示：使用多行模板字符串

练习2：购物清单
要求：
  - 声明3个商品对象（名称、单价、数量）
  - 使用模板字符串生成购物清单
  - 计算每个商品的小计和总计
  - 格式：
    ```
    商品1：单价 x 数量 = 小计
    商品2：单价 x 数量 = 小计
    商品3：单价 x 数量 = 小计
    总计：XXX元
    ```
提示：使用表达式计算小计

练习3：动态问候语
要求：
  - 声明变量：用户名、当前小时（0-23）
  - 根据时间生成不同的问候语：
    * 0-11：早上好
    * 12-17：下午好
    * 18-23：晚上好
  - 格式：`XXX好，${用户名}！`
提示：使用三元运算符或条件表达式

练习4：URL构建器
要求：
  - 声明变量：域名、路径、用户ID、查询参数
  - 使用模板字符串构建完整URL
  - 格式：`https://${域名}/${路径}/${用户ID}?参数=值`
  - 例如：https://api.example.com/users/123?lang=zh
提示：注意URL的结构

练习5：找出错误
要求：
  - 分析以下代码，找出所有错误：
  ```jsx
  const name = "Tom";
  const age = 25;

  // 错误1
  const msg1 = 'Hello, ${name}!';

  // 错误2
  const msg2 = `I am age years old`;

  // 错误3
  const msg3 = `Status: ${if (age >= 18) { "adult" }}`;

  // 错误4
  let city;
  const msg4 = `City: ${city}`;

  console.log(msg1, msg2, msg3, msg4);
  ```
提示：引号类型、${}、语句vs表达式、undefined处理
*/

// ============================================
// 总结
// ============================================

/*
【今日重点】

1. 模板字符串使用反引号（`），不是单引号（'）
2. 使用${}插入变量和表达式
3. 支持多行文本，不需要\n
4. ${}中只能是表达式，不能是语句
5. 使用||提供默认值，避免显示undefined

【React中的重要性】
- JSX中动态插入数据
- 构建动态className
- 生成API URL
- 条件渲染

【下一步】
完成第一个综合练习（04_practice_hello.jsx）
*/
