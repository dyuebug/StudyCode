// ============================================
// 阶段4综合项目 - 通用资源管理器
// 日期：2026-05-28（第24天）
// 难度：⭐⭐⭐⭐ 高级（综合应用）
// ============================================

/*
【项目目标】

实现一个通用的资源管理器（ResourceManager），综合运用阶段4所学知识：
1. 模板编程：支持任意类型的资源
2. 智能指针：自动管理资源生命周期
3. STL 容器：存储和管理资源
4. Lambda 表达式：实现资源过滤和查询
5. 移动语义：优化性能
6. 现代 C++ 语法：auto、范围for、nullptr

【应用场景】
- 游戏引擎：管理纹理、音频、模型等资源
- 数据库连接池：管理数据库连接
- 文件句柄管理：管理打开的文件
*/

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

// ============================================
// 第一部分：资源基类 ⭐⭐⭐
// ============================================

/*
【设计思路】

定义一个资源基类 Resource，所有资源都继承自它。
这样可以统一管理不同类型的资源。
*/

class Resource {
protected:
    string name;        // 资源名称
    string type;        // 资源类型
    size_t size;        // 资源大小（字节）

public:
    // 构造函数
    Resource(const string& name, const string& type, size_t size)
        : name(name), type(type), size(size) {
        cout << "创建资源: " << name << " (" << type << ")" << endl;
    }

    // 虚析构函数（重要！基类必须有虚析构函数）
    virtual ~Resource() {
        cout << "释放资源: " << name << " (" << type << ")" << endl;
    }

    // 获取资源信息
    string getName() const { return name; }
    string getType() const { return type; }
    size_t getSize() const { return size; }

    // 纯虚函数：加载资源（子类必须实现）
    virtual void load() = 0;

    // 纯虚函数：卸载资源（子类必须实现）
    virtual void unload() = 0;

    // 打印资源信息
    virtual void printInfo() const {
        cout << "资源名称: " << name << endl;
        cout << "资源类型: " << type << endl;
        cout << "资源大小: " << size << " 字节" << endl;
    }
};

// ============================================
// 第二部分：具体资源类 ⭐⭐⭐
// ============================================

// --------------------------------------------
// 2.1 纹理资源
// --------------------------------------------

class Texture : public Resource {
private:
    int width;
    int height;

public:
    Texture(const string& name, int width, int height)
        : Resource(name, "Texture", width * height * 4),  // RGBA 4字节
          width(width), height(height) {}

    void load() override {
        cout << "加载纹理: " << name << " (" << width << "x" << height << ")" << endl;
    }

    void unload() override {
        cout << "卸载纹理: " << name << endl;
    }

    void printInfo() const override {
        Resource::printInfo();
        cout << "分辨率: " << width << "x" << height << endl;
    }
};

// --------------------------------------------
// 2.2 音频资源
// --------------------------------------------

class Audio : public Resource {
private:
    int duration;  // 时长（秒）

public:
    Audio(const string& name, int duration)
        : Resource(name, "Audio", duration * 44100 * 2),  // 44.1kHz 立体声
          duration(duration) {}

    void load() override {
        cout << "加载音频: " << name << " (" << duration << "秒)" << endl;
    }

    void unload() override {
        cout << "卸载音频: " << name << endl;
    }

    void printInfo() const override {
        Resource::printInfo();
        cout << "时长: " << duration << " 秒" << endl;
    }
};

// --------------------------------------------
// 2.3 模型资源
// --------------------------------------------

class Model : public Resource {
private:
    int vertexCount;  // 顶点数

public:
    Model(const string& name, int vertexCount)
        : Resource(name, "Model", vertexCount * 32),  // 每个顶点32字节
          vertexCount(vertexCount) {}

    void load() override {
        cout << "加载模型: " << name << " (" << vertexCount << " 顶点)" << endl;
    }

    void unload() override {
        cout << "卸载模型: " << name << endl;
    }

    void printInfo() const override {
        Resource::printInfo();
        cout << "顶点数: " << vertexCount << endl;
    }
};

// ============================================
// 第三部分：资源管理器（核心） ⭐⭐⭐⭐⭐
// ============================================

/*
【设计要点】

1. 使用 map 存储资源：ID → shared_ptr<Resource>
2. 使用 shared_ptr 自动管理资源生命周期
3. 使用 Lambda 实现资源过滤和查询
4. 使用移动语义优化性能
*/

class ResourceManager {
private:
    // 资源存储：ID → 资源智能指针
    map<string, shared_ptr<Resource>> resources;

    // 统计信息
    size_t totalSize;      // 总大小
    size_t loadedCount;    // 已加载数量

public:
    // 构造函数
    ResourceManager() : totalSize(0), loadedCount(0) {
        cout << "资源管理器初始化" << endl;
    }

    // 析构函数
    ~ResourceManager() {
        cout << "资源管理器销毁，清理所有资源..." << endl;
        clear();
    }

    // --------------------------------------------
    // 3.1 添加资源
    // --------------------------------------------

    // 添加资源（使用移动语义）
    void addResource(const string& id, shared_ptr<Resource> resource) {
        // 检查 ID 是否已存在
        if (resources.count(id) > 0) {
            cout << "警告：资源 ID 已存在: " << id << endl;
            return;
        }

        // 添加资源
        resources[id] = move(resource);  // 使用移动语义
        totalSize += resources[id]->getSize();
        loadedCount++;

        cout << "添加资源: " << id << " -> " << resources[id]->getName() << endl;
    }

    // 便捷方法：创建并添加纹理
    void addTexture(const string& id, const string& name, int width, int height) {
        auto texture = make_shared<Texture>(name, width, height);
        addResource(id, move(texture));
    }

    // 便捷方法：创建并添加音频
    void addAudio(const string& id, const string& name, int duration) {
        auto audio = make_shared<Audio>(name, duration);
        addResource(id, move(audio));
    }

    // 便捷方法：创建并添加模型
    void addModel(const string& id, const string& name, int vertexCount) {
        auto model = make_shared<Model>(name, vertexCount);
        addResource(id, move(model));
    }

    // --------------------------------------------
    // 3.2 查询资源
    // --------------------------------------------

    // 根据 ID 获取资源
    shared_ptr<Resource> getResource(const string& id) {
        auto it = resources.find(id);
        if (it != resources.end()) {
            return it->second;
        }
        return nullptr;  // 未找到
    }

    // 检查资源是否存在
    bool hasResource(const string& id) const {
        return resources.count(id) > 0;
    }

    // 获取资源数量
    size_t getResourceCount() const {
        return resources.size();
    }

    // --------------------------------------------
    // 3.3 删除资源
    // --------------------------------------------

    // 删除指定资源
    void removeResource(const string& id) {
        auto it = resources.find(id);
        if (it != resources.end()) {
            totalSize -= it->second->getSize();
            loadedCount--;
            cout << "删除资源: " << id << endl;
            resources.erase(it);
        } else {
            cout << "警告：资源不存在: " << id << endl;
        }
    }

    // 清空所有资源
    void clear() {
        cout << "清空所有资源..." << endl;
        resources.clear();
        totalSize = 0;
        loadedCount = 0;
    }

    // --------------------------------------------
    // 3.4 资源过滤（使用 Lambda）⭐⭐⭐⭐⭐
    // --------------------------------------------

    // 根据类型过滤资源
    vector<shared_ptr<Resource>> filterByType(const string& type) {
        vector<shared_ptr<Resource>> result;

        // 使用 Lambda 表达式过滤
        for (const auto& pair : resources) {
            if (pair.second->getType() == type) {
                result.push_back(pair.second);
            }
        }

        return result;
    }

    // 根据自定义条件过滤（使用 Lambda 参数）
    template<typename Predicate>
    vector<shared_ptr<Resource>> filter(Predicate pred) {
        vector<shared_ptr<Resource>> result;

        for (const auto& pair : resources) {
            if (pred(pair.second)) {
                result.push_back(pair.second);
            }
        }

        return result;
    }

    // --------------------------------------------
    // 3.5 统计信息
    // --------------------------------------------

    // 打印所有资源
    void printAllResources() const {
        cout << string(60, '=') << endl;
        cout << "所有资源列表" << endl;
        cout << string(60, '=') << endl;

        if (resources.empty()) {
            cout << "（无资源）" << endl;
            return;
        }

        for (const auto& pair : resources) {
            cout << "\nID: " << pair.first << endl;
            pair.second->printInfo();
            cout << string(60, '-') << endl;
        }
    }

    // 打印统计信息
    void printStatistics() const {
        cout << string(60, '=') << endl;
        cout << "资源统计信息" << endl;
        cout << string(60, '=') << endl;

        cout << "资源总数: " << resources.size() << endl;
        cout << "已加载数: " << loadedCount << endl;
        cout << "总大小: " << totalSize << " 字节 ("
             << totalSize / 1024.0 << " KB)" << endl;

        // 按类型统计
        map<string, int> typeCount;
        for (const auto& pair : resources) {
            typeCount[pair.second->getType()]++;
        }

        cout << "\n按类型统计:" << endl;
        for (const auto& pair : typeCount) {
            cout << "  " << pair.first << ": " << pair.second << " 个" << endl;
        }
    }

    // --------------------------------------------
    // 3.6 批量操作
    // --------------------------------------------

    // 加载所有资源
    void loadAll() {
        cout << "加载所有资源..." << endl;
        for (auto& pair : resources) {
            pair.second->load();
        }
    }

    // 卸载所有资源
    void unloadAll() {
        cout << "卸载所有资源..." << endl;
        for (auto& pair : resources) {
            pair.second->unload();
        }
    }

    // 加载指定类型的资源
    void loadByType(const string& type) {
        cout << "加载类型为 " << type << " 的资源..." << endl;
        for (auto& pair : resources) {
            if (pair.second->getType() == type) {
                pair.second->load();
            }
        }
    }
};

// ============================================
// 第四部分：测试和演示 ⭐⭐⭐⭐⭐
// ============================================

void demo1_basicUsage() {
    cout << "\n" << string(60, '=') << endl;
    cout << "演示1：基本使用" << endl;
    cout << string(60, '=') << endl;

    // 创建资源管理器
    ResourceManager manager;

    // 添加资源
    manager.addTexture("tex1", "player.png", 512, 512);
    manager.addTexture("tex2", "enemy.png", 256, 256);
    manager.addAudio("audio1", "bgm.mp3", 180);
    manager.addModel("model1", "character.obj", 10000);

    // 打印统计信息
    manager.printStatistics();

    // 打印所有资源
    manager.printAllResources();
}

void demo2_queryAndFilter() {
    cout << "\n" << string(60, '=') << endl;
    cout << "演示2：查询和过滤" << endl;
    cout << string(60, '=') << endl;

    ResourceManager manager;

    // 添加资源
    manager.addTexture("tex1", "player.png", 512, 512);
    manager.addTexture("tex2", "enemy.png", 256, 256);
    manager.addAudio("audio1", "bgm.mp3", 180);
    manager.addAudio("audio2", "sfx.wav", 5);
    manager.addModel("model1", "character.obj", 10000);

    // 查询单个资源
    cout << "\n查询资源 tex1:" << endl;
    auto res = manager.getResource("tex1");
    if (res) {
        res->printInfo();
    }

    // 按类型过滤
    cout << "\n过滤所有纹理资源:" << endl;
    auto textures = manager.filterByType("Texture");
    cout << "找到 " << textures.size() << " 个纹理" << endl;
    for (const auto& tex : textures) {
        cout << "  - " << tex->getName() << endl;
    }

    // 使用 Lambda 自定义过滤：查找大于 1MB 的资源
    cout << "\n过滤大于 1MB 的资源:" << endl;
    auto largeResources = manager.filter([](const shared_ptr<Resource>& res) {
        return res->getSize() > 1024 * 1024;  // 1MB
    });
    cout << "找到 " << largeResources.size() << " 个大资源" << endl;
    for (const auto& res : largeResources) {
        cout << "  - " << res->getName() << " ("
             << res->getSize() / 1024.0 / 1024.0 << " MB)" << endl;
    }
}

void demo3_loadAndUnload() {
    cout << "\n" << string(60, '=') << endl;
    cout << "演示3：加载和卸载" << endl;
    cout << string(60, '=') << endl;

    ResourceManager manager;

    // 添加资源
    manager.addTexture("tex1", "player.png", 512, 512);
    manager.addAudio("audio1", "bgm.mp3", 180);
    manager.addModel("model1", "character.obj", 10000);

    // 加载所有资源
    cout << "\n加载所有资源:" << endl;
    manager.loadAll();

    // 卸载所有资源
    cout << "\n卸载所有资源:" << endl;
    manager.unloadAll();

    // 按类型加载
    cout << "\n只加载纹理资源:" << endl;
    manager.loadByType("Texture");
}

void demo4_smartPointerDemo() {
    cout << "\n" << string(60, '=') << endl;
    cout << "演示4：智能指针自动管理" << endl;
    cout << string(60, '=') << endl;

    {
        cout << "进入作用域..." << endl;
        ResourceManager manager;

        manager.addTexture("tex1", "test.png", 256, 256);
        manager.addAudio("audio1", "test.mp3", 60);

        cout << "\n资源管理器即将离开作用域..." << endl;
    }  // manager 析构，自动释放所有资源

    cout << "已离开作用域，资源已自动释放" << endl;
}

void demo5_moveSemantics() {
    cout << "\n" << string(60, '=') << endl;
    cout << "演示5：移动语义优化" << endl;
    cout << string(60, '=') << endl;

    ResourceManager manager;

    // 创建资源并移动到管理器
    auto texture = make_shared<Texture>("large_texture.png", 2048, 2048);
    cout << "引用计数（移动前）: " << texture.use_count() << endl;

    manager.addResource("tex1", move(texture));
    cout << "引用计数（移动后）: " << (texture ? texture.use_count() : 0) << endl;

    // 获取资源
    auto res = manager.getResource("tex1");
    cout << "引用计数（获取后）: " << res.use_count() << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "阶段4综合项目 - 通用资源管理器" << endl;
    cout << "============================================" << endl;

    // 演示1：基本使用
    demo1_basicUsage();

    // 演示2：查询和过滤
    demo2_queryAndFilter();

    // 演示3：加载和卸载
    demo3_loadAndUnload();

    // 演示4：智能指针自动管理
    demo4_smartPointerDemo();

    // 演示5：移动语义优化
    demo5_moveSemantics();

    cout << "\n" << string(60, '=') << endl;
    cout << "项目演示完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}

/*
==============================================
🎓 项目总结
==============================================

【综合运用的技术】

1. ✅ 面向对象编程
   - 继承：Resource 基类 → Texture/Audio/Model 子类
   - 多态：虚函数、纯虚函数
   - 封装：private/public 访问控制

2. ✅ 智能指针
   - shared_ptr：共享资源所有权
   - make_shared：高效创建智能指针
   - 自动内存管理：离开作用域自动释放

3. ✅ STL 容器
   - map：存储 ID → 资源映射
   - vector：存储过滤结果
   - 迭代器：遍历容器

4. ✅ Lambda 表达式
   - 作为过滤条件
   - 捕获外部变量
   - 简化代码逻辑

5. ✅ 模板编程
   - 函数模板：filter<Predicate>
   - 泛型编程：支持任意谓词

6. ✅ 移动语义
   - std::move：转移所有权
   - 避免不必要的拷贝
   - 提升性能

7. ✅ 现代 C++ 语法
   - auto：自动类型推导
   - 范围 for：简化遍历
   - nullptr：空指针字面量
   - override：明确重写

【设计模式】

1. RAII（资源获取即初始化）
   - 构造函数获取资源
   - 析构函数释放资源
   - 利用对象生命周期管理资源

2. 工厂模式
   - addTexture/addAudio/addModel 便捷方法
   - 封装对象创建过程

3. 策略模式
   - filter 方法接受 Lambda 作为策略
   - 灵活的过滤条件

【性能优化】

1. 使用 shared_ptr 避免深拷贝
2. 使用 move 语义转移所有权
3. 使用 map 实现 O(log n) 查找
4. 使用引用避免不必要的拷贝

【实际应用】

这个资源管理器可以应用于：
- 游戏引擎：管理游戏资源
- 多媒体应用：管理音视频资源
- 数据库连接池：管理数据库连接
- 文件系统：管理文件句柄

【扩展方向】

1. 添加资源缓存机制
2. 实现异步加载
3. 添加资源依赖管理
4. 实现资源热重载
5. 添加资源压缩/解压

==============================================
*/
