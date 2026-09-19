// ============================================
// user.js - 用户模块（混合导出示例）
// ============================================

/**
 * 这个模块演示"混合导出"
 * - 既有命名导出，又有默认导出
 * - 导入时可以同时导入两者
 */

console.log("📦 user.js 模块已加载");

// --------------------------------------------
// 1. 命名导出：用户相关的常量
// --------------------------------------------

export const USER_ROLES = {
    ADMIN: 'admin',
    USER: 'user',
    GUEST: 'guest'
};

export const USER_STATUS = {
    ACTIVE: 'active',
    INACTIVE: 'inactive',
    BANNED: 'banned'
};

// --------------------------------------------
// 2. 命名导出：工具函数
// --------------------------------------------

export function validateEmail(email) {
    const regex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return regex.test(email);
}

export function validatePassword(password) {
    // 至少8位，包含字母和数字
    return password.length >= 8 && /[a-zA-Z]/.test(password) && /[0-9]/.test(password);
}

// --------------------------------------------
// 3. 默认导出：User类
// --------------------------------------------

class User {
    constructor(name, email, role = USER_ROLES.USER) {
        this.id = Date.now();
        this.name = name;
        this.email = email;
        this.role = role;
        this.status = USER_STATUS.ACTIVE;
        this.createdAt = new Date();
    }

    // 获取用户信息
    getInfo() {
        return {
            id: this.id,
            name: this.name,
            email: this.email,
            role: this.role,
            status: this.status,
            createdAt: this.createdAt
        };
    }

    // 更新名称
    updateName(newName) {
        this.name = newName;
    }

    // 更新邮箱
    updateEmail(newEmail) {
        if (validateEmail(newEmail)) {
            this.email = newEmail;
            return true;
        }
        return false;
    }

    // 更改角色
    changeRole(newRole) {
        if (Object.values(USER_ROLES).includes(newRole)) {
            this.role = newRole;
            return true;
        }
        return false;
    }

    // 激活用户
    activate() {
        this.status = USER_STATUS.ACTIVE;
    }

    // 停用用户
    deactivate() {
        this.status = USER_STATUS.INACTIVE;
    }

    // 封禁用户
    ban() {
        this.status = USER_STATUS.BANNED;
    }

    // 判断是否为管理员
    isAdmin() {
        return this.role === USER_ROLES.ADMIN;
    }

    // 判断是否激活
    isActive() {
        return this.status === USER_STATUS.ACTIVE;
    }
}

// 默认导出User类
export default User;

console.log("✅ user.js 导出：");
console.log("   - 命名导出：USER_ROLES, USER_STATUS, validateEmail, validatePassword");
console.log("   - 默认导出：User 类");
