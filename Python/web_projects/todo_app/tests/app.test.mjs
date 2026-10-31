// TodoApp 的 DOM 交互测试（jsdom）
// 运行：npm test
//
// 说明：
// - 使用真实 index.html，但移除 <script> 标签，由测试手动加载 storage.js 与 app.js。
// - 每个用例创建独立的 jsdom 实例，LocalStorage 互不影响。

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import { JSDOM } from 'jsdom';

const htmlSource = readFileSync(new URL('../index.html', import.meta.url), 'utf8');
const storageSource = readFileSync(new URL('../storage.js', import.meta.url), 'utf8');
const appSource = readFileSync(new URL('../app.js', import.meta.url), 'utf8');

// app.js 末尾会在 DOMContentLoaded 时自动实例化；测试需要手动控制，先截掉。
const bootstrapMarker = "document.addEventListener('DOMContentLoaded'";
const appClassSource = appSource.slice(0, appSource.indexOf(bootstrapMarker));

function createApp() {
    const html = htmlSource.replace(/<script[\s\S]*?<\/script>/gi, '');
    const dom = new JSDOM(html, {
        url: 'http://localhost/',
        runScripts: 'outside-only',
        pretendToBeVisual: true
    });

    const { window } = dom;
    window.alert = () => {};

    let clock = 1000;
    window.Date.now = () => ++clock;

    window.eval(`${storageSource}\n;window.Storage = Storage;`);
    window.eval(`${appClassSource}\n;window.TodoApp = TodoApp;`);

    const app = new window.TodoApp();
    return { dom, window, app };
}

function click(element) {
    element.dispatchEvent(new element.ownerDocument.defaultView.MouseEvent('click', {
        bubbles: true,
        cancelable: true
    }));
}

function addTaskViaUi(window, app, text) {
    window.document.getElementById('taskInput').value = text;
    click(window.document.getElementById('addTaskBtn'));
    return app;
}

test('初始化时从 LocalStorage 恢复任务', () => {
    const first = createApp();
    addTaskViaUi(first.window, first.app, '已有任务');
    const saved = first.window.localStorage.getItem('todoApp_tasks');

    // 新实例模拟刷新页面
    const second = createApp();
    second.window.localStorage.setItem('todoApp_tasks', saved);
    const reloaded = new second.window.TodoApp();

    assert.equal(reloaded.tasks.length, 1);
    assert.equal(reloaded.tasks[0].text, '已有任务');
});

test('通过界面添加任务并更新统计与存储', () => {
    const { window, app } = createApp();

    addTaskViaUi(window, app, '学习测试');

    assert.equal(app.tasks.length, 1);
    assert.equal(app.tasks[0].text, '学习测试');
    assert.equal(window.document.querySelectorAll('.task-item').length, 1);
    assert.equal(window.document.getElementById('totalCount').textContent, '1');
    assert.equal(window.document.getElementById('activeCount').textContent, '1');
    assert.equal(window.document.getElementById('completedCount').textContent, '0');

    const saved = JSON.parse(window.localStorage.getItem('todoApp_tasks'));
    assert.equal(saved[0].text, '学习测试');
});

test('空输入不会添加任务，并标记输入框', () => {
    const { window, app } = createApp();

    addTaskViaUi(window, app, '   ');

    assert.equal(app.tasks.length, 0);
    assert.equal(window.document.querySelectorAll('.task-item').length, 0);
    assert.equal(window.document.getElementById('taskInput').classList.contains('is-invalid'), true);
});

test('切换完成状态会更新统计和进度条', () => {
    const { window, app } = createApp();
    addTaskViaUi(window, app, '任务A');

    app.toggleTask(app.tasks[0].id);

    assert.equal(app.tasks[0].completed, true);
    assert.equal(window.document.getElementById('completedCount').textContent, '1');
    assert.equal(window.document.getElementById('activeCount').textContent, '0');
    assert.equal(window.document.getElementById('progressBar').style.width, '100%');
    assert.equal(window.document.getElementById('progressBar').getAttribute('aria-valuenow'), '100');
});

test('筛选按钮只显示对应状态的任务', () => {
    const { window, app } = createApp();
    addTaskViaUi(window, app, '进行中');
    addTaskViaUi(window, app, '已完成');
    app.toggleTask(app.tasks[0].id);

    click(window.document.querySelector('[data-filter="active"]'));
    assert.equal(app.currentFilter, 'active');
    assert.equal(window.document.querySelectorAll('.task-item').length, 1);
    assert.match(window.document.getElementById('taskList').textContent, /进行中/);

    click(window.document.querySelector('[data-filter="completed"]'));
    assert.equal(app.currentFilter, 'completed');
    assert.equal(window.document.querySelectorAll('.task-item').length, 1);
    assert.match(window.document.getElementById('taskList').textContent, /已完成/);

    click(window.document.querySelector('[data-filter="all"]'));
    assert.equal(window.document.querySelectorAll('.task-item').length, 2);
});

test('点击任务文本通过事件委托切换状态', () => {
    const { window, app } = createApp();
    addTaskViaUi(window, app, '点击切换');

    click(window.document.querySelector('.task-text'));

    assert.equal(app.tasks[0].completed, true);
});

test('点击删除按钮移除任务并同步存储', () => {
    const { window, app } = createApp();
    addTaskViaUi(window, app, '待删除');
    // 删除有 300ms 动画，测试中改为同步执行
    window.setTimeout = (callback) => { callback(); return 0; };

    click(window.document.querySelector('.task-delete'));

    assert.equal(app.tasks.length, 0);
    assert.equal(window.document.querySelectorAll('.task-item').length, 0);
    assert.deepEqual(JSON.parse(window.localStorage.getItem('todoApp_tasks')), []);
});

test('任务文本按 HTML 转义，避免 XSS', () => {
    const { window, app } = createApp();

    addTaskViaUi(window, app, '<img src=x onerror="window.hacked=true">');

    const list = window.document.getElementById('taskList');
    assert.equal(list.querySelector('img'), null);
    assert.match(list.textContent, /<img src=x/);
    assert.equal(window.hacked, undefined);
});

test('统计数字随任务数量变化', () => {
    const { window, app } = createApp();
    addTaskViaUi(window, app, 'A');
    addTaskViaUi(window, app, 'B');
    addTaskViaUi(window, app, 'C');
    app.toggleTask(app.tasks[0].id);

    assert.equal(window.document.getElementById('totalCount').textContent, '3');
    assert.equal(window.document.getElementById('completedCount').textContent, '1');
    assert.equal(window.document.getElementById('activeCount').textContent, '2');
    assert.equal(window.document.getElementById('progressBar').style.width, '33%');
});
