```c++
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
#include <iostream>
#include <map>
#include <string>
#include <signal.h>
using std::cout;
using std::string;
using std::map;
using std::pair;
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int signum){
    cout << "done!\n";
    waitGroup.done();
}
int main(){
    signal(SIGINT,sigHandler);

    wfrest::HttpServer server;
    server.GET("/path",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        resp->append_output_body("GET");
    });
    server.POST("/path",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        resp->append_output_body("POST");
    });
    server.POST("/login",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp, SeriesWork * series){
        map<string,string> queryList = req->query_list();
        for(auto pair:queryList){
            cout << "first = " << pair.first << " second = " << pair.second << "\n";
        }
        string password = queryList["password"];
        WFRedisTask * redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379",10,
            [resp,password](WFRedisTask * redisTask){
                protocol::RedisResponse * respRedis = redisTask->get_resp();
                protocol::RedisValue result;
                respRedis->get_result(result);
                if(result.is_string() && (result.string_value() == password)){
                    //cout << "login success\n";
                    resp->String("login success");
                }
                else{
                    //cout << "login fail\n";
                    resp->String("login fail");
                }
            }
        );
        redisTask->get_req()->set_request("HGET",{"57user",queryList["username"]});
        series->push_back(redisTask);
    });
    server.GET("/login1",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        resp->File("postform.html"); // 部署一个静态资源服务器
    });
    server.POST("/login1",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        map<string,string> formKV = req->form_kv();
        for(auto pair:formKV){
            cout << "first = " << pair.first << " second = " << pair.second << "\n";
        }
    });
    server.POST("/form",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        map<string,pair<string,string>> form = req->form();
        for(auto pair:form){
            cout << "key = " << pair.first 
                 << " value.first = " << pair.second.first
                 << " value.second = " << pair.second.second
                 << "\n";
        }
    });
    server.GET("/upload",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        resp->File("upload.html"); // 部署一个静态资源服务器
    });
    server.POST("/upload",[](const wfrest::HttpReq* req, wfrest::HttpResp *resp){
        map<string,pair<string,string>> form = req->form();
        for(auto pair:form){
            cout << "key = " << pair.first 
                 << " value.first = " << pair.second.first
                 << " value.second = " << pair.second.second
                 << "\n";
        }
    });
    if(server.track().start(12345) == 0){ // track 每次客户端请求都显示客户端的信息
        server.list_routes(); //列出支持的路径和方法
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