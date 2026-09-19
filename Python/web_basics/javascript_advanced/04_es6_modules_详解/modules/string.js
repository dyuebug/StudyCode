// ============================================
// string.js - 字符串工具模块（默认导出示例）
// ============================================

/**
 * 这个模块演示"默认导出"
 * - 一个模块只能有一个默认导出
 * - 导入时不需要 { }，可以自定义名称
 * - 默认导出可以是函数、类、对象等
 */

console.log("📦 string.js 模块已加载");

// --------------------------------------------
// 默认导出一个对象（包含多个工具函数）
// --------------------------------------------

const StringUtils = {
    // 首字母大写
    capitalize(str) {
        if (!str) return '';
        return str.charAt(0).toUpperCase() + str.slice(1);
    },

    // 转为大写
    toUpperCase(str) {
        return str.toUpperCase();
    },

    // 转为小写
    toLowerCase(str) {
        return str.toLowerCase();
    },

    // 反转字符串
    reverse(str) {
        return str.split('').reverse().join('');
    },

    // 截断字符串
    truncate(str, maxLength) {
        if (str.length <= maxLength) {
            return str;
        }
        return str.slice(0, maxLength) + '...';
    },

    // 重复字符串
    repeat(str, times) {
        return str.repeat(times);
    },

    // 移除空格
    trim(str) {
        return str.trim();
    },

    // 单词计数
    wordCount(str) {
        return str.trim().split(/\s+/).length;
    },

    // 字符计数（不包含空格）
    charCount(str) {
        return str.replace(/\s/g, '').length;
    },

    // 判断是否为回文
    isPalindrome(str) {
        const cleaned = str.toLowerCase().replace(/[^a-z0-9]/g, '');
        return cleaned === cleaned.split('').reverse().join('');
    }
};

// 默认导出
export default StringUtils;

console.log("✅ string.js 默认导出：StringUtils 对象");
