# 离线阶段
## 模块二
### SplitTool
用到了cppjieba的cut（）-> 分词函数、vector
#### vector
[[2.类与对象#C++动态数组]]

#### 虚析构函数
[[8.多态#析构函数设为虚函数（重点）]]

### SplitToolCppjieba
#### 用到了单例模式
[[2.类与对象#单例模式（重点*）]]

[[5.运算符重载#单例对象自动释放（重点*）]]



### Configuration
用到了string、set、map、fstream、目录流、单例模式

#### fstream文件输入输出流
[[3.C++输入输出流#按行读取]]
ifstream文件输入流 ifstream有一个getline函数（兼容c）
``` c++
ifstream ifs("test.cc");
//方法一，兼容C的写法，使用较少
char buff[100] = {0};
while(ifs.getline(buff,sizeof(buff))){
    cout << buff << endl;
    memset(buff,0,sizeof(buff));
}

//更方便，使用更多
string line;
while(getline(ifs,line)){
    cout << line << endl;
}
```

读取指定字节数
[[3.C++输入输出流#读取指定字节数的内容]]
![[Pasted image 20240728102019.png]]
```c++
void test0(){
    string filename = "test.cc";
    ifstream ifs(filename); 

    if(!ifs){
        cerr << "ifs open file fail!";
        return;
    }
    
    //读取一个文件的所有内容先要获取文件的大小
    //将游标放到了文件的最后（尾后）
    fs.seekg(0,std::ios::end);
    long length = ifs.tellg();//获取尾后下标，实际就是总的字符数
    cout << length << endl;

    char * pdata = new char[length + 1]();
    //需要将游标再放置到文件开头
    ifs.seekg(0,std::ios::beg);
    ifs.read(pdata,length);

    //content包含了文件的所有内容，包括空格、换行
    string content(pdata);
    cout << "content:" << content << endl;
    /* cout << pdata << endl; */
    ifs.close();
}
```

#### 单例模式
[[2.类与对象#单例模式（重点*）]]

[[5.运算符重载#单例对象自动释放（重点*）]]

### DictProducer
用到了 vector、string、map、unordered_map、set、fstream
