```c++
#include "nlohmann/json.hpp"
#include <iostream>
using namespace std;
int main(){
    char arr[] = "[\"hello\",123,{\"key1\":\"value1\"}]";
    nlohmann::json json_object = nlohmann::json::parse(arr);
    cout << "[0] = " << json_object[0] << "\n";
    cout << "[1] = " << json_object[1] << "\n";
    cout << "[2][key] = " << json_object[2]["key1"] << "\n";
}
```
