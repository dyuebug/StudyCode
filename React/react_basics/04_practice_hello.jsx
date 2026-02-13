// ============================================
// 第一个综合练习：自我介绍程序
// 日期：2026-02-12（第1天）
// 难度：初级
// ============================================

/*
【练习目标】

通过这个练习，你将综合运用今天学到的知识：
1. 使用const和let声明变量
2. 使用5种基本数据类型
3. 使用模板字符串拼接文本
4. 使用typeof检测类型
5. 进行简单的数学运算

【任务说明】

创建一个自我介绍程序，包含以下信息：
- 个人基本信息（姓名、年龄、城市）
- 学习目标和计划
- 技能清单
- 学习统计

【完成标准】

✅ 正确使用const和let
✅ 使用所有5种基本数据类型
✅ 使用模板字符串生成输出
✅ 代码有详细注释
✅ 输出格式美观易读
*/

// ============================================
// 第一部分：个人基本信息
// ============================================

// 使用const声明不会改变的个人信息
const myName = "张三";              // String类型：姓名
const birthYear = 2000;             // Number类型：出生年份
const currentYear = 2026;           // Number类型：当前年份
const myCity = "北京";              // String类型：城市
const isStudent = true;             // Boolean类型：是否是学生

// 使用let声明可能会改变的信息
let myAge = currentYear - birthYear;  // 计算年龄

// 输出个人信息
console.log("=".repeat(60));
console.log("个人基本信息");
console.log("=".repeat(60));

// 使用模板字符串生成自我介绍
const introduction = `
大家好！我叫${myName}，今年${myAge}岁，来自${myCity}。
我${isStudent ? "是" : "不是"}一名学生，正在学习React开发。
`;

console.log(introduction);

// 输出数据类型（帮助理解类型系统）
console.log("数据类型检测：");
console.log(`myName的类型：${typeof myName}`);        // string
console.log(`myAge的类型：${typeof myAge}`);          // number
console.log(`isStudent的类型：${typeof isStudent}`);  // boolean

console.log("=".repeat(60));

// ============================================
// 第二部分：学习目标和计划
// ============================================

// 学习相关信息
const learningGoal = "成为一名React开发者";
const totalWeeks = 17;              // 总学习周数
const currentWeek = 1;              // 当前周数
let completedWeeks = 0;             // 已完成周数（初始为0）

// 计算学习进度
const progressPercentage = (completedWeeks / totalWeeks * 100).toFixed(2);

// 输出学习计划
console.log("学习目标和计划");
console.log("=".repeat(60));

const learningPlan = `
学习目标：${learningGoal}
学习周期：${totalWeeks}周
当前进度：第${currentWeek}周
已完成：${completedWeeks}周
完成度：${progressPercentage}%
剩余周数：${totalWeeks - completedWeeks}周
`;

console.log(learningPlan);

console.log("=".repeat(60));

// ============================================
// 第三部分：技能清单
// ============================================

// 当前技能（使用Boolean表示是否掌握）
const knowsHTML = true;
const knowsCSS = true;
const knowsJavaScript = false;      // 正在学习中
const knowsReact = false;           // 还未开始
const knowsTypeScript = false;      // 还未开始

// 统计已掌握技能数量
let masteredSkills = 0;
if (knowsHTML) masteredSkills++;
if (knowsCSS) masteredSkills++;
if (knowsJavaScript) masteredSkills++;
if (knowsReact) masteredSkills++;
if (knowsTypeScript) masteredSkills++;

const totalSkills = 5;

// 输出技能清单
console.log("技能清单");
console.log("=".repeat(60));

const skillsList = `
HTML：${knowsHTML ? "✅ 已掌握" : "❌ 未掌握"}
CSS：${knowsCSS ? "✅ 已掌握" : "❌ 未掌握"}
JavaScript：${knowsJavaScript ? "✅ 已掌握" : "❌ 未掌握"} (学习中)
React：${knowsReact ? "✅ 已掌握" : "❌ 未掌握"}
TypeScript：${knowsTypeScript ? "✅ 已掌握" : "❌ 未掌握"}

已掌握：${masteredSkills}/${totalSkills}
掌握率：${(masteredSkills / totalSkills * 100).toFixed(2)}%
`;

console.log(skillsList);

console.log("=".repeat(60));

// ============================================
// 第四部分：学习统计
// ============================================

// 学习时间统计
let totalStudyHours = 0;            // 总学习时长（小时）
const todayStudyHours = 2;          // 今天学习时长
const targetDailyHours = 2;         // 每天目标学习时长

// 更新总学习时长
totalStudyHours = totalStudyHours + todayStudyHours;

// 学习天数
let studyDays = 1;                  // 今天是第1天

// 计算平均每天学习时长
const averageHoursPerDay = totalStudyHours / studyDays;

// 输出学习统计
console.log("学习统计");
console.log("=".repeat(60));

const studyStats = `
学习天数：${studyDays}天
总学习时长：${totalStudyHours}小时
今天学习：${todayStudyHours}小时
平均每天：${averageHoursPerDay.toFixed(2)}小时
目标时长：${targetDailyHours}小时/天
是否达标：${averageHoursPerDay >= targetDailyHours ? "✅ 已达标" : "❌ 未达标"}
`;

console.log(studyStats);

console.log("=".repeat(60));

// ============================================
// 第五部分：激励语和下一步计划
// ============================================

// 根据学习进度生成激励语
let motivationMessage;

if (completedWeeks === 0) {
  motivationMessage = "万事开头难，加油！今天是新的开始！";
} else if (completedWeeks < totalWeeks / 2) {
  motivationMessage = "继续保持，你已经走在正确的道路上！";
} else if (completedWeeks < totalWeeks) {
  motivationMessage = "已经过半了，胜利就在前方！";
} else {
  motivationMessage = "恭喜你完成了全部学习计划！";
}

// 下一步学习内容
const nextTopic = "JavaScript函数和箭头函数";
const nextWeekGoal = "掌握JavaScript基础语法";

// 输出激励语和计划
console.log("激励语和下一步");
console.log("=".repeat(60));

const finalMessage = `
${motivationMessage}

下一步学习：${nextTopic}
本周目标：${nextWeekGoal}

记住：
- 每天坚持学习${targetDailyHours}小时
- 理解比记忆更重要
- 多动手实践
- 遇到问题及时提问

加油，${myName}！你一定可以成为优秀的React开发者！
`;

console.log(finalMessage);

console.log("=".repeat(60));

// ============================================
// 第六部分：类型检测总结
// ============================================

// 演示所有5种基本类型
console.log("五种基本数据类型演示");
console.log("=".repeat(60));

const stringExample = "这是字符串";
const numberExample = 42;
const booleanExample = true;
const nullExample = null;
let undefinedExample;

console.log(`String示例：${stringExample}，类型：${typeof stringExample}`);
console.log(`Number示例：${numberExample}，类型：${typeof numberExample}`);
console.log(`Boolean示例：${booleanExample}，类型：${typeof booleanExample}`);
console.log(`Null示例：${nullExample}，类型：${typeof nullExample}`);  // 注意：返回"object"
console.log(`Undefined示例：${undefinedExample}，类型：${typeof undefinedExample}`);

// 特别说明null的类型检测
console.log("\n特别提示：");
console.log(`typeof null 返回 "${typeof nullExample}"（这是JavaScript的历史bug）`);
console.log(`正确检测null的方式：nullExample === null 结果为 ${nullExample === null}`);

console.log("=".repeat(60));

// ============================================
// 练习任务（请完成以下任务）
// ============================================

/*
【任务1：修改个人信息】
要求：
  - 将上面的个人信息改成你自己的
  - 修改姓名、出生年份、城市
  - 运行代码，查看输出

【任务2：更新技能清单】
要求：
  - 根据你的实际情况修改技能掌握状态
  - 如果你已经会JavaScript，将knowsJavaScript改为true
  - 运行代码，查看技能统计的变化

【任务3：添加新的个人信息】
要求：
  - 添加以下新信息：
    * 你的爱好（字符串）
    * 你的邮箱（字符串）
    * 是否有编程经验（布尔值）
  - 使用模板字符串输出这些信息

【任务4：计算学习目标】
要求：
  - 假设你每天学习2小时
  - 计算17周需要学习多少小时
  - 计算每周需要学习多少小时
  - 使用模板字符串输出结果

【任务5：创建学习日历】
要求：
  - 声明变量：学习开始日期（字符串）
  - 计算17周后的结束日期（可以简单计算天数）
  - 使用模板字符串输出学习日历
  - 格式：
    ```
    学习日历
    开始日期：XXXX-XX-XX
    结束日期：XXXX-XX-XX
    总天数：XXX天
    ```

【任务6：综合练习】
要求：
  - 创建一个完整的"学习档案"
  - 包含：个人信息、学习目标、技能清单、学习统计
  - 使用多行模板字符串生成美观的输出
  - 确保所有变量使用正确的类型
  - 添加详细的注释
*/

// ============================================
// 你的代码写在这里
// ============================================

// 任务1：修改个人信息
// TODO: 将上面的个人信息改成你自己的

// 任务2：更新技能清单
// TODO: 根据实际情况修改技能状态

// 任务3：添加新的个人信息
// TODO: 添加爱好、邮箱、编程经验

// 任务4：计算学习目标
// TODO: 计算总学习时长

// 任务5：创建学习日历
// TODO: 创建学习日历

// 任务6：综合练习
// TODO: 创建完整的学习档案

// ============================================
// 总结
// ============================================

/*
【今天学到了什么？】

1. 变量声明
   - const：声明常量（优先使用）
   - let：声明可变变量
   - var：旧语法（不推荐）

2. 数据类型
   - String：字符串（文本）
   - Number：数字（整数和小数）
   - Boolean：布尔值（true/false）
   - Null：空值（主动设置为空）
   - Undefined：未定义（未赋值）

3. 模板字符串
   - 使用反引号（`）
   - 使用${}插入变量
   - 支持多行文本
   - 可以插入表达式

4. 类型检测
   - 使用typeof检测类型
   - 注意typeof null返回"object"
   - 检测null用===

【下一步】

明天我们将学习：
- JavaScript函数声明
- 箭头函数
- 函数参数和返回值
- 函数作用域

【学习建议】

1. 完成上面的所有练习任务
2. 尝试修改代码，观察输出变化
3. 在浏览器控制台（F12）中运行代码
4. 遇到问题及时提问
5. 做好笔记，记录重点和易错点

【如何运行这个文件？】

方法1：在浏览器中运行
  1. 创建一个HTML文件
  2. 使用<script src="04_practice_hello.jsx"></script>引入
  3. 在浏览器中打开HTML文件
  4. 按F12打开控制台查看输出

方法2：在Node.js中运行
  1. 确保安装了Node.js
  2. 在终端中运行：node 04_practice_hello.jsx
  3. 查看终端输出

方法3：在在线编辑器中运行
  1. 访问 https://jsfiddle.net/ 或 https://codepen.io/
  2. 复制代码到编辑器
  3. 运行并查看控制台输出

加油！完成这些练习后，你就掌握了JavaScript的基础知识！
*/
