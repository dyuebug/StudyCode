// storage.js 的单元测试
// 运行：npm test（或 node --test tests/storage.test.mjs），在 web_projects/todo_app 目录下

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import vm from 'node:vm';

const storageSource = readFileSync(new URL('../storage.js', import.meta.url), 'utf8');

/**
 * vm 中创建的对象/数组属于另一个 realm，原型不同，
 * assert.deepEqual（strict 模式）会因为原型不一致而失败。
 * 统一转成当前 realm 的普通对象再比较。
 */
function toPlain(value) {
    return JSON.parse(JSON.stringify(value));
}

/**
 * 创建假的 LocalStorage。
 * 默认行为与浏览器一致：只存字符串、不存在时返回 null。
 */
function createFakeStorage({ throwOnSet = false } = {}) {
    const data = new Map();

    return {
        throwOnSet,
        getItem(key) {
            return data.has(key) ? data.get(key) : null;
        },
        setItem(key, value) {
            if (this.throwOnSet) {
                throw new Error('QuotaExceededError');
            }
            data.set(key, String(value));
        },
        removeItem(key) {
            data.delete(key);
        },
        clear() {
            data.clear();
        },
        get length() {
            return data.size;
        },
        has(key) {
            return data.has(key);
        }
    };
}

/** 在 vm 沙箱中加载 storage.js，返回 Storage 类。 */
function loadStorage(localStorage) {
    const sandbox = {
        localStorage,
        console: { error() {}, warn() {}, log() {} }
    };
    vm.createContext(sandbox);
    vm.runInContext(
        `${storageSource}\n;globalThis.__Storage = Storage;`,
        sandbox,
        { filename: 'storage.js' }
    );
    return sandbox.__Storage;
}

test('saveTasks 后 loadTasks 返回相同数据', () => {
    const Storage = loadStorage(createFakeStorage());
    const tasks = [
        { id: 1, text: '学习 JavaScript', completed: false },
        { id: 2, text: '写测试', completed: true }
    ];

    assert.equal(Storage.saveTasks(tasks), true);
    assert.deepEqual(toPlain(Storage.loadTasks()), tasks);
});

test('没有存储数据时返回空数组', () => {
    const Storage = loadStorage(createFakeStorage());

    assert.deepEqual(toPlain(Storage.loadTasks()), []);
});

test('存储内容是损坏 JSON 时返回空数组', () => {
    const storage = createFakeStorage();
    storage.setItem(storageKey(), '{not valid json');
    const Storage = loadStorage(storage);

    assert.deepEqual(toPlain(Storage.loadTasks()), []);
});

test('存储内容不是数组时返回空数组', () => {
    const storage = createFakeStorage();
    storage.setItem(storageKey(), JSON.stringify({ id: 1 }));
    const Storage = loadStorage(storage);

    assert.deepEqual(toPlain(Storage.loadTasks()), []);
});

test('写入抛异常时 saveTasks 返回 false', () => {
    const Storage = loadStorage(createFakeStorage({ throwOnSet: true }));

    assert.equal(Storage.saveTasks([{ id: 1 }]), false);
});

test('clearStorage 删除已保存的数据', () => {
    const storage = createFakeStorage();
    const Storage = loadStorage(storage);
    Storage.saveTasks([{ id: 1 }]);

    assert.equal(Storage.clearStorage(), true);
    assert.equal(storage.has(storageKey()), false);
    assert.deepEqual(toPlain(Storage.loadTasks()), []);
});

test('isStorageAvailable 在可写环境下返回 true', () => {
    const Storage = loadStorage(createFakeStorage());

    assert.equal(Storage.isStorageAvailable(), true);
});

test('isStorageAvailable 在写入失败时返回 false', () => {
    const Storage = loadStorage(createFakeStorage({ throwOnSet: true }));

    assert.equal(Storage.isStorageAvailable(), false);
});

test('getStorageInfo 返回已用字节数与百分比', () => {
    const storage = createFakeStorage();
    const Storage = loadStorage(storage);
    Storage.saveTasks([{ id: 1, text: 'abc' }]);
    const raw = storage.getItem(storageKey());

    const info = Storage.getStorageInfo();

    assert.equal(info.used, raw.length);
    assert.equal(info.total, 5 * 1024 * 1024);
    assert.equal(info.percentage, Number(((raw.length / info.total) * 100).toFixed(2)));
});

test('getStorageInfo 在无数据时返回 0', () => {
    const Storage = loadStorage(createFakeStorage());

    assert.deepEqual(toPlain(Storage.getStorageInfo()), { used: 0, total: 5 * 1024 * 1024, percentage: 0 });
});

/** storage.js 中的键名是私有约定，测试独立声明一份，避免依赖实现细节的静态属性。 */
function storageKey() {
    return 'todoApp_tasks';
}
