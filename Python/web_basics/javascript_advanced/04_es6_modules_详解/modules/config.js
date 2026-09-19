// ============================================
// config.js - 配置模块
// ============================================

/**
 * 这个模块演示配置管理
 * - 集中管理应用配置
 * - 导出配置对象
 */

console.log("📦 config.js 模块已加载");

// 应用配置
export const appConfig = {
    appName: "我的应用",
    version: "1.0.0",
    apiUrl: "https://api.example.com",
    timeout: 5000,
    debug: true
};

// 主题配置
export const themeConfig = {
    primaryColor: "#667eea",
    secondaryColor: "#764ba2",
    fontSize: "16px",
    fontFamily: "Arial, sans-serif"
};

// 功能开关
export const featureFlags = {
    enableNotifications: true,
    enableDarkMode: false,
    enableAnalytics: true
};

console.log("✅ config.js 导出：appConfig, themeConfig, featureFlags");
