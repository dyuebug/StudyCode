// ============================================
// main.js - 主入口文件
// ============================================

/**
 * 这个文件演示如何导入和使用模块
 * - 命名导入
 * - 默认导入
 * - 混合导入
 * - 重命名导入
 * - 全部导入
 */

console.log("=".repeat(60));
console.log("🚀 主程序启动");
console.log("=".repeat(60));

// ============================================
// 第一部分：导入命名导出 ⭐⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第一部分：导入命名导出");
console.log("=".repeat(60));

// 从math.js导入多个命名导出
import { PI, E, add, subtract, multiply, divide, Calculator } from './modules/math.js';

console.log("\n1.1 使用导入的常量");
console.log("PI =", PI);
console.log("E =", E);

console.log("\n1.2 使用导入的函数");
console.log("add(5, 3) =", add(5, 3));
console.log("subtract(10, 4) =", subtract(10, 4));
console.log("multiply(6, 7) =", multiply(6, 7));
console.log("divide(20, 4) =", divide(20, 4));

console.log("\n1.3 使用导入的类");
const calc = new Calculator();
const result = calc.add(10).multiply(2).subtract(5).divide(3).getResult();
console.log("计算器链式调用结果:", result);

// ============================================
// 第二部分：导入默认导出 ⭐⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第二部分：导入默认导出");
console.log("=".repeat(60));

// 从string.js导入默认导出（可以自定义名称）
import StringUtils from './modules/string.js';

console.log("\n2.1 使用字符串工具");
const text = "hello world";
console.log("原始文本:", text);
console.log("首字母大写:", StringUtils.capitalize(text));
console.log("转大写:", StringUtils.toUpperCase(text));
console.log("反转:", StringUtils.reverse(text));
console.log("截断(5):", StringUtils.truncate(text, 5));
console.log("重复(3):", StringUtils.repeat("abc", 3));

const sentence = "这是一个测试句子";
console.log("\n句子:", sentence);
console.log("单词数:", StringUtils.wordCount(sentence));
console.log("字符数:", StringUtils.charCount(sentence));

// ============================================
// 第三部分：混合导入 ⭐⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第三部分：混合导入（默认+命名）");
console.log("=".repeat(60));

// 同时导入默认导出和命名导出
import User, { USER_ROLES, USER_STATUS, validateEmail, validatePassword } from './modules/user.js';

console.log("\n3.1 使用导入的常量");
console.log("用户角色:", USER_ROLES);
console.log("用户状态:", USER_STATUS);

console.log("\n3.2 使用导入的工具函数");
const email1 = "test@example.com";
const email2 = "invalid-email";
console.log(`${email1} 是否有效:`, validateEmail(email1));
console.log(`${email2} 是否有效:`, validateEmail(email2));

const pwd1 = "abc12345";
const pwd2 = "123";
console.log(`${pwd1} 是否有效:`, validatePassword(pwd1));
console.log(`${pwd2} 是否有效:`, validatePassword(pwd2));

console.log("\n3.3 使用导入的类");
const user1 = new User("张三", "zhangsan@example.com", USER_ROLES.ADMIN);
console.log("用户信息:", user1.getInfo());
console.log("是否为管理员:", user1.isAdmin());
console.log("是否激活:", user1.isActive());

// ============================================
// 第四部分：重命名导入 ⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第四部分：重命名导入");
console.log("=".repeat(60));

// 使用as关键字重命名导入
import { add as mathAdd, multiply as mathMultiply } from './modules/math.js';

console.log("\n4.1 使用重命名的函数");
console.log("mathAdd(3, 7) =", mathAdd(3, 7));
console.log("mathMultiply(4, 5) =", mathMultiply(4, 5));

console.log("✅ 好处：避免命名冲突");

// ============================================
// 第五部分：全部导入 ⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第五部分：全部导入");
console.log("=".repeat(60));

// 使用 * as 导入模块的所有导出
import * as MathModule from './modules/math.js';

console.log("\n5.1 使用命名空间访问");
console.log("MathModule.PI =", MathModule.PI);
console.log("MathModule.add(1, 2) =", MathModule.add(1, 2));

const calc2 = new MathModule.Calculator();
console.log("计算器:", calc2.add(100).getResult());

console.log("✅ 好处：清晰的命名空间，避免污染");

// ============================================
// 第六部分：导入配置模块 ⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第六部分：导入配置模块");
console.log("=".repeat(60));

import { appConfig, themeConfig, featureFlags } from './modules/config.js';

console.log("\n6.1 应用配置");
console.log("应用名称:", appConfig.appName);
console.log("版本:", appConfig.version);
console.log("API地址:", appConfig.apiUrl);

console.log("\n6.2 主题配置");
console.log("主色调:", themeConfig.primaryColor);
console.log("副色调:", themeConfig.secondaryColor);

console.log("\n6.3 功能开关");
console.log("通知:", featureFlags.enableNotifications ? "开启" : "关闭");
console.log("深色模式:", featureFlags.enableDarkMode ? "开启" : "关闭");

// ============================================
// 第七部分：综合应用示例 ⭐⭐⭐⭐⭐
// ============================================

console.log("\n" + "=".repeat(60));
console.log("第七部分：综合应用示例");
console.log("=".repeat(60));

console.log("\n7.1 创建用户管理系统");

// 创建多个用户
const users = [
    new User("张三", "zhangsan@example.com", USER_ROLES.ADMIN),
    new User("李四", "lisi@example.com", USER_ROLES.USER),
    new User("王五", "wangwu@example.com", USER_ROLES.USER)
];

// 显示用户信息
users.forEach((user, index) => {
    const info = user.getInfo();
    console.log(`用户${index + 1}:`, {
        name: info.name,
        email: info.email,
        role: info.role,
        isAdmin: user.isAdmin()
    });
});

console.log("\n7.2 字符串处理示例");
const longText = "这是一个很长的文本内容，需要进行处理";
console.log("原文:", longText);
console.log("截断:", StringUtils.truncate(longText, 10));
console.log("字符数:", StringUtils.charCount(longText));

console.log("\n7.3 数学计算示例");
const numbers = [10, 20, 30, 40, 50];
const sum = numbers.reduce((acc, num) => add(acc, num), 0);
const avg = divide(sum, numbers.length);
console.log("数组:", numbers);
console.log("总和:", sum);
console.log("平均值:", avg);

// ============================================
// 总结
// ============================================

console.log("\n" + "=".repeat(60));
console.log("✅ ES6模块化学习完成！");
console.log("=".repeat(60));

console.log("\n📝 总结：");
console.log("1. 命名导出：export { a, b } → import { a, b }");
console.log("2. 默认导出：export default X → import X");
console.log("3. 混合导出：可以同时有命名和默认导出");
console.log("4. 重命名：import { a as b }");
console.log("5. 全部导入：import * as Module");
console.log("6. 模块路径：相对路径必须有./或../");
console.log("7. 文件扩展名：必须写.js");
console.log("8. 运行环境：必须在服务器环境（不能file://）");

console.log("\n💡 模块化的价值：");
console.log("✅ 代码组织清晰");
console.log("✅ 避免全局污染");
console.log("✅ 依赖关系明确");
console.log("✅ 代码可复用");
console.log("✅ 按需加载");
