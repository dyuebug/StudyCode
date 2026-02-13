```c++
#include "nlohmann/json.hpp"
#include <iostream>
using std::cout;
int main(){
    nlohmann::json json_object; // json_object可以是object或者array
    json_object.push_back("hello"); //json_object是array类型
    json_object[1] = "world";
    json_object[2]["key1"]="value1";
    // dump方法 负责序列化
    cout << json_object.dump() << "\n";
    cout << json_object[0] << "\n";
}
```