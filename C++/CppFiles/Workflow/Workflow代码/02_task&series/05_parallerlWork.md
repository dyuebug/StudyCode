```c++
// WaitGroup 让某个线程处于等待状态
#include <workflow/WFFacilities.h>
#include <workflow/Workflow.h>
#include <iostream>
#include <string>
#include <vector>
#include <signal.h>
#include <unistd.h>
using std::cerr;
using std::cout;
using std::string;
using std::vector;
using std::cout;
struct SeriesContext{
    string url;
    size_t size;
};
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
void httpCallback(WFHttpTask * httpTask){
    const void *body;
    size_t size;
    httpTask->get_resp()->get_parsed_body(&body,&size);
    SeriesContext * context = static_cast<SeriesContext *>(series_of(httpTask)->get_context());
    // 说明任务已经完成了基本工作，将size填入context当中
    context->size = size;
    cout << "http callback url = " << context->url << " size = " << context->size << "\n";
}
void parallelCallback(const ParallelWork *parallel){
    // 并行任务的回调当中，可以访问所有子序列以及其context
    cout << "----------------------------------------------------\n";
    string key;
    size_t value = 0;
    for(int i = 0; i < parallel->size(); ++i){
        //series_at 获取并行任务里面的小序列
        SeriesContext * context = static_cast<SeriesContext *>(parallel->series_at(i)->get_context());
        if(context->size > value){
            key = context->url;
            value = context->size;
        }
        cout << "parallel callback url = " << context->url << " size = " << context->size << "\n";
        delete context;
    }

    WFRedisTask * redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,nullptr);
    redisTask->get_req()->set_request("SET",{key,std::to_string(value)});
    // 并行任务所在的大序列
    series_of(parallel)->push_back(redisTask);
}
int main(){
    signal(SIGINT,sigHandler);
    // 创建一个并行任务，空壳
    ParallelWork * parallel = Workflow::create_parallel_work(nullptr);
    vector<string> urls = {"http://www.jd.com", "http://www.taobao.com", "http://www.baidu.com"};
    for(int i = 0; i < 3; ++i){
        // 创建一个http任务
        WFHttpTask * httpTask = WFTaskFactory::create_http_task(urls[i],10,10,httpCallback);
        // 申请一个context 暂时存入了url，size还没有存储
        SeriesContext * context = new SeriesContext;
        context->url = urls[i];
        // 创建一个序列
        SeriesWork * series = Workflow::create_series_work(httpTask,nullptr);
        series->set_context(context);
        // 将序列加入到并行任务中
        parallel->add_series(series);
    }
    parallel->set_callback(parallelCallback);
    parallel->start();
    waitGroup.wait();
    cout << "finished!\n"; 
    return 0;
}
```