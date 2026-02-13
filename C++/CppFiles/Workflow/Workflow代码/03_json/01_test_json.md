```c++
#include "nlohmann/json.hpp"
#include <iostream>
using std::cout;
int main(){
    nlohmann::json json_object; // json_object可以是object或者array
    json_object["key1"] = "value1"; // json_object 是object类型，"key1"-->"value1"
    json_object["key2"] = 1234; //"key2"-->1234
    json_object["key3"]["key4"] = 12345;
    // dump方法 负责序列化
    cout << json_object.dump() << "\n";
}
```