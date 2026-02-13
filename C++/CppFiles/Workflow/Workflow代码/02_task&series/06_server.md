```c++
// WaitGroup 让某个线程处于等待状态
#include <workflow/WFFacilities.h>
// WFHttpServer.h 包含server类的设计
#include <workflow/WFHttpServer.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
using std::cout;
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
void process(WFHttpTask *serverTask){
    //cout << "hello world!\n";
    // 做一个echo业务，收请求
    protocol::HttpRequest * req = serverTask->get_req();
    const void *body;
    size_t size;
    req->get_parsed_body(&body,&size);
    cout << "req body = " << static_cast<const char *>(body) << "\n";
    // 找到响应的地址
    protocol::HttpResponse * resp = serverTask->get_resp();
    resp->append_output_body(body,size);
    sleep(3);
}
int main(){
    signal(SIGINT,sigHandler);
    WFHttpServer server(process);
    if(server.start(12345) == 0){
        waitGroup.wait();
        server.stop();
    }
    else{
        perror("server start failed!");
        return -1;
    }
    cout << "finished!\n"; 
    return 0;
}
```