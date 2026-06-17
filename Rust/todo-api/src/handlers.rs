// ============================================
// Todo API - 请求处理函数
// 日期：2026-06-21（第25天）
// ============================================

use crate::models::{ApiResponse, CreateTodo, ErrorResponse, Todo};
use crate::state::AppState;
use actix_web::{web, HttpResponse, Responder};

// --------------------------------------------
// GET /todos - 获取所有任务
// --------------------------------------------

/// 获取所有任务
///
/// # 参数
/// - state: web::Data<AppState> - 共享状态（自动注入）
///
/// # 返回
/// - HttpResponse - JSON 响应，包含所有任务
///
/// # 示例响应
/// ```json
/// {
///   "success": true,
///   "data": [
///     {"id": 1, "title": "学习 Rust", "completed": false}
///   ]
/// }
/// ```
pub async fn get_all(state: web::Data<AppState>) -> impl Responder {
    // 1. 获取 Mutex 锁（阻塞，直到获取成功）
    //    lock() 返回 LockResult<MutexGuard<Vec<Todo>>>
    //    unwrap() 处理可能的锁中毒（PoisonError）
    let todos = state.lock().unwrap();

    // 2. 克隆数据（因为锁守卫不能跨 .await 点）
    //    Vec<Todo> 实现了 Clone，可以创建副本
    //    克隆后立即释放锁，减少锁持有时间
    let todos_clone = todos.clone();

    // 3. 锁守卫在这里离开作用域，自动释放锁

    // 4. 构造成功响应
    //    ApiResponse::success() 创建统一格式的响应
    let response = ApiResponse::success(todos_clone);

    // 5. 返回 JSON 响应
    //    HttpResponse::Ok() - HTTP 200 状态码
    //    .json() - 自动序列化为 JSON
    HttpResponse::Ok().json(response)
}

// --------------------------------------------
// POST /todos - 创建新任务
// --------------------------------------------

/// 创建新任务
///
/// # 参数
/// - state: web::Data<AppState> - 共享状态（自动注入）
/// - new_todo: web::Json<CreateTodo> - 请求体（自动解析 JSON）
///
/// # 返回
/// - HttpResponse - JSON 响应，包含新创建的任务
///
/// # 请求示例
/// ```json
/// {
///   "title": "学习 Actix-web"
/// }
/// ```
///
/// # 响应示例
/// ```json
/// {
///   "success": true,
///   "data": {
///     "id": 1,
///     "title": "学习 Actix-web",
///     "completed": false
///   }
/// }
/// ```
pub async fn create(
    state: web::Data<AppState>,
    new_todo: web::Json<CreateTodo>,
) -> impl Responder {
    // 1. 获取 Mutex 锁（可变访问）
    let mut todos = state.lock().unwrap();

    // 2. 生成新的 ID
    //    如果列表为空，ID = 1
    //    否则，ID = 最大 ID + 1
    let new_id = todos.iter().map(|t| t.id).max().unwrap_or(0) + 1;

    // 3. 创建新的 Todo 实例
    //    new_todo.title.clone() - 克隆字符串（因为 CreateTodo 被 web::Json 包装）
    let todo = Todo::new(new_id, new_todo.title.clone());

    // 4. 添加到列表
    todos.push(todo.clone());

    // 5. 锁守卫在这里离开作用域，自动释放锁

    // 6. 构造成功响应
    let response = ApiResponse::success(todo);

    // 7. 返回 JSON 响应（HTTP 201 Created）
    HttpResponse::Created().json(response)
}

// --------------------------------------------
// GET /todos/:id - 获取单个任务
// --------------------------------------------

/// 获取单个任务
///
/// # 参数
/// - state: web::Data<AppState> - 共享状态（自动注入）
/// - id: web::Path<u32> - 路径参数（自动提取）
///
/// # 返回
/// - HttpResponse - JSON 响应，包含任务或 404 错误
///
/// # 路径参数
/// - id: 任务的唯一标识符
///
/// # 成功响应示例
/// ```json
/// {
///   "success": true,
///   "data": {
///     "id": 1,
///     "title": "学习 Rust",
///     "completed": false
///   }
/// }
/// ```
///
/// # 错误响应示例（404）
/// ```json
/// {
///   "success": false,
///   "error": "Todo not found"
/// }
/// ```
pub async fn get_by_id(
    state: web::Data<AppState>,
    id: web::Path<u32>,
) -> impl Responder {
    // 1. 获取 Mutex 锁
    let todos = state.lock().unwrap();

    // 2. 提取路径参数
    //    web::Path<u32> 自动从 URL 中提取 id
    //    /todos/1 -> id = 1
    let todo_id = id.into_inner();

    // 3. 在列表中查找任务
    //    iter() - 创建迭代器
    //    find() - 查找满足条件的第一个元素
    //    |t| t.id == todo_id - 闭包，检查 id 是否匹配
    let todo = todos.iter().find(|t| t.id == todo_id);

    // 4. 根据查找结果返回响应
    match todo {
        Some(found_todo) => {
            // 找到了任务
            // clone() - 克隆任务（因为 found_todo 是引用）
            let response = ApiResponse::success(found_todo.clone());
            HttpResponse::Ok().json(response)
        }
        None => {
            // 没找到任务，返回 404
            let error = ErrorResponse::error("Todo not found");
            HttpResponse::NotFound().json(error)
        }
    }
}

// --------------------------------------------
// PUT /todos/:id - 更新任务
// --------------------------------------------

/// 更新任务
///
/// # 参数
/// - state: web::Data<AppState> - 共享状态（自动注入）
/// - id: web::Path<u32> - 路径参数（任务ID）
/// - update_data: web::Json<UpdateTodo> - 请求体（更新数据）
///
/// # 返回
/// - HttpResponse - JSON 响应，包含更新后的任务或 404 错误
///
/// # 请求示例
/// ```json
/// {
///   "title": "学习 Actix-web（已更新）",
///   "completed": true
/// }
/// ```
///
/// # 说明
/// - 支持部分更新（只更新提供的字段）
/// - title 和 completed 都是可选的
/// - 至少需要提供一个字段
pub async fn update(
    state: web::Data<AppState>,
    id: web::Path<u32>,
    update_data: web::Json<crate::models::UpdateTodo>,
) -> impl Responder {
    // 1. 获取 Mutex 锁（需要可变访问）
    let mut todos = state.lock().unwrap();

    // 2. 提取路径参数
    let todo_id = id.into_inner();

    // 3. 在列表中查找任务（可变引用）
    //    iter_mut() - 创建可变迭代器
    //    find() - 查找并返回可变引用
    let todo = todos.iter_mut().find(|t| t.id == todo_id);

    // 4. 根据查找结果进行更新
    match todo {
        Some(found_todo) => {
            // 找到了任务，进行更新

            // 5. 更新 title（如果提供了）
            //    if let Some(new_title) = ... - 如果 Option 是 Some
            //    clone() - 克隆新的标题字符串
            if let Some(new_title) = &update_data.title {
                found_todo.title = new_title.clone();
            }

            // 6. 更新 completed（如果提供了）
            if let Some(new_completed) = update_data.completed {
                found_todo.completed = new_completed;
            }

            // 7. 克隆更新后的任务（用于响应）
            let updated_todo = found_todo.clone();

            // 8. 锁守卫在这里离开作用域，自动释放锁

            // 9. 返回更新后的任务
            let response = ApiResponse::success(updated_todo);
            HttpResponse::Ok().json(response)
        }
        None => {
            // 没找到任务，返回 404
            let error = ErrorResponse::error("Todo not found");
            HttpResponse::NotFound().json(error)
        }
    }
}

// --------------------------------------------
// DELETE /todos/:id - 删除任务
// --------------------------------------------

/// 删除任务
///
/// # 参数
/// - state: web::Data<AppState> - 共享状态（自动注入）
/// - id: web::Path<u32> - 路径参数（任务ID）
///
/// # 返回
/// - HttpResponse - JSON 响应，确认删除或 404 错误
///
/// # 成功响应示例
/// ```json
/// {
///   "success": true,
///   "data": {
///     "id": 1,
///     "title": "学习 Rust",
///     "completed": false
///   }
/// }
/// ```
pub async fn delete(
    state: web::Data<AppState>,
    id: web::Path<u32>,
) -> impl Responder {
    // 1. 获取 Mutex 锁（需要可变访问）
    let mut todos = state.lock().unwrap();

    // 2. 提取路径参数
    let todo_id = id.into_inner();

    // 3. 在列表中查找任务的位置
    //    iter() - 创建迭代器
    //    position() - 返回第一个满足条件的元素的索引
    //    返回 Option<usize>
    let position = todos.iter().position(|t| t.id == todo_id);

    // 4. 根据查找结果进行删除
    match position {
        Some(index) => {
            // 找到了任务

            // 5. 从列表中移除任务
            //    remove(index) - 移除指定位置的元素
            //    返回被移除的元素（获取所有权）
            let removed_todo = todos.remove(index);

            // 6. 锁守卫在这里离开作用域，自动释放锁

            // 7. 返回被删除的任务
            let response = ApiResponse::success(removed_todo);
            HttpResponse::Ok().json(response)
        }
        None => {
            // 没找到任务，返回 404
            let error = ErrorResponse::error("Todo not found");
            HttpResponse::NotFound().json(error)
        }
    }
}

/*
【关键知识点】

1. async fn（异步函数）：
   - 所有 handler 必须是 async fn
   - 可以在函数内使用 .await
   - 返回 impl Responder（实现 Responder trait 的类型）

2. web::Data<T>（状态提取器）：
   - Actix-web 自动注入共享状态
   - Data<T> 内部是 Arc<T>，可以安全共享
   - 在配置路由时通过 .app_data() 传入

3. web::Json<T>（JSON 提取器）：
   - 自动解析请求体为 JSON
   - 自动反序列化为指定类型（需要 Deserialize trait）
   - 如果解析失败，自动返回 400 Bad Request

4. web::Path<T>（路径参数提取器）：
   - 自动从 URL 路径中提取参数
   - /todos/{id} -> web::Path<u32>
   - into_inner() 获取内部值
   - 支持元组：web::Path<(u32, String)>

5. impl Responder（响应类型）：
   - Responder 是一个 trait
   - HttpResponse、String、&str、Json<T> 都实现了它
   - impl Responder 表示返回任何实现该 trait 的类型

6. Mutex 锁的使用：
   - lock() 获取锁（阻塞操作）
   - unwrap() 处理锁中毒错误
   - 锁守卫（MutexGuard）离开作用域时自动释放锁
   - 为什么要克隆？因为锁守卫不能跨 .await 点

7. HTTP 状态码：
   - 200 OK: 成功（get_all, get_by_id, update, delete）
   - 201 Created: 创建成功（create）
   - 400 Bad Request: 请求无效（自动处理）
   - 404 Not Found: 资源不存在（get_by_id, update, delete）

8. ID 生成策略：
   - 简单的自增 ID
   - 使用 iter().map().max() 找到最大 ID
   - unwrap_or(0) 处理空列表情况
   - 生产环境可能需要更复杂的 ID 生成策略

9. 为什么需要 clone()？
   - new_todo.title.clone(): CreateTodo 被 Json 包装，需要克隆
   - todo.clone(): 需要在返回响应时使用，但 todos 列表也需要所有权
   - todos.clone(): 释放锁后仍需要数据

10. iter() vs iter_mut()：
    - iter(): 不可变迭代器，返回不可变引用 &T
    - iter_mut(): 可变迭代器，返回可变引用 &mut T
    - 更新操作需要使用 iter_mut()

11. find() vs position()：
    - find(): 返回 Option<&T>，找到元素的引用
    - position(): 返回 Option<usize>，找到元素的索引
    - 删除操作需要索引，所以用 position()

12. Vec::remove()：
    - remove(index): 移除指定位置的元素
    - 返回被移除的元素（获取所有权）
    - 后续元素会向前移动

13. Option 的 if let 模式：
    - if let Some(value) = option { ... }
    - 只有当 Option 是 Some 时才执行代码块
    - 用于部分更新（UpdateTodo）

14. match 表达式：
    - 用于处理 Option 和 Result
    - Some/None 分支处理不同情况
    - 404 错误在 None 分支返回

15. 多个 Extractor 组合：
    - update() 同时使用 web::Path 和 web::Json
    - 参数顺序不重要，Actix-web 自动匹配类型
    - 每个 Extractor 独立工作

16. RESTful 设计模式：
    - GET /todos/:id - 获取单个资源
    - PUT /todos/:id - 更新资源
    - DELETE /todos/:id - 删除资源
    - 路径参数表示资源标识符
*/
