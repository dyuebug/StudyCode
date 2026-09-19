// ============================================
// math.js - 数学工具模块（命名导出示例）
// ============================================

/**
 * 这个模块演示"命名导出"
 * - 可以导出多个内容
 * - 导入时必须使用 { } 包裹
 * - 可以导出变量、函数、类
 */

console.log("📦 math.js 模块已加载");

// --------------------------------------------
// 1. 导出常量
// --------------------------------------------

export const PI = 3.14159;
export const E = 2.71828;

// --------------------------------------------
// 2. 导出函数
// --------------------------------------------

// 加法
export function add(a, b) {
    return a + b;
}

// 减法
export function subtract(a, b) {
    return a - b;
}

// 乘法
export function multiply(a, b) {
    return a * b;
}

// 除法
export function divide(a, b) {
    if (b === 0) {
        throw new Error("除数不能为0");
    }
    return a / b;
}

// 幂运算
export function power(base, exponent) {
    return Math.pow(base, exponent);
}

// --------------------------------------------
// 3. 导出类
// --------------------------------------------

export class Calculator {
    constructor() {
        this.result = 0;
    }

    add(num) {
        this.result += num;
        return this;
    }

    subtract(num) {
        this.result -= num;
        return this;
    }

    multiply(num) {
        this.result *= num;
        return this;
    }

    divide(num) {
        if (num === 0) {
            throw new Error("除数不能为0");
        }
        this.result /= num;
        return this;
    }

    getResult() {
        return this.result;
    }

    reset() {
        this.result = 0;
        return this;
    }
}

// --------------------------------------------
// 4. 私有函数（不导出）
// --------------------------------------------

// 这个函数不导出，只能在模块内部使用
function privateHelper() {
    console.log("这是私有函数，外部无法访问");
}

// --------------------------------------------
// 5. 批量导出（可选写法）
// --------------------------------------------

// 也可以在最后统一导出
// export { PI, E, add, subtract, multiply, divide, Calculator };

console.log("✅ math.js 导出：PI, E, add, subtract, multiply, divide, Calculator");
