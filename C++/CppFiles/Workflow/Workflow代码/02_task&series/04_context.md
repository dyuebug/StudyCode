```c++
// WaitGroup 让某个线程处于等待状态
#include <workflow/WFFacilities.h>
#include <workflow/Workflow.h>
#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>
using std::cerr;
using std::cout;
using std::string;
// 自定义的类 SeriesContext
struct SeriesContext{
    int id;
    string name;
};
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
// void seriesCallback(const SeriesWork * series){
//     SeriesContext *context = static_cast<SeriesContext *>(series->get_context());
//     delete context;
//     cerr << "context is freed!\n";
// }
void redisCallback2(WFRedisTask * redisTask){
    cerr << "before callback\n";
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

    SeriesContext *context = static_cast<SeriesContext *>(series_of(redisTask)->get_context());
    cerr << "I am task2 callback\n";
    cerr << "current context id = " << context->id << " currrent name = " << context->name << "\n";
    context->id = 1002;
    context->name = "redisTask2";
    sleep(3);
    cerr << "after callback\n";
}
void redisCallback1(WFRedisTask * redisTask){
    cerr << "before callback\n";
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
    // 申请一片内存
    SeriesContext * context = new SeriesContext;
    context->id = 1001;
    context->name = "redisTask1";
    // set_context 将内存的首地址作为序列的属性存起来
    series_of(redisTask)->set_context(context);
    //series_of(redisTask)->set_callback(seriesCallback);
    series_of(redisTask)->set_callback([context](const SeriesWork * series){
        delete context;
        cerr << "context is freed in lambda\n";
    });
    // 在1号任务的回调函数中调用series_of 一定可以找到某个序列
    SeriesWork *series = series_of(redisTask);
    // 创建任务
    WFRedisTask * redisTask2 = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,redisCallback2);
    // 设置任务的属性
    protocol::RedisRequest * req2 = redisTask2->get_req();
    req2->set_request("get",{"57key"});
    // 往正在运行的序列中加入任务
    series->push_back(redisTask2);
    sleep(3);
    cerr << "after callback\n";
}
int main(){
    signal(SIGINT,sigHandler);
    // 创建任务
    WFRedisTask * redisTask1 = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,redisCallback1);
    // 设置任务的属性
    protocol::RedisRequest * req1 = redisTask1->get_req();
    req1->set_request("set",{"57key","8888"});
   
    // 启动任务
    redisTask1->start();
    waitGroup.wait();
    
    cout << "finished!\n"; 
    return 0;
}
```