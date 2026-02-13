```c++
// WaitGroup 让某个线程处于等待状态
#include <workflow/WFFacilities.h>
#include <iostream>
#include <signal.h>
using std::cerr;
using std::cout;
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
void redisCallback(WFRedisTask * redisTask){
    // 检查报错
    int state = redisTask->get_state(); //获取状态
	int error = redisTask->get_error(); //获取errno
	switch (state){
	case WFT_STATE_SYS_ERROR:
		cerr << "system error: " << strerror(error) << "\n";
		break;
	case WFT_STATE_DNS_ERROR:
		cerr << "DNS error: " << gai_strerror(error) << "\n";
		break;
	case WFT_STATE_SSL_ERROR:
		cerr <<"SSL error: " << error << "\n";
		break;
	case WFT_STATE_TASK_ERROR:
		cerr <<"Task error: " << error << "\n"; 
		break;
	case WFT_STATE_SUCCESS:
		break;
	} //只能检查连接错误，不能检查业务的语法错误
    protocol::RedisResponse * resp = redisTask->get_resp();
    protocol::RedisValue result; //用来保存redis返回的结果
    resp->get_result(result); // 获取结果 RedisValue
    if(result.is_error()){
        state = WFT_STATE_TASK_ERROR;
        cerr <<"Redis error: " << error << "\n"; 
    }
	if (state != WFT_STATE_SUCCESS){
		cerr << "Failed. Press Ctrl-C to exit.\n";
		return;
	}
    if(result.is_string()){
        cerr << "result = " << result.string_value() << "\n";
    }
    else if(result.is_array()){
        cerr << "result is array \n";
        for(int i = 0; i < result.arr_size(); ++i){
            cerr << "i = " << i
                 << ", arr[i] = " << result.arr_at(i).string_value() << "\n";
        }
    }
}
int main(){
    signal(SIGINT,sigHandler);
    // 创建任务
    WFRedisTask * redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,redisCallback);
    // 设置任务的属性
    protocol::RedisRequest * req = redisTask->get_req();
    //req->set_request("get",{"57key"});
    req->set_request("hgetall",{"57table"});
    // 启动任务
    redisTask->start();
    waitGroup.wait();
    cout << "finished!\n"; 
    return 0;
}
```