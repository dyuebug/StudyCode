// ============================================
// Storage Module - 数据持久化模块
// 日期：2026-06-16（第46天）
// 作用：负责与LocalStorage交互，保存和加载任务数据
// ============================================

/**
 * Storage类 - 数据持久化管理
 *
 * 职责：
 * - 保存任务数据到LocalStorage
 * - 从LocalStorage加载任务数据
 * - 清空存储数据
 *
 * 设计模式：使用静态方法，无需实例化
 */
class Storage {
    // 存储键名（常量）
    static STORAGE_KEY = 'todoApp_tasks';

    /**
     * 保存任务到LocalStorage
     * @param {Array} tasks - 任务数组
     * @returns {boolean} - 保存是否成功
     */
    static saveTasks(tasks) {
        try {
            // 将任务数组转换为JSON字符串
            const tasksJSON = JSON.stringify(tasks);

            // 保存到LocalStorage
            localStorage.setItem(this.STORAGE_KEY, tasksJSON);

            // 成功
            return true;
        } catch (error) {
            // 错误处理：可能的原因
            // 1. LocalStorage空间已满
            // 2. 浏览器隐私模式禁用LocalStorage
            // 3. 数据无法序列化
            console.error('保存任务失败:', error);
            return false;
        }
    }

    /**
     * 从LocalStorage加载任务
     * @returns {Array} - 任务数组（如果失败返回空数组）
     */
    static loadTasks() {
        try {
            // 从LocalStorage读取数据
            const tasksJSON = localStorage.getItem(this.STORAGE_KEY);

            // 如果没有数据，返回空数组
            if (!tasksJSON) {
                return [];
            }

            // 将JSON字符串解析为数组
            const tasks = JSON.parse(tasksJSON);

            // 验证数据格式（确保是数组）
            if (!Array.isArray(tasks)) {
                console.warn('存储数据格式错误，返回空数组');
                return [];
            }

            return tasks;
        } catch (error) {
            // 错误处理：可能的原因
            // 1. JSON格式错误（数据损坏）
            // 2. LocalStorage不可用
            console.error('加载任务失败:', error);
            return [];
        }
    }

    /**
     * 清空LocalStorage中的任务数据
     * @returns {boolean} - 清空是否成功
     */
    static clearStorage() {
        try {
            localStorage.removeItem(this.STORAGE_KEY);
            return true;
        } catch (error) {
            console.error('清空存储失败:', error);
            return false;
        }
    }

    /**
     * 检查LocalStorage是否可用
     * @returns {boolean} - LocalStorage是否可用
     */
    static isStorageAvailable() {
        try {
            const testKey = '__storage_test__';
            localStorage.setItem(testKey, 'test');
            localStorage.removeItem(testKey);
            return true;
        } catch (error) {
            return false;
        }
    }

    /**
     * 获取存储空间使用情况（估算）
     * @returns {Object} - {used: 使用字节数, percentage: 使用百分比}
     */
    static getStorageInfo() {
        try {
            const tasksJSON = localStorage.getItem(this.STORAGE_KEY);
            const used = tasksJSON ? tasksJSON.length : 0;

            // LocalStorage通常限制为5-10MB（这里估算为5MB）
            const total = 5 * 1024 * 1024; // 5MB
            const percentage = (used / total * 100).toFixed(2);

            return {
                used,
                total,
                percentage: parseFloat(percentage)
            };
        } catch (error) {
            return {
                used: 0,
                total: 0,
                percentage: 0
            };
        }
    }
}

// --------------------------------------------
// 深入理解：LocalStorage
// --------------------------------------------

/**
 * 🔍 深入理解：LocalStorage的特点
 *
 * 1. 持久化存储
 *    - 数据不会过期，除非手动删除
 *    - 浏览器关闭后数据仍然存在
 *    - 不同于SessionStorage（关闭浏览器就清空）
 *
 * 2. 存储限制
 *    - 大小限制：通常为5-10MB（浏览器不同而不同）
 *    - 只能存储字符串（需要JSON.stringify/parse）
 *    - 同步API（可能阻塞UI，但对小数据影响很小）
 *
 * 3. 作用域
 *    - 同源策略：协议、域名、端口都相同才能访问
 *    - 不同页面共享数据（同一域名下）
 *
 * 4. 优缺点
 *    优点：
 *    - 简单易用
 *    - 数据持久化
 *    - 无需服务器
 *
 *    缺点：
 *    - 容量有限
 *    - 只能存字符串
 *    - 同步API（大数据可能卡顿）
 *    - 不够安全（不适合存敏感数据）
 *
 * 5. 使用场景
 *    - 用户偏好设置
 *    - 简单数据缓存
 *    - 离线应用数据
 *    - 购物车数据
 */

// --------------------------------------------
// 最佳实践
// --------------------------------------------

/**
 * 1. 错误处理：
 *    - 始终使用try/catch包裹
 *    - 提供降级方案（返回空数组）
 *
 * 2. 数据验证：
 *    - 检查数据格式是否正确
 *    - 防止数据损坏导致应用崩溃
 *
 * 3. 容量管理：
 *    - 避免存储大量数据
 *    - 定期清理无用数据
 *
 * 4. 安全性：
 *    - 不存储敏感信息（密码、token等）
 *    - 不信任LocalStorage中的数据（可能被篡改）
 */
