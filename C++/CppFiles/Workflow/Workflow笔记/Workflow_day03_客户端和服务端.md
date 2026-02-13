## 一些类型
![[Pasted image 20240724164514.png|400]]
### 写类型
![[Pasted image 20240724164546.png]]

### 读类型的指令需要用到回调函数
![[Pasted image 20240724164610.png]]

### redis的读类型的任务
![[Pasted image 20240724164647.png]]

### 先set 再get
![[Pasted image 20240724164859.png]]

### 第二种思路
![[Pasted image 20240724164933.png]]

## 序列机制
![[Pasted image 20240724164956.png]]
在 Workflow 框架中，**序列机制**（Sequence Mechanism）是用来组织和管理多个任务（Tasks）的一种方式，确保这些任务按照特定的顺序执行。这个机制在处理复杂的异步流程时特别有用，帮助开发者清晰地定义任务之间的执行顺序和依赖关系。

### 序列机制是什么？

Workflow 框架中的序列机制允许你将多个任务串联成一个“序列”（Sequence），这些任务会按照添加的顺序依次执行。每个任务完成后，框架会自动触发下一个任务的执行，直到整个序列中的所有任务完成。序列机制还支持错误处理和并行任务执行的能力。

### 为什么使用序列机制？

1. **顺序控制**: 在处理复杂的异步操作时，确保任务按照正确的顺序执行是非常重要的。序列机制简化了这种顺序控制。
    
2. **错误处理**: 你可以在序列中设置错误处理逻辑，当某个任务失败时，触发特定的错误处理任务。
    
3. **简化代码**: 使用序列机制可以使代码更清晰、更易于维护，因为任务的执行顺序和依赖关系被显式地定义和管理。
    
4. **并发控制**: 序列机制不仅支持顺序执行任务，还可以在需要时并发执行任务，提升效率。
    

### 如何使用序列机制？

以下是使用 Workflow 框架中的序列机制的基本步骤：

1. **创建任务**: 首先，你需要创建要加入序列的任务。任务可以是网络请求、数据库操作或自定义的任务。
    
2. **创建序列**: 使用 `WFTaskFactory::create_series_work` 创建一个序列对象。
    
3. **添加任务到序列**: 使用 `series_of(task)` 将任务添加到序列中。
    
4. **启动序列**: 启动序列以执行任务。
    

#### 示例代码

下面是一个简单的示例，展示如何在 Workflow 框架中使用序列机制：
```c++
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFacilities.h>
#include <iostream>

void task1_callback(WFMySQLTask *task) {
    std::cout << "Task 1 completed" << std::endl;
}

void task2_callback(WFMySQLTask *task) {
    std::cout << "Task 2 completed" << std::endl;
}

int main() {
    WFFacilities::WaitGroup waitGroup(1);

    // 创建任务
    WFMySQLTask *task1 = WFTaskFactory::create_mysql_task("mysql://user:password@localhost/db", 1, task1_callback);
    WFMySQLTask *task2 = WFTaskFactory::create_mysql_task("mysql://user:password@localhost/db", 1, task2_callback);

    // 创建序列
    auto *series = WFTaskFactory::create_series_work(task1, nullptr);

    // 将任务添加到序列
    series_of(task1)->push_back(task2);

    // 启动序列
    series->start();

    waitGroup.wait();
    return 0;
}
```
在这个示例中，我们创建了两个 MySQL 任务，并将它们添加到一个序列中。序列会依次执行这些任务，当所有任务完成时程序结束。

### 总结

Workflow 框架的序列机制提供了一种有效的方式来管理异步任务的执行顺序和依赖关系。它有助于处理复杂的异步操作，使代码更清晰、更易于维护，并且在处理错误和控制并发执行时非常有用。
###  序列执行之前，准备好所有的任务
![[Pasted image 20240724165045.png]]
### 任务的start方法底层用到了序列
![[Pasted image 20240724165131.png]]

### series_of
![[Pasted image 20240724165156.png]]

### 序列执行中，动态添加任务
![[Pasted image 20240724165233.png]]

### 在序列的任务之间共享数据
![[Pasted image 20240724165301.png]]

![[Pasted image 20240724165316.png]]

### 使用序列context解决内存泄漏问题
![[Pasted image 20240724165401.png]]

### 并行任务
![[Pasted image 20240724165437.png]]

### 实现并行任务
![[Pasted image 20240724165458.png]]
![[Pasted image 20240724165537.png]]
![[Pasted image 20240724165552.png]]

### 并行任务串联模型
![[Pasted image 20240724165631.png]]

### 总结
![[Pasted image 20240724165703.png]]


## 使用workflow充当服务端
![[Pasted image 20240724165741.png]]

### 客户端的任务逻辑和服务端有区别
![[Pasted image 20240724165803.png]]
### workflow针对服务端任务做了一个特殊设计
![[Pasted image 20240724165827.png]]

### server类
![[Pasted image 20240724170042.png]]

![[Pasted image 20240724170051.png]]

### Echo业务
![[Pasted image 20240724170210.png]]

### 登录业务
![[Pasted image 20240724170231.png]]
![[Pasted image 20240724170241.png]]
![[Pasted image 20240724170250.png]]

### 反向代理 静态资源服务器
![[Pasted image 20240724170310.png]]

## 总结
![[Pasted image 20240724170327.png]]
