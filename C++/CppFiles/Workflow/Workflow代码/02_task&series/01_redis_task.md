```c++
// WaitGroup 让某个线程处于等待状态
#include <workflow/WFFacilities.h>
#include <iostream>
#include <signal.h>
using std::cout;
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
int main(){
    signal(SIGINT,sigHandler);
    // 创建任务
    WFRedisTask * redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,nullptr);
    // 设置任务的属性
    protocol::RedisRequest * req = redisTask->get_req();
    req->set_request("set",{"57key","8888"});
    // 启动任务
    redisTask->start();
    waitGroup.wait();
    cout << "finished!\n"; 
    return 0;
}
```