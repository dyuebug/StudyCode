// ============================================
// JavaScript数据类型 - 超详细注释版
// 日期：2026-02-12（第1天）
// 难度：初级
// ============================================

/*
【核心概念】JavaScript数据类型

是什么？
- 数据类型定义了数据的种类和可以进行的操作
- JavaScript有7种基本数据类型（今天学5种最常用的）

为什么需要？
- 不同类型的数据有不同的用途和操作方式
- 类型系统帮助我们正确处理数据

核心特点？
- JavaScript是动态类型语言（变量类型可以改变）
- 5种基本类型：string、number、boolean、null、undefined
- 可以使用typeof检测类型
*/

// ============================================
// 第一部分：核心概念详解
// ============================================

/*
【概念1：什么是数据类型？】

是什么？
- 数据类型是数据的分类，决定了数据能做什么操作
- 就像物品的分类：水果可以吃，书可以读

为什么需要？
- 不同类型有不同的操作（数字可以加减，字符串可以拼接）
- 类型帮助我们避免错误（不能把字符串当数字相加）

核心特点：
- JavaScript有7种基本类型（今天学5种）
- 变量的类型可以改变（动态类型）
- 使用typeof可以检测类型
*/

// --------------------------------------------
// 1.1 五种基本数据类型
// --------------------------------------------

/*
【类型1：String（字符串）】

是什么？
- 文本数据，用引号包裹

语法：
const str1 = "双引号字符串";
const str2 = '单引号字符串';
const str3 = `反引号字符串（模板字符串）`;

特点：
- 可以用单引号、双引号、反引号
- 反引号支持变量插值和多行
- 字符串是不可变的（修改会创建新字符串）

常用操作：
- 拼接：str1 + str2
- 长度：str.length
- 大小写：str.toUpperCase()、str.toLowerCase()

示例：
```jsx
const name = "张三";
const greeting = `你好，${name}！`;  // 模板字符串
console.log(greeting);  // 输出：你好，张三！
```
*/

/*
【类型2：Number（数字）】

是什么？
- 数值数据，包括整数和小数

语法：
const num1 = 42;        // 整数
const num2 = 3.14;      // 小数
const num3 = -10;       // 负数
const num4 = 1e6;       // 科学计数法（1000000）

特点：
- 不区分整数和浮点数
- 有特殊值：Infinity（无穷大）、NaN（非数字）
- 精度有限（小数计算可能不精确）

常用操作：
- 四则运算：+、-、*、/
- 取余：%
- 幂运算：**

示例：
```jsx
const price = 99.99;
const quantity = 3;
const total = price * quantity;  // 299.97
```
*/

/*
【类型3：Boolean（布尔值）】

是什么？
- 逻辑值，只有两个值：true或false

语法：
const isTrue = true;
const isFalse = false;

特点：
- 只有两个值：true、false
- 用于条件判断
- 很多值可以转换为布尔值

常用场景：
- 条件判断：if (isLoggedIn) { ... }
- 开关状态：isVisible、isActive
- 权限控制：hasPermission、canEdit

示例：
```jsx
const isLoggedIn = true;
const isAdmin = false;

if (isLoggedIn) {
  console.log("欢迎回来！");
}
```
*/

/*
【类型4：Null（空值）】

是什么？
- 表示"故意设置为空"的值

语法：
const value = null;

特点：
- 只有一个值：null
- 表示"没有值"或"空对象"
- 是一个关键字（不能作为变量名）

使用场景：
- 初始化对象变量：let user = null;
- 清空变量：user = null;
- API返回空值

注意：
- typeof null 返回 "object"（这是JavaScript的历史bug）

示例：
```jsx
let selectedUser = null;  // 初始化为空

// 用户选择后赋值
selectedUser = { name: "张三", age: 25 };

// 取消选择
selectedUser = null;
```
*/

/*
【类型5：Undefined（未定义）】

是什么？
- 表示"变量已声明但未赋值"

语法：
let value;  // 自动为undefined

特点：
- 只有一个值：undefined
- 变量声明但未赋值时的默认值
- 函数没有返回值时返回undefined

使用场景：
- 检测变量是否赋值
- 函数参数未传递时的默认值

与null的区别：
- undefined：系统自动设置（未赋值）
- null：程序员主动设置（故意为空）

示例：
```jsx
let name;
console.log(name);  // undefined

function greet() {
  // 没有return语句
}
console.log(greet());  // undefined
```
*/

// --------------------------------------------
// 1.2 类型检测API
// --------------------------------------------

/*
【API卡片】

名称：typeof
作用：检测数据类型
类型：运算符

完整语法：
typeof 值
typeof(值)  // 也可以加括号

返回值：
- 字符串，表示类型名称
- 可能的返回值：
  * "string"
  * "number"
  * "boolean"
  * "undefined"
  * "object"（注意：null也返回"object"）
  * "function"
  * "symbol"
  * "bigint"

使用示例：
```jsx
console.log(typeof "hello");     // "string"
console.log(typeof 42);          // "number"
console.log(typeof true);        // "boolean"
console.log(typeof undefined);   // "undefined"
console.log(typeof null);        // "object"（bug）
```

注意事项：
- typeof null 返回 "object"（历史遗留bug）
- 检测null要用 value === null
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 String（字符串）类型
// --------------------------------------------

// 声明字符串：三种方式
const str1 = "双引号字符串";
const str2 = '单引号字符串';
const str3 = `反引号字符串`;

console.log("字符串1：", str1);
console.log("字符串2：", str2);
console.log("字符串3：", str3);

// 字符串拼接
const firstName = "张";
const lastName = "三";
const fullName = firstName + lastName;  // 使用+拼接
console.log("全名：", fullName);

// 字符串长度
const message = "Hello World";
console.log("字符串长度：", message.length);  // 11

// 字符串方法
const text = "JavaScript";
console.log("转大写：", text.toUpperCase());  // JAVASCRIPT
console.log("转小写：", text.toLowerCase());  // javascript
console.log("截取：", text.slice(0, 4));      // Java

// 模板字符串（重要！React中常用）
const name = "李四";
const age = 25;
const intro = `我叫${name}，今年${age}岁`;  // 使用${}插入变量
console.log(intro);

// 多行字符串
const multiLine = `第一行
第二行
第三行`;
console.log(multiLine);

console.log("=".repeat(60));

// --------------------------------------------
// 2.2 Number（数字）类型
// --------------------------------------------

// 声明数字
const integer = 42;           // 整数
const float = 3.14;           // 小数
const negative = -10;         // 负数
const scientific = 1e6;       // 科学计数法（1000000）

console.log("整数：", integer);
console.log("小数：", float);
console.log("负数：", negative);
console.log("科学计数法：", scientific);

// 数学运算
const a = 10;
const b = 3;

console.log("加法：", a + b);      // 13
console.log("减法：", a - b);      // 7
console.log("乘法：", a * b);      // 30
console.log("除法：", a / b);      // 3.333...
console.log("取余：", a % b);      // 1
console.log("幂运算：", a ** 2);   // 100

// 特殊数字值
console.log("无穷大：", 1 / 0);           // Infinity
console.log("负无穷：", -1 / 0);          // -Infinity
console.log("非数字：", "abc" / 2);       // NaN（Not a Number）

// 检测NaN
const result = "abc" / 2;
console.log("是否为NaN：", isNaN(result));  // true

console.log("=".repeat(60));

// --------------------------------------------
// 2.3 Boolean（布尔值）类型
// --------------------------------------------

// 声明布尔值
const isLoggedIn = true;
const isAdmin = false;

console.log("是否登录：", isLoggedIn);
console.log("是否管理员：", isAdmin);

// 布尔值用于条件判断
const age2 = 18;
const isAdult = age2 >= 18;  // 比较运算返回布尔值
console.log("是否成年：", isAdult);

// 逻辑运算
const hasPermission = true;
const isActive = true;

console.log("与运算：", hasPermission && isActive);  // true（都为true）
console.log("或运算：", hasPermission || isActive);  // true（至少一个true）
console.log("非运算：", !hasPermission);             // false（取反）

// 真值和假值（重要概念）
// 以下值转换为布尔值时为false（假值）：
// false、0、""（空字符串）、null、undefined、NaN
// 其他所有值都为true（真值）

console.log("空字符串转布尔：", Boolean(""));      // false
console.log("非空字符串转布尔：", Boolean("hi"));  // true
console.log("0转布尔：", Boolean(0));              // false
console.log("非0数字转布尔：", Boolean(42));       // true

console.log("=".repeat(60));

// --------------------------------------------
// 2.4 Null（空值）类型
// --------------------------------------------

// 声明null
let selectedUser = null;  // 初始化为空

console.log("初始值：", selectedUser);
console.log("类型：", typeof selectedUser);  // "object"（bug）

// 赋值
selectedUser = { name: "王五", age: 30 };
console.log("赋值后：", selectedUser);

// 清空
selectedUser = null;
console.log("清空后：", selectedUser);

// 检测null（正确方式）
if (selectedUser === null) {
  console.log("用户为空");
}

console.log("=".repeat(60));

// --------------------------------------------
// 2.5 Undefined（未定义）类型
// --------------------------------------------

// 声明但未赋值
let userName;
console.log("未赋值的变量：", userName);        // undefined
console.log("类型：", typeof userName);         // "undefined"

// 赋值后
userName = "赵六";
console.log("赋值后：", userName);

// 函数没有返回值
function noReturn() {
  // 没有return语句
}
console.log("函数返回值：", noReturn());  // undefined

// 对象不存在的属性
const user = { name: "孙七" };
console.log("不存在的属性：", user.age);  // undefined

console.log("=".repeat(60));

// --------------------------------------------
// 2.6 类型检测
// --------------------------------------------

// 使用typeof检测类型
console.log("typeof检测：");
console.log(typeof "hello");      // "string"
console.log(typeof 42);           // "number"
console.log(typeof true);         // "boolean"
console.log(typeof undefined);    // "undefined"
console.log(typeof null);         // "object"（bug！）
console.log(typeof {});           // "object"
console.log(typeof []);           // "object"
console.log(typeof function(){}); // "function"

// 检测null的正确方式
const value = null;
if (value === null) {
  console.log("这是null");
}

// 检测undefined
let value2;
if (value2 === undefined) {
  console.log("这是undefined");
}

// 或者使用typeof
if (typeof value2 === "undefined") {
  console.log("这也是undefined");
}

console.log("=".repeat(60));

/*
==============================================
深入理解：类型转换（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【深入内容】

1. 隐式类型转换
   JavaScript会自动转换类型，有时会导致意外结果

   例子：
   ```jsx
   console.log("5" + 3);    // "53"（字符串拼接）
   console.log("5" - 3);    // 2（数字减法）
   console.log(true + 1);   // 2（true转为1）
   console.log(false + 1);  // 1（false转为0）
   ```

2. 显式类型转换
   使用函数明确转换类型

   转为字符串：
   ```jsx
   String(123)        // "123"
   (123).toString()   // "123"
   ```

   转为数字：
   ```jsx
   Number("123")      // 123
   Number("abc")      // NaN
   parseInt("123")    // 123
   parseFloat("3.14") // 3.14
   ```

   转为布尔值：
   ```jsx
   Boolean(1)         // true
   Boolean(0)         // false
   Boolean("")        // false
   Boolean("hi")      // true
   ```

3. 相等比较
   == vs ===

   == 会进行类型转换：
   ```jsx
   "5" == 5    // true（字符串转为数字）
   null == undefined  // true
   ```

   === 不会转换类型（推荐使用）：
   ```jsx
   "5" === 5   // false（类型不同）
   null === undefined  // false
   ```

4. 为什么typeof null是"object"？
   这是JavaScript的历史bug，但为了兼容性无法修复
   正确检测null：value === null

==============================================
*/

// ============================================
// 第三部分：常见错误和陷阱
// ============================================

/*
3.1 错误1：字符串和数字相加

错误代码：
```jsx
const age = "25";
const nextAge = age + 1;
console.log(nextAge);  // "251"（字符串拼接，不是26）
```

结果：得到字符串"251"而不是数字26

原因：+运算符遇到字符串会进行拼接，不是数学运算

正确代码：
```jsx
const age = 25;  // 使用数字类型
const nextAge = age + 1;
console.log(nextAge);  // 26

// 或者转换类型
const ageStr = "25";
const nextAge2 = Number(ageStr) + 1;
console.log(nextAge2);  // 26
```

记忆技巧：数字用数字类型，不要用字符串
*/

/*
3.2 错误2：使用==比较不同类型

错误代码：
```jsx
const num = 0;
const str = "";
if (num == str) {
  console.log("相等");  // 会执行（意外！）
}
```

结果：0和空字符串被认为相等

原因：==会进行类型转换，导致意外结果

正确代码：
```jsx
const num = 0;
const str = "";
if (num === str) {
  console.log("相等");  // 不会执行
}
```

记忆技巧：永远使用===，不用==
*/

/*
3.3 错误3：使用typeof检测null

错误代码：
```jsx
const value = null;
if (typeof value === "null") {
  console.log("是null");  // 不会执行
}
```

结果：条件不成立，代码不执行

原因：typeof null 返回 "object"，不是 "null"

正确代码：
```jsx
const value = null;
if (value === null) {
  console.log("是null");  // 正确
}
```

记忆技巧：检测null用 === null，不用typeof
*/

/*
3.4 错误4：未初始化变量就使用

错误代码：
```jsx
let total;
total = total + 10;  // NaN（undefined + 10）
console.log(total);
```

结果：得到NaN

原因：undefined参与数学运算结果是NaN

正确代码：
```jsx
let total = 0;  // 初始化为0
total = total + 10;
console.log(total);  // 10
```

记忆技巧：数字变量初始化为0，字符串初始化为""
*/

/*
3.5 错误5：混淆null和undefined

错误理解：
```jsx
let user = undefined;  // 不推荐
```

正确理解：
```jsx
let user = null;  // 推荐：主动设置为空用null
let name;         // 推荐：未赋值自动为undefined
```

原因：
- undefined：系统自动设置（变量未赋值）
- null：程序员主动设置（故意为空）

记忆技巧：
- 想表示"空"用null
- 让系统自动处理用undefined（不赋值）
*/

/*
3.6 错误6：字符串数字的数学运算

错误代码：
```jsx
const price = "99.99";
const discount = price * 0.8;  // 可以运算，但不推荐
console.log(discount);  // 79.992
```

问题：虽然能运算，但类型不明确，容易出bug

正确代码：
```jsx
const price = 99.99;  // 使用数字类型
const discount = price * 0.8;
console.log(discount);  // 79.992
```

记忆技巧：数字就用number类型，不要用string
*/

/*
3.7 错误7：NaN的比较

错误代码：
```jsx
const result = "abc" / 2;  // NaN
if (result === NaN) {
  console.log("是NaN");  // 不会执行！
}
```

结果：条件不成立

原因：NaN不等于任何值，包括它自己

正确代码：
```jsx
const result = "abc" / 2;
if (isNaN(result)) {
  console.log("是NaN");  // 正确
}
```

记忆技巧：检测NaN用isNaN()函数
*/

// ============================================
// 第四部分：类型转换实践
// ============================================

// 字符串转数字
const strNum = "123";
const num1 = Number(strNum);
const num2 = parseInt(strNum);
const num3 = parseFloat("3.14");

console.log("Number()：", num1, typeof num1);
console.log("parseInt()：", num2, typeof num2);
console.log("parseFloat()：", num3, typeof num3);

// 数字转字符串
const numValue = 456;
const str4 = String(numValue);
const str5 = numValue.toString();

console.log("String()：", str4, typeof str4);
console.log("toString()：", str5, typeof str5);

// 转为布尔值
console.log("Boolean(1)：", Boolean(1));        // true
console.log("Boolean(0)：", Boolean(0));        // false
console.log("Boolean('')：", Boolean(""));      // false
console.log("Boolean('hi')：", Boolean("hi"));  // true

console.log("=".repeat(60));

// ============================================
// 第五部分：类型速查表
// ============================================

/*
【类型1：String】
声明：const str = "文本" 或 '文本' 或 `文本`
检测：typeof str === "string"
转换：String(value) 或 value.toString()
常用：length、toUpperCase()、toLowerCase()、slice()

【类型2：Number】
声明：const num = 123 或 3.14
检测：typeof num === "number"
转换：Number(value)、parseInt(value)、parseFloat(value)
特殊值：Infinity、-Infinity、NaN

【类型3：Boolean】
声明：const bool = true 或 false
检测：typeof bool === "boolean"
转换：Boolean(value)
假值：false、0、""、null、undefined、NaN

【类型4：Null】
声明：const value = null
检测：value === null（不用typeof）
用途：主动设置为空

【类型5：Undefined】
声明：let value;（不赋值）
检测：value === undefined 或 typeof value === "undefined"
用途：变量未赋值的默认值
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习1：类型识别
要求：
  - 声明以下变量并输出类型：
    * 你的姓名（字符串）
    * 你的年龄（数字）
    * 是否是学生（布尔值）
    * 你的宠物（null，表示没有宠物）
    * 你的爱好（先声明不赋值）
  - 使用typeof检测每个变量的类型
提示：注意null的typeof返回"object"

练习2：类型转换
要求：
  - 将字符串"2026"转为数字
  - 将数字100转为字符串
  - 将空字符串转为布尔值
  - 将数字0转为布尔值
  - 输出所有转换结果和类型
提示：使用Number()、String()、Boolean()

练习3：模板字符串
要求：
  - 声明变量：商品名称、价格、数量
  - 使用模板字符串输出：
    "商品：XXX，单价：XX元，数量：X，总价：XX元"
  - 计算总价并在模板字符串中显示
提示：使用反引号和${}

练习4：真值和假值
要求：
  - 测试以下值转为布尔值的结果：
    * ""（空字符串）
    * "0"（字符串0）
    * 0（数字0）
    * 1（数字1）
    * null
    * undefined
    * NaN
  - 输出每个值的布尔转换结果
提示：使用Boolean()函数

练习5：找出错误
要求：
  - 分析以下代码，找出所有问题：
  ```jsx
  const age = "25";
  const nextAge = age + 1;
  console.log(nextAge);  // 期望26，实际？

  const value = null;
  if (typeof value === "null") {
    console.log("是null");
  }

  const result = "abc" / 2;
  if (result === NaN) {
    console.log("是NaN");
  }
  ```
提示：类型转换、typeof null、NaN比较
*/

// ============================================
// 总结
// ============================================

/*
【今日重点】

1. 五种基本类型：string、number、boolean、null、undefined
2. 使用typeof检测类型（注意typeof null是"object"）
3. 字符串用引号，推荐使用模板字符串（反引号）
4. 永远使用===比较，不用==
5. 检测null用===，检测NaN用isNaN()

【下一步】
学习模板字符串详解（03_js_template_strings.jsx）
*/
