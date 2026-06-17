// ============================================
// TodoApp - 主应用逻辑
// 日期：2026-06-16（第46天）
// 作用：管理Todo应用的核心功能和UI交互
// ============================================

/**
 * TodoApp类 - 应用主控制器
 *
 * 职责：
 * - 管理任务数据（增删改查）
 * - 处理用户交互（点击、输入）
 * - 更新界面显示（渲染任务、统计）
 * - 与Storage模块交互（数据持久化）
 *
 * 设计模式：单一职责原则（SRP）
 */
class TodoApp {
    /**
     * 构造函数 - 初始化应用
     */
    constructor() {
        // 任务数组
        this.tasks = [];

        // 当前筛选状态：'all' | 'active' | 'completed'
        this.currentFilter = 'all';

        // DOM元素引用（缓存DOM查询，提高性能）
        this.elements = {
            taskInput: document.getElementById('taskInput'),
            addTaskBtn: document.getElementById('addTaskBtn'),
            taskList: document.getElementById('taskList'),
            filterBtns: document.querySelectorAll('.filter-btn'),
            totalCount: document.getElementById('totalCount'),
            completedCount: document.getElementById('completedCount'),
            activeCount: document.getElementById('activeCount'),
            progressBar: document.getElementById('progressBar')
        };

        // 初始化应用
        this.init();
    }

    /**
     * 初始化应用
     * - 加载数据
     * - 绑定事件
     * - 渲染界面
     */
    init() {
        // 1. 检查LocalStorage是否可用
        if (!Storage.isStorageAvailable()) {
            console.warn('LocalStorage不可用，数据不会被保存');
            alert('⚠️ 浏览器不支持数据保存功能，刷新页面后数据会丢失');
        }

        // 2. 从LocalStorage加载任务
        this.tasks = Storage.loadTasks();

        // 3. 绑定事件监听器
        this.bindEvents();

        // 4. 渲染任务列表
        this.render();

        // 5. 输入框自动聚焦
        this.elements.taskInput.focus();

        console.log('✅ TodoApp初始化完成');
    }

    /**
     * 绑定事件监听器
     */
    bindEvents() {
        // 1. 添加按钮点击事件
        this.elements.addTaskBtn.addEventListener('click', () => {
            this.handleAddTask();
        });

        // 2. 输入框回车键事件
        this.elements.taskInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.handleAddTask();
            }
        });

        // 3. 任务列表点击事件（事件委托）
        this.elements.taskList.addEventListener('click', (e) => {
            this.handleTaskListClick(e);
        });

        // 4. 筛选按钮点击事件
        this.elements.filterBtns.forEach(btn => {
            btn.addEventListener('click', (e) => {
                const filter = e.target.dataset.filter;
                this.filterTasks(filter);
            });
        });
    }

    /**
     * 处理添加任务
     */
    handleAddTask() {
        // 获取输入内容并去除首尾空格
        const text = this.elements.taskInput.value.trim();

        // 输入验证：不能为空
        if (!text) {
            // 提示用户
            this.elements.taskInput.focus();
            this.elements.taskInput.classList.add('is-invalid');

            // 0.5秒后移除错误样式
            setTimeout(() => {
                this.elements.taskInput.classList.remove('is-invalid');
            }, 500);

            return;
        }

        // 添加任务
        this.addTask(text);

        // 清空输入框
        this.elements.taskInput.value = '';

        // 输入框重新聚焦
        this.elements.taskInput.focus();
    }

    /**
     * 添加任务
     * @param {string} text - 任务内容
     */
    addTask(text) {
        // 创建任务对象
        const task = {
            id: Date.now(),              // 唯一ID（时间戳）
            text: text,                   // 任务内容
            completed: false,             // 完成状态
            createdAt: Date.now()         // 创建时间
        };

        // 添加到任务数组（添加到开头，新任务在最上面）
        this.tasks.unshift(task);

        // 保存到LocalStorage
        this.saveToStorage();

        // 重新渲染界面
        this.render();
    }

    /**
     * 处理任务列表点击事件（事件委托）
     * @param {Event} e - 点击事件对象
     */
    handleTaskListClick(e) {
        // 获取点击的目标元素
        const target = e.target;

        // 1. 点击删除按钮
        if (target.classList.contains('task-delete')) {
            const taskId = parseInt(target.dataset.id);
            this.deleteTask(taskId);
            return;
        }

        // 2. 点击任务文本或checkbox（切换完成状态）
        if (target.classList.contains('task-text') ||
            target.classList.contains('task-checkbox')) {
            // 获取任务项元素
            const taskItem = target.closest('.task-item');
            if (taskItem) {
                const taskId = parseInt(taskItem.dataset.id);
                this.toggleTask(taskId);
            }
            return;
        }
    }

    /**
     * 删除任务
     * @param {number} id - 任务ID
     */
    deleteTask(id) {
        // 找到要删除的任务元素
        const taskElement = document.querySelector(`[data-id="${id}"]`);

        if (taskElement) {
            // 添加淡出动画
            taskElement.classList.add('fade-out');

            // 等待动画完成后删除
            setTimeout(() => {
                // 从数组中删除任务
                this.tasks = this.tasks.filter(task => task.id !== id);

                // 保存到LocalStorage
                this.saveToStorage();

                // 重新渲染界面
                this.render();
            }, 300); // 动画持续时间
        }
    }

    /**
     * 切换任务完成状态
     * @param {number} id - 任务ID
     */
    toggleTask(id) {
        // 找到任务
        const task = this.tasks.find(task => task.id === id);

        if (task) {
            // 切换完成状态
            task.completed = !task.completed;

            // 保存到LocalStorage
            this.saveToStorage();

            // 重新渲染界面
            this.render();
        }
    }

    /**
     * 筛选任务
     * @param {string} filter - 筛选类型：'all' | 'active' | 'completed'
     */
    filterTasks(filter) {
        // 更新当前筛选状态
        this.currentFilter = filter;

        // 更新筛选按钮的激活状态
        this.elements.filterBtns.forEach(btn => {
            if (btn.dataset.filter === filter) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });

        // 重新渲染界面
        this.render();
    }

    /**
     * 获取过滤后的任务列表
     * @returns {Array} - 过滤后的任务数组
     */
    getFilteredTasks() {
        switch (this.currentFilter) {
            case 'active':
                // 返回未完成的任务
                return this.tasks.filter(task => !task.completed);

            case 'completed':
                // 返回已完成的任务
                return this.tasks.filter(task => task.completed);

            case 'all':
            default:
                // 返回所有任务
                return this.tasks;
        }
    }

    /**
     * 渲染界面
     * - 渲染任务列表
     * - 更新统计信息
     */
    render() {
        this.renderTasks();
        this.updateStats();
    }

    /**
     * 渲染任务列表
     */
    renderTasks() {
        // 获取过滤后的任务
        const filteredTasks = this.getFilteredTasks();

        // 如果没有任务，显示空状态
        if (filteredTasks.length === 0) {
            this.renderEmptyState();
            return;
        }

        // 使用map生成任务HTML
        const tasksHTML = filteredTasks.map(task => this.createTaskHTML(task)).join('');

        // 更新DOM
        this.elements.taskList.innerHTML = tasksHTML;
    }

    /**
     * 创建任务HTML
     * @param {Object} task - 任务对象
     * @returns {string} - 任务的HTML字符串
     */
    createTaskHTML(task) {
        // 解构任务对象（ES6+特性）
        const { id, text, completed } = task;

        // 使用模板字符串生成HTML（ES6+特性）
        return `
            <li class="list-group-item task-item ${completed ? 'completed' : ''}" data-id="${id}">
                <div class="task-checkbox">
                    ${completed ? '✓' : ''}
                </div>
                <span class="task-text">${this.escapeHTML(text)}</span>
                <button class="task-delete" data-id="${id}">删除</button>
            </li>
        `;
    }

    /**
     * 渲染空状态
     */
    renderEmptyState() {
        let emptyMessage = '';

        switch (this.currentFilter) {
            case 'active':
                emptyMessage = '🎉 太棒了！所有任务都已完成！';
                break;
            case 'completed':
                emptyMessage = '😊 还没有完成任何任务，加油！';
                break;
            case 'all':
            default:
                emptyMessage = '暂无任务，添加你的第一个待办事项吧！';
        }

        this.elements.taskList.innerHTML = `
            <li class="list-group-item text-center empty-state">
                <div class="py-5">
                    <svg width="80" height="80" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" class="text-muted mb-3">
                        <circle cx="12" cy="12" r="10"></circle>
                        <path d="M12 6v6l4 2"></path>
                    </svg>
                    <p class="text-muted mb-0">${emptyMessage}</p>
                </div>
            </li>
        `;
    }

    /**
     * 更新统计信息
     */
    updateStats() {
        // 计算统计数据
        const total = this.tasks.length;
        const completed = this.tasks.filter(task => task.completed).length;
        const active = total - completed;
        const percentage = total > 0 ? Math.round((completed / total) * 100) : 0;

        // 更新DOM
        this.elements.totalCount.textContent = total;
        this.elements.completedCount.textContent = completed;
        this.elements.activeCount.textContent = active;
        this.elements.progressBar.style.width = `${percentage}%`;
        this.elements.progressBar.setAttribute('aria-valuenow', percentage);
    }

    /**
     * 保存到LocalStorage
     */
    saveToStorage() {
        const success = Storage.saveTasks(this.tasks);

        if (!success) {
            console.error('保存失败');
        }
    }

    /**
     * 转义HTML特殊字符（防止XSS攻击）
     * @param {string} text - 要转义的文本
     * @returns {string} - 转义后的文本
     */
    escapeHTML(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// --------------------------------------------
// 深入理解：事件委托
// --------------------------------------------

/**
 * 🔍 深入理解：为什么使用事件委托？
 *
 * 问题：
 * 如果有100个任务，每个任务都绑定点击事件，就有100个事件监听器。
 * 当任务增删时，需要手动管理这些监听器，容易出错。
 *
 * 解决方案：事件委托
 * 原理：利用事件冒泡机制，在父元素上监听子元素的事件。
 *
 * 优势：
 * 1. 性能更好：只有一个事件监听器
 * 2. 动态元素：新增的任务自动有事件处理
 * 3. 代码简洁：不需要为每个元素绑定事件
 *
 * 实现：
 * 1. 在父元素（taskList）上监听点击事件
 * 2. 通过e.target判断点击的是哪个子元素
 * 3. 执行对应的操作
 */

// --------------------------------------------
// 深入理解：数据驱动UI
// --------------------------------------------

/**
 * 🔍 深入理解：数据驱动UI的设计模式
 *
 * 核心思想：
 * UI是数据的映射，数据改变 → UI自动更新
 *
 * 流程：
 * 1. 用户操作（点击按钮）
 * 2. 更新数据（this.tasks数组）
 * 3. 保存数据（LocalStorage）
 * 4. 渲染UI（render方法）
 *
 * 优势：
 * 1. 逻辑清晰：数据和UI分离
 * 2. 易于维护：数据改变，UI自动同步
 * 3. 易于测试：可以单独测试数据逻辑
 *
 * 不直接操作DOM的原因：
 * - 直接操作DOM容易出现数据和UI不同步
 * - 难以追踪状态变化
 * - 代码耦合度高，难以维护
 */

// --------------------------------------------
// 应用启动
// --------------------------------------------

// 等待DOM加载完成后启动应用
document.addEventListener('DOMContentLoaded', () => {
    // 创建TodoApp实例（应用自动初始化）
    const app = new TodoApp();

    // 开发模式：将app实例暴露到全局（方便调试）
    if (typeof window !== 'undefined') {
        window.todoApp = app;
        console.log('💡 提示：可以在控制台通过 window.todoApp 访问应用实例');
    }
});
