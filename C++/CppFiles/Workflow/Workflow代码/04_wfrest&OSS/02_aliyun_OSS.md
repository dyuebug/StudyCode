```c++
#include <alibabacloud/oss/OssClient.h>
#include <iostream>
using namespace AlibabaCloud::OSS;
using std::string;
using std::cerr;
// 做一些准备工作
struct OSSInfo{
    string EndPoint = "oss-cn-wuhan-lr.aliyuncs.com";
    string AcessKeyID = "<YOUR_ACCESS_KEY_ID>";
    string AcessKeySecret = "<YOUR_ACCESS_KEY_SECRET>";
    string Bucket = "57mytest";
};
int main(void){
    OSSInfo ossinfo;
    /* 初始化网络等资源。*/
    InitializeSdk();
    // 创建一个配置对象
    ClientConfiguration conf; //默认构造表示客户端选择默认参数
    OssClient client(ossinfo.EndPoint,ossinfo.AcessKeyID,ossinfo.AcessKeySecret,conf);

    // 上传文件（Object）
    PutObjectOutcome outcome = client.PutObject(ossinfo.Bucket,"dir1/3.txt","3.txt");
    if(!outcome.isSuccess()){
        cerr << "Put Object fail, code = " << outcome.error().Code()
             << " message = " << outcome.error().Message() << "\n";
    }
    /* 释放网络等资源。*/
    ShutdownSdk();
    return 0;
}
```
