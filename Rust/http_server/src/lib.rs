// ============================================
// 线程池实现
// 功能：管理固定数量的工作线程，处理任务队列
// ============================================

use std::sync::{mpsc, Arc, Mutex};
use std::thread;

// 线程池结构体
pub struct ThreadPool {
    workers: Vec<Worker>,           // 工作线程列表
    sender: mpsc::Sender<Job>,      // 任务发送端
}

// 任务类型：可以发送到线程的闭包
type Job = Box<dyn FnOnce() + Send + 'static>;

impl ThreadPool {
    /// 创建新的线程池
    ///
    /// size: 线程池中的线程数量
    ///
    /// # Panics
    ///
    /// 当 size 为 0 时会 panic
    pub fn new(size: usize) -> ThreadPool {
        assert!(size > 0);

        // 创建通道：用于在主线程和工作线程之间传递任务
        let (sender, receiver) = mpsc::channel();

        // Arc<Mutex<T>> 组合：
        // - Arc：多个线程共享所有权（原子引用计数）
        // - Mutex：互斥锁，保证同一时间只有一个线程访问
        let receiver = Arc::new(Mutex::new(receiver));

        let mut workers = Vec::with_capacity(size);

        for id in 0..size {
            // 创建工作线程
            workers.push(Worker::new(id, Arc::clone(&receiver)));
        }

        ThreadPool { workers, sender }
    }

    /// 执行任务
    ///
    /// f: 要执行的闭包
    pub fn execute<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        let job = Box::new(f);

        // 将任务发送到通道
        // 工作线程会从通道接收任务并执行
        self.sender.send(job).unwrap();
    }
}

// 工作线程结构体
struct Worker {
    id: usize,
    thread: thread::JoinHandle<()>,
}

impl Worker {
    /// 创建新的工作线程
    fn new(id: usize, receiver: Arc<Mutex<mpsc::Receiver<Job>>>) -> Worker {
        let thread = thread::spawn(move || loop {
            // 从通道接收任务
            // lock() 获取互斥锁，recv() 接收任务
            let job = receiver.lock().unwrap().recv().unwrap();

            println!("🔧 Worker {} 开始执行任务", id);

            // 执行任务
            job();

            println!("✅ Worker {} 完成任务", id);
        });

        Worker { id, thread }
    }
}
