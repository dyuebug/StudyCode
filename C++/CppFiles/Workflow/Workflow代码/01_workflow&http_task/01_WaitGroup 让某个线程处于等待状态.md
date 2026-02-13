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
    waitGroup.wait();
    cout << "finished!\n"; 
    return 0;
}
```
