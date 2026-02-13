# C++57期STL部分
## 什么是STL
C++ STL（Standard Template Library，标准模板库）是C++语言的一个重要组成部分，它提供了一组通用的类和函数，用于处理数据结构和算法。STL主要包括以下几个部分：

1. **容器（Containers）**：用于存储和管理数据。常见的容器包括：
    
    - **序列式容器**：如`vector`、`deque`、`list`
    - **关联式容器**：如`set`、`map`、`multiset`、`multimap`
    - **无序式容器**：如`unordered_set`、`unordered_map`
2. **迭代器（Iterators）**：用于遍历容器中的元素。迭代器类似于指针，允许对容器中的元素进行访问和操作。
    
3. **算法（Algorithms）**：STL提供了许多常用的算法函数，如排序、查找、复制、删除等。算法通常通过迭代器来操作容器中的元素。
    
4. **函数对象（Functors）**：函数对象是可以作为函数使用的对象，通常用于自定义算法中的操作行为。
    

### 为什么使用STL

1. **代码简洁**：STL提供了许多通用的数据结构和算法，减少了自己编写这些代码的需要。
2. **效率高**：STL中的数据结构和算法经过精心优化，性能通常非常高。
3. **可重用性**：STL中的组件是通用的，可以在不同的项目中重复使用。
4. **安全性**：STL通过泛型编程和类型安全，减少了运行时错误的发生。


## Day18

### 一、问题回顾

1、右值引用？

右值引用可以设别右值，但是不能识别左值，所以对右值可以做特殊的处理

```C++
int number = 10;
int &ref = number;

int &&rref = 10;//右值引用可以设别右值，但是不能识别左值，所以对右值可以做特殊的处理

String s1 = String("hello");

void func(String &&rhs)
{
    
}

int &&func()
{
    return 10;
}
```

2、移动构造函数？移动赋值函数

```C++
//String s2 = String("hello");
String(const String &rhs)
: _pstr(new char[strlen(rhs._pstr) + 1]())
{
    strcpy();
}

String(String &&rhs)
: _pstr(rhs._pstr)
{
    rhs._pstr = nullptr;
}


//s2 = String("hello")
String  &operator=(const String &rhs)
{
    if(this != &rhs)
    {
        delete [] _pstr;
        
        _pstr = new char[strlen(rhs._pstr) + 1]());//深拷贝
        strcpy();      
    }
    return *this;
}

//s2 = String("hello")
String  &operator=( String &&rhs)
{
    if(this != &rhs)
    {
        delete [] _pstr;
        
        _pstr = rhs._pstr;//浅拷贝
        rhs._pstr = nullptr;     
    }
    return *this;
}
```

3、std::move?

```C++
String s1("hello");
String s2("world");

std::move(s1);
cout << "s1 = " << s1 << endl;


s2 = std::move(s1);//移动赋值函数
cout << "s2 = " << s2 << endl;
cout << "s1 = " << s1 << endl;
```

4、RAII？

具备4大特征；利用栈对象的生命周期管理资源

5、对象语义？

不能复制与赋值。

```C++
class A
{
    //拷贝构造函数与赋值运算符函数删除
    A(const A &rhs)  = delete;
    A &operator=(const A &rhs)  = delete;
    
    //拷贝构造函数与赋值运算符函数设置为私有的
private：
    A(const A &rhs)；
     A &operator=(const A &rhs);
};

class NonCopyable
{
public:
    NonCopyable(const NonCopyable &rhs)  = delete;
    NonCopyable &operator=(const NonCopyable &rhs)  = delete; 
};

class A
: NonCopyable
{
    
};

A a;
A b = a;//error
A c;
c = a;//error
```

6、智能指针auto_ptr？

```C++
auto_ptr<int> ap(new int(10));

auto_ptr<int> ap2(ap);//在复制或者赋值的时候，底层已经发生了所有权的转移。
```

7、智能指针unique_ptr？

不能复制与赋值，但是具备移动语义。

```C++
unique_ptr<int> up(new int(10));
unique_ptr<int> up2(up);//error

unique_ptr<int> up3(new int(20));
up3 = up;//error

vector<unique_ptr<int>>  vec;
vec.push_back(std::move(up));
vec.push_back(unique_ptr<int>(new int(20)));
```

8、智能指针shared_ptr？

可以进行复制与赋值，引用技术的概念。

```C++
shared_ptr<int> sp(new int(10));
shared_ptr<int> sp2(sp);//ok

shared_ptr<int> sp3(new int(20));
sp3 = sp;//ok

vector<shared_ptr<int>>  vec;
vec.push_back(std::move(sp));
vec.push_back(shared_ptr<int>(new int(20)));
```

问题：循环引用？

9、智能指针weak_ptr？

```C++
weak_ptr<int> wp(new int(10));//error
```

10、删除器？

```C++
string msg("hello");
unique_ptr<FILE> up(fopen("wd.txt", "r+"));
```

11、智能指针的误用？

```C++
//使用了不同的智能指针托管了同一块堆空间（裸指针）

unique_ptr<int> up(new int(10));
unique_ptr<int> up2(new int(20));
up.reset(up2.get());
```

文本查询扩展作业布置

```C++
单词 + 词频;
struct Record
{
    string _word;
    int _fre;
};
vector<Record>;
map<string, int>;//单词词频
vector<string>;//每一行的内容
map<string, set<int>>;//用来存行号的

ifstream ifs("wd.txt");
getline(ifs, line)

//..... cjkdsafka heiqhe
```

## Day19

### 一、文本查询扩展作业讲解

Query_base

```C++
class Query_base 
{
    friend class Query;  
protected:
    typedef TextQuery::line_no line_no; // used in the eval functions
    virtual ~Query_base() { }
private:  
    virtual QueryResult eval(const TextQuery&) const = 0; 
	virtual std::string rep() const = 0;
};

```

Word_query

```C++
class WordQuery
: public Query_base 
{
    friend class Query; // Query uses the WordQuery constructor
    WordQuery(const std::string &s)
    : query_word(s)
    {
    }

    // concrete class: WordQuery defines all inherited pure virtual functions
    QueryResult eval(const TextQuery &t) const
    { 
		return t.query(query_word); 
    }

    std::string rep() const 
    {
        return query_word; 
    }

    std::string query_word;   // word for which to search 
};
```

get_file函数

```C++
该函数背后会读文件，将每一行的内容存放在shared_ptr<vector<string>> file中；
然后将单词与单词出现的行号存放在map<string, shared_ptr<set<size_t>>> wm存起来。（将其称为预处理操作）
```

查询某个单词的过程

```C++
查询某一个单词sought的时候，会构建一个Query对象，在Query的构造函数中会构建WordQuery的对象，然后用数据成员shared_ptr<Query_base> q这种类型的指针指向了派生类的对象；shared_ptr<Query_base> q(new WordQuery(sought));
然后在查询的时候会走两个函数，一个是eval，一个是rep，然后使用Query的对象走上述两个函数，在这两个函数中会用到基类的指针q->eval函数以及q->rep函数，就可以满足多态。在WordQuery的eval函数中会调用TextQuery的eval函数，最终query函数走的是TextQuery中的，在该函数中会将单词、行号、每一行的内容交给QueryResult的对象，最后将其打印出来。
```

查询两个单词同时出现的过程

```C++
通过键盘输入两个单词，分别交给sought1与sought2。接下来用
sought1构建了Query对象，也会有数据成员shared_ptr<Query_base> q(new WordQuery(sought1));
sought2构建了Query对象，也会有数据成员shared_ptr<Query_base> q(new WordQuery(sought2));

Query andq对象也会有数据成员shared_ptr<Query_base> q；此q会指向派生类类型的对象AndQuery；
shared_ptr<Query_base> q(new AndQuery(lhs, rhs));此处的lhs = Query(sought1),rhs = Query(sought2);
后续会使用andq对象走eval函数，然后该函数会体现多态，走到派生类AndQuery的eval函数中，在该函数中会将Query(sought1)与Query(sought2)的结果取交集，然后打印出来，这就是两个单词同时出现的行。
```

```C++
inline Query operator&(const Query &lhs, const Query &rhs)
{
    return shared_ptr<Query_base>(new AndQuery(lhs, rhs));//隐式转换
    //shared_ptr<Query_base>  t(new AndQuery(lhs, rhs));
}
Query(shared_ptr<Query_base> );

Query(std::shared_ptr<Query_base> query)
: q(query)
{ 

}

从shared_ptr<Query_base> ---->Query;
Point  pt = 10;//隐式转换  int-->Point   Point(10, 0)
int x = pt;//类型转换函数   operator int(){  return 10;}


QueryResult AndQuery::eval(const TextQuery& text) const
{  
    QueryResult left = lhs.eval(text), right = rhs.eval(text);

    shared_ptr<set<line_no> > ret_lines(new set<line_no>);  

    //取交集
    set_intersection(left.begin(), left.end(), 
                   right.begin(), right.end(),
                   inserter(*ret_lines, ret_lines->begin()));
    return QueryResult(rep(), ret_lines, left.get_file());
}

```

查询两个单词中至少出现一个情况

```C++
通过键盘输入两个单词，分别交给sought1与sought2。接下来用
sought1构建了Query对象，也会有数据成员shared_ptr<Query_base> q(new WordQuery(sought1));
sought2构建了Query对象，也会有数据成员shared_ptr<Query_base> q(new WordQuery(sought2));

Query orq对象也会有数据成员shared_ptr<Query_base> q；此q会指向派生类类型的对象OrQuery；
shared_ptr<Query_base> q(new OrQuery(lhs, rhs));此处的lhs = Query(sought1),rhs = Query(sought2);
后续会使用oq对象走eval函数，然后该函数会体现多态，走到派生类OrQuery的eval函数中，在该函数中会将Query(sought1)与Query(sought2)的结果取并集，然后打印出来，这就是两个单词至少出现一个的行。
       
inline Query operator|(const Query &lhs, const Query &rhs)
{
    return std::shared_ptr<Query_base>(new OrQuery(lhs, rhs));
}
```

查询某个单词在整篇文章中没有出现的行~hello

```C++
文章一共有10行；出现hello：1 3 5 7，那么没有出现hello是 0 2 4 6 8 9
sz就应该是10；
出现hello： 1 3 5 7
        beg
1 3 5 7
         end

0 2 4 6 8 9
for(size_t n = 0;n != 10; ++n)
{
    if(beg == end || *beg != n)
    {
        ret_lines->insert(n);
    }
    else if(beg != end)
        ++beg;
}
查询某个单词在整篇文章中不存在的方法是：将某个单词出现的情况与整个行号都找出来，然后取二者的差集，这就是某个单词没有出现的行号。
```

![image-20240624115234015](C++57期STL部分.assets/image-20240624115234015.png)

### 二、标准模板库

容器：数据结构。==**重要**==

- 序列式容器  vector、list
- 关联式容器  set、map
- 无序关联式容器  unordered_set、unordered_map

迭代器：理解为广义的指针。

算法：本质就是一个个函数，可以利用迭代器访问容器中的元素，对容器中的元素做处理。

适配器：起到适配的作用。

- 容器的适配器  stack、priority_queue
- 迭代器的适配器  
- 函数适配器   bind、mem_fn

函数对象：定制化操作。

空间配置器：管理空间的申请与释放。==**用法 + 原理 + 源码**==



### 三、序列式容器

![[Pasted image 20240818180547.png]]

#### 1、初始化
![[Pasted image 20240818180559.png]]


==总结：三种序列式容器，vector、deque、list都有五种初始化的方式。==

#### 2、遍历
![[Pasted image 20240818180612.png]]



==**注意：list不支持下标操作。**==

![[Pasted image 20240818180632.png]]


==总结：对于vector与deque而言，遍历的方式是完全一样，但是对于list而言，因为list不支持下标操作，所以list不能使用下标访问。而其他的遍历方式list是支持的==

#### 3、在尾部进行插入与删除

![[Pasted image 20240818180644.png]]


==总结：三种序列式容器在尾部进行插入与删除是完全一样的。==

#### 4、在头部进行插入与删除

![[Pasted image 20240818180654.png]]


==总结：deque与list可以在头部进行插入与删除，但是vector不行==

![[Pasted image 20240818180706.png]]


#### 5、vector的源码（==了解==）

![[Pasted image 20240818180721.png]]


##### 5.1、继承图

![[Pasted image 20240818180734.png]]


##### 5.2、源码

```C++
template <class _Tp, class _Alloc = __STL_DEFAULT_ALLOCATOR(_Tp) >
class vector : protected _Vector_base<_Tp, _Alloc> 
{
public:
  typedef _Tp value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
    
};
```

注意：vector的at函数与operator[]都可以进行随机访问，但是at的随机访问更加安全一些，具备范围检查。

#### 6、deque的源码（==了解==）

##### 6.1、继承图

![[Pasted image 20240818180751.png]]


##### 6.2、原理图

![[Pasted image 20240818180812.png]]


#### 7、序列式容器的insert操作（==重要==）

![[Pasted image 20240818180834.png]]
![[Pasted image 20240818180846.png]]


## Day20

### 一、问题回顾

1、序列式容器的初始化方式有哪几种？遍历的方式有哪几种？

2、序列式容器在尾部进行插入与删除的函数是什么？在头部进行插入与删除的函数是什么？

3、vector的底层元素是严格连续的吗？

4、deque的底层元素是严格连续的吗？

5、序列式容器在任意位置进行插入的函数是什么？有哪几种方式？



### 二、序列式容器

#### 1、vector迭代器失效（==重要==）

![[Pasted image 20240818180913.png]]

#### 2、vector的erase操作（==重要==）
![[Pasted image 20240818180929.png]]

#### 3、序列式容器的清空
![[Pasted image 20240818180948.png]]

==总结：三种序列式容器都有clear函数进行清空元素，以及获取元素个数的size函数。对于vector与deque而言，还有回收多余空间的函数shrink_to_fit函数。对于vector而言，还有记录容量大小的函数capcacity。==

#### 4、其他操作

swap函数：可以交换两个容器的内容。三个序列式容器都支持。

resize函数：可以更新容器中元素的个数。三个序列式容器都支持。

front函数：获取容器的第一个元素。三个序列式容器都支持。

back函数：获取容器的最后一个元素。三个序列式容器都支持。

#### 5、emplace_back函数

![[Pasted image 20240818181006.png]]


### 三、list的特殊操作（==重要==）

#### 1、排序

![[Pasted image 20240818181021.png]]


#### 2、逆置

![[Pasted image 20240818181031.png]]


#### 3、去除连续重复元素

![[Pasted image 20240818181059.png]]


#### 4、合并两个有序链表（==重要==）

![[Pasted image 20240818181109.png]]


#### 5、splice操作
![[Pasted image 20240818181134.png]]

==注意：splice函数可以在同一个链表中进行操作。==

### 四、vector的insert扩容（==了解==）
![[Pasted image 20240818181222.png]]


### 五、set的使用

#### 1、基本特征

![image-20240625155701401](C++57期STL部分.assets/image-20240625155701401.png)

#### 2、查找操作

![image-20240625151438159](C++57期STL部分.assets/image-20240625151438159.png)

#### 3、插入操作

![image-20240625152648191](C++57期STL部分.assets/image-20240625152648191.png)

#### 4、不支持下标

![image-20240625155259513](C++57期STL部分.assets/image-20240625155259513.png)

#### 5、不支持修改

![image-20240625155621854](C++57期STL部分.assets/image-20240625155621854.png)

#### 6、针对于自定义类型的写法(==最重要==)

![image-20240625165501171](C++57期STL部分.assets/image-20240625165501171.png)

对于set容器而言，如果传递的类型Key是自定义类型，那么第二个模板参数Compare就需要改写，改写的方式有如下几种，要么直接改写std::less,要么给Compare重新传递一个模板参数。而std::less本质上就是在比较两个自定义类型的小于符号，所以可以有小于符号的重载；可以将std::less针对于自定义类型进行特化；可以自己实现一个类，重载函数调用运算符（写法类似std::less）.最终，set针对于自定义类型而言，有三种改写方法：==**模板的特化、运算符重载、函数对象。**==

![image-20240625170357230](C++57期STL部分.assets/image-20240625170357230.png)

![image-20240625170425929](C++57期STL部分.assets/image-20240625170425929.png)

![image-20240625170451899](C++57期STL部分.assets/image-20240625170451899.png)



### 六、multiset的使用

#### 1、基本特征

![image-20240625171739377](C++57期STL部分.assets/image-20240625171739377.png)

#### 2、其他操作

multiset的查找count与find、插入insert、删除erase与set容器基本一样，multiset也不支持下标操作与不支持使用迭代器进行修改元素。

#### 3、bound系列函数

![image-20240625173319474](C++57期STL部分.assets/image-20240625173319474.png)

#### 4、针对于自定义类型

![image-20240625173603275](C++57期STL部分.assets/image-20240625173603275.png)

multiset针对于自定义类型而言，也需要改写第二个模板参数Compare，改写方法有三种：模板的特化、函数运算符重载、函数对象。

## Day21

### 一、问题回顾

1、序列式容器的基本操作包括哪些？

2、vector为何在执行insert时候，会产生迭代器失效？如何解决？（重要）

3、list有哪些特殊操作？splice的作用是什么？可以在同一个链表中进行吗？

4、set的基本特征是什么？有哪些基本操作？

5、set针对于自定义类型有哪些改写方式？（重要）

6、multiset的基本特征是什么？有哪些基本操作？



### 二、map的使用

#### 1、基本特征

![image-20240625175016447](C++57期STL部分.assets/image-20240625175016447.png)

#### 2、查找操作

![image-20240626101522460](C++57期STL部分.assets/image-20240626101522460.png)

#### 3、插入操作

可以插入一个元素、或者一个迭代器范围的元素、或者大括号范围的（与set插入的三种方式一样）。

![image-20240626101616044](C++57期STL部分.assets/image-20240626101616044.png)

#### 4、map的下标（==重要==）

![image-20240626102435755](C++57期STL部分.assets/image-20240626102435755.png)

map的下标具备查找、插入、修改的功能。

#### 5、针对于自定义类型

![image-20240626103200472](C++57期STL部分.assets/image-20240626103200472.png)

如果map的key类型是类类型，那么就需要考虑能否比较大小，如果不行就需要改写Compare，改写方式依旧是：模板的特化、运算符重载、函数对象形式。

### 三、multimap的使用

#### 1、基本特征

![image-20240626105752114](C++57期STL部分.assets/image-20240626105752114.png)

#### 2、其他操作

multimap的查找count、find、插入insert、删除erase与map是基本一样的。

#### 3、不支持下标

![image-20240626110152526](C++57期STL部分.assets/image-20240626110152526.png)

#### 4、针对于自定义类型

如果multimap的key类型是**类类型**，那么就需要考虑能否比较大小，如果不行就需要改写Compare，改写方式依旧是：模板的特化、运算符重载、函数对象形式。

#### 总结：

1、关联式容器的底层实现是**红黑树**

2、关联式容器中的元素是**有序的**。

3、关联式容器中，只有map具备下标。

### 四、哈希相关的概念

#### 1、哈希函数

通过key值找到在哈希表中位置值。

```C++
index = H(key);
```

#### 2、构建方式

直接定址法： H(key) = a * key + b
平方取中法： key^2 = 1234^2 = 1522756 ------>227
数字分析法：H(key) = key % 10000；
除留取余法：H(key) = key mod p (p <= m, m为表长)  

#### 3、哈希冲突

不同的key值通过哈希函数进行映射之后，对应的位置值是一样，那就叫哈希冲突。

```C++
H(key1) = H(key2),在key1 != key2
```

#### 4、解决哈希冲突的方式

开放定址法
链地址法 (推荐使用这种，这也是STL中使用的方法)
再散列法
建立公共溢出区  

![image-20240626112300293](C++57期STL部分.assets/image-20240626112300293.png)

#### 5、装载因子

装载因子 = 元素的个数/表长 ，如果装载因子的值比较大，那么说明空间的利用率会比较高，同时冲突的概率也会比较高；如果装载因为的值比较小， 那么说明空间的利用率会比较低，同时冲突的概率也会比较低，[0.5, 0.75]



### 五、unordered_set的使用

#### 1、基本特征

![image-20240626113634717](C++57期STL部分.assets/image-20240626113634717.png)

#### 2、其他操作

unoredered_set的查找count、find、插入insert、删除erase与set是完全一样。也没有下标操作。

#### 3、针对于自定义类型的写法（==重要==）

![image-20240626114100787](C++57期STL部分.assets/image-20240626114100787.png)

对于模板参数Hash而言，当传递的Key类型是类类型的时候，需要自己实现Hash，实现方式有两种：模板的特化、函数对象。

![image-20240627095304861](C++57期STL部分.assets/image-20240627095304861.png)

![image-20240627095411234](C++57期STL部分.assets/image-20240627095411234.png)

对于模板参数KeyEqual而言，当传递的Key类型是类类型的时候，需要自己实现KeyEqual，实现方式有三种：模板的特化、函数对象、运算符重载。

![image-20240627095546147](C++57期STL部分.assets/image-20240627095546147.png)

![image-20240627095615136](C++57期STL部分.assets/image-20240627095615136.png)



## Day22

### 一、问题回顾

1、关联式容器有哪四个？有哪些基本操作？有什么共同点？

2、对于map而言，其中的下标访问运算符有哪些功能？

3、什么是哈希函数？什么是哈希冲突？什么是装载因子？



### 二、unordered_multiset的使用

#### 1、基本特征

![image-20240627100135930](C++57期STL部分.assets/image-20240627100135930.png)

#### 2、其他操作

unoredered_multiset的查找count、find、插入insert、删除erase与multiset是完全一样。也没有下标操作。

#### 3、针对于自定义类型

写法与unoredered_set是完全一样的，也要实现模板参数Hash以及KeyEqual。



### 三、unordered_map的使用

#### 1、基本特征

![image-20240627100933307](C++57期STL部分.assets/image-20240627100933307.png)

#### 2、其他操作

unoredered_map的查找count、find、插入insert、删除erase与map是完全一样。

#### 3、下标操作（==重要==）

![image-20240627101031654](C++57期STL部分.assets/image-20240627101031654.png)

unoredered_map的下标也具备查找、插入与修改的功能。

#### 4、针对于自定义类型

![image-20240627101119719](C++57期STL部分.assets/image-20240627101119719.png)

string虽然是类类型，但是已经实现了针对hash的特化版本，所以无需改写Hash。

![image-20240627101519926](C++57期STL部分.assets/image-20240627101519926.png)

### 四、unordered_multimap的使用

#### 1、基本特征

![image-20240627101941487](C++57期STL部分.assets/image-20240627101941487.png)

#### 2、其他操作

unoredered_multimap的查找count、find、插入insert、删除erase与multimap是完全一样。同时也不具备下标操作。

#### 总结：

1、无序关联式容器的底层实现是**哈希**

2、无序关联式容器中的元素是**没有顺序**的。

3、无序关联式容器中，只有unordered_map具备下标。



### 五、容器的选择（==重要==）

#### 1、元素是不是有序的

首先，最先想到的是关联式容器；最不应该想到的是无序关联式容器；其次才是序列式容器（vector、deque、list）

#### 2、容器有没有下标

序列式容器：vector、deque

关联式容器：map

无序关联式容器：unordered_map

#### 3、查找的时间复杂度

序列式容器：O(N).

关联式容器：O(logN)

无序关联式容器：O(1)

#### 4、迭代器的类型

随机访问迭代器：vector、deque

双向迭代器：list、关联式容器

前向迭代器：无序关联式容器



### 六、priority_queue的使用

#### 1、模板参数

![image-20240627112037104](C++57期STL部分.assets/image-20240627112037104.png)

#### 2、原理

![image-20240627114656566](C++57期STL部分.assets/image-20240627114656566.png)

#### 3、基本操作

```C++
插入操作：push;
获取顶部元素:top;
删除顶部元素：pop;
容器是不是空的:empty;
元素个数的函数：size;
```

注意：容器适配器是没有迭代器的，所以不能使用迭代进行遍历。

![image-20240627114904543](C++57期STL部分.assets/image-20240627114904543.png)

#### 4、针对于自定义类型

针对于自定义类型而言，需要实现模板参数Compare，实现方式依旧是三种：模板的特化、函数对象、运算符重载。

### 七、迭代器

#### 1、基本概念

迭代器可以看成是一种广义指针。

#### 2、分类

输入迭代器、输出迭代器、前向迭代器、双向迭代器、随机访问迭代器。

![image-20240627195134182](C++57期STL部分.assets/image-20240627195134182.png)

#### 3、ostream_iterator的使用

```C++
class ostream_iterator
{
    //__s = cout;
    //__c = "\n"
    //_M_stream = &cout;
    //_M_string = "\n"
    ostream_iterator(ostream_type& __s, const _CharT* __c) 
    : _M_stream(&__s)
    , _M_string(__c)  
    {}
    
   ostream_iterator<_Tp>& operator*() { return *this; }
   ostream_iterator<_Tp>& operator++() { return *this; } 
   ostream_iterator<_Tp>& operator++(int) { return *this; } 
    
    ostream_iterator<_Tp>& operator=(const _Tp& __value) { 
    *_M_stream << __value;
    if (_M_string) *_M_stream << _M_string;
    return *this;
  }
private:
  ostream_type* _M_stream;
  const _CharT* _M_string;

}
```

copy函数的实现

```C++
template<class InputIt, class OutputIt>
OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
{
    while (first != last) {
        *d_first++ = *first++;
    }
    return d_first;
}

//first = vec.begin();
//last = vec.end();
//d_first = osi;
     f             last
1 5  7   9   3
OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
{
    while (first != last) {
        *d_first++ = *first++;
    }

    return d_first;
}

osi = 7;

ostream_iterator<_Tp>& operator=(const intp& __value) { 
    *_M_stream << __value;    //cout <<  5
    if (_M_string) *_M_stream << _M_string; //cout << "\n"
    return *this;
  }
```

#### 4、istream_iterator的使用

```C++
class istream_iterator {
    //__s = cin
    //_M_stream = &cin
    istream_iterator(istream_type& __s) 
    : _M_stream(&__s) 
    { 
        _M_read();
    }
    
    istream_iterator() 
    : _M_stream(0)
    , _M_ok(false) 
    {}
    
    istream_iterator operator++(int)  
    {
        istream_iterator __tmp = *this;
        _M_read();
        return __tmp;
    }
    
    void _M_read() 
    {
        _M_ok = (_M_stream && *_M_stream) ? true : false;
        if (_M_ok) 
        {
            *_M_stream >> _M_value;//cin >> _M_value;//2
            _M_ok = *_M_stream ? true : false;
        }
    }
    
    bool _M_equal(const istream_iterator& __y) const
    { return (_M_ok == __y._M_ok) && (!_M_ok || _M_stream == __y._M_stream); }
    
    reference operator*() const { return _M_value; }
    
private:
  istream_type* _M_stream;
  _Tp _M_value;
  bool _M_ok;

};

inline bool operator!=(const istream_iterator& __x, const istream_iterator& __y) 
{
  return !__x._M_equal(__y);
}
__x = isi;
__y = istream_iterator<int>();

bool _M_equal(const istream_iterator& __y) const
{ 
    return (_M_ok == __y._M_ok) && (!_M_ok || _M_stream == __y._M_stream); 
}


template <class _Container>
inline back_insert_iterator<_Container> back_inserter(_Container& __x) {
  return back_insert_iterator<_Container>(__x);
}

class back_insert_iterator 
{
    //container = &vec;
    explicit back_insert_iterator(_Container& __x) 
    : container(&__x) 
    {}
    
    back_insert_iterator<_Container>& operator*() { return *this; }
    back_insert_iterator<_Container>& operator++() { return *this; }
    back_insert_iterator<_Container>& operator++(int) { return *this; } 
    
    back_insert_iterator<_Container>&  operator=(const int& __value) 
    { 
        //执行了容器的push_back
        container->push_back(__value);
        return *this;
    }
};
```

```C++
//first = isi;
//last = istream_iterator<int>();
//d_first  = std::back_inserter(vec)
OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
{
    while (first != last) {
        *d_first++ = *first++;
    }
    return d_first;
}

d_first  = 2;

//这样就可以将键盘输入进来的数字1 插入到了vector尾部了，并且使用的是push_back；
```



## Day23

### 一、问题回顾

1、无序关联式容器针对于自定义类型需要改写什么？

2、优先级队列的底层采用的是什么数据结构？优先级队列提供了哪些操作？

3、如何选择合适的容器？容器的维度有哪些？

4、迭代器的作用是什么？有哪些分类？



### 二、三组插入迭代器

back_inserter是函数模板，该函数的返回类型是back_insert_iterator类型，底层会调用push_back。
front_inserter是函数模板，该函数的返回类型是front_insert_iterator类型，底层会调用push_front。
inserter是函数模板，该函数的返回类型是insert_iterator类型，底层会调用insert。

![image-20240628095944375](C++57期STL部分.assets/image-20240628095944375.png)

### 三、反向迭代器

![image-20240628100505614](C++57期STL部分.assets/image-20240628100505614.png)

![image-20240628101347396](C++57期STL部分.assets/image-20240628101347396.png)

### 四、算法

#### 1、分类

- 非修改式的算法  count、find、**for_each**
- 修改式的算法  **copy**、replace、**remove**、**remove_if**
- 排序操作  **sort**
- 二分查找  lower_bound、upper_bound
- 集合操作  **set_intersection**
- 堆相关的操作  make_heap、push_heap
- 取最值  max、min
- 数值操作  accumulate
- 未初始化的内存操作  **uninitialized_copy**

#### 2、for_each算法

一元函数：函数的参数是一个。二元函数：函数的参数是两个。

一元谓词/断言：函数的参数是一个，并且函数的返回类型是bool。二元谓词/断言：函数的参数是两个，并且函数的返回类型是bool。

```C++
template<class InputIt, class UnaryFunction>
constexpr UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f)
{
    for (; first != last; ++first) {
        f(*first);
    }
    return f; // implicit move since C++11
}
```

#### 3、lambda表达式（==重要==）

##### 3.1、基本概念

![image-20240628111900778](C++57期STL部分.assets/image-20240628111900778.png)

##### 3.2、调用方法

![image-20240628112614127](C++57期STL部分.assets/image-20240628112614127.png)

##### 3.3、捕获列表

![image-20240628114144199](C++57期STL部分.assets/image-20240628114144199.png)

==**注意：捕获列表中，一般需要捕获是局部变量，对于全局变量而言，无需捕获也可以直接进行使用。**==

#### 4、lambda的误用

![image-20240628174922252](C++57期STL部分.assets/image-20240628174922252.png)

https://cppinsights.io/   

#### 4、remove_if算法

```C++
template<class InputIt, class UnaryPredicate>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate p)
{
    for (; first != last; ++first) {
        if (p(*first)) {
            return first;
        }
    }
    return last;
}

template<class ForwardIt, class UnaryPredicate>
ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPredicate p)
{
    first = std::find_if(first, last, p);
    if (first != last)
        for(ForwardIt i = first; ++i != last; )
            if (!p(*i))
                *first++ = std::move(*i);
    return first;
}
//first = vec.begin();
//last = vec.end();
//p = func() ;    
```



```C++
                  f           last
1, 3, 5, 4, 3, 2, 6, 4, 3, 2
                               i
```

![image-20240628145941458](C++57期STL部分.assets/image-20240628145941458.png)

![image-20240628155808991](C++57期STL部分.assets/image-20240628155808991.png)

#### 5、vector中的push_back导致迭代器失效（==重要==）

![image-20240628151430212](C++57期STL部分.assets/image-20240628151430212.png)

#### 6、bind1st、bind2nd

![image-20240628155022420](C++57期STL部分.assets/image-20240628155022420.png)

![image-20240628152804849](C++57期STL部分.assets/image-20240628152804849.png)

![image-20240628155746156](C++57期STL部分.assets/image-20240628155746156.png)

### 四、bind的使用（==非常重要==）

#### 1、模板参数

![image-20240628160033158](C++57期STL部分.assets/image-20240628160033158.png)

#### 2、引用折叠

```C++
& && = &;
&& && = &&;
&  & = &;
&& & = &;
```

#### 3、bind绑定函数

![image-20240628161642089](C++57期STL部分.assets/image-20240628161642089.png)

#### 4、函数指针的使用

![image-20240628163901413](C++57期STL部分.assets/image-20240628163901413.png)

==包括函数指针的定义、函数指针的延迟调用思想、函数指针可以注册回调函数与执行回调函数。函数指针只能执行某些类型的函数，不能随便指向其他类型的函数==

![image-20240628164041488](C++57期STL部分.assets/image-20240628164041488.png)

#### 5、占位符

![image-20240628171527074](C++57期STL部分.assets/image-20240628171527074.png)

#### 6、function的使用

![image-20240628172626082](C++57期STL部分.assets/image-20240628172626082.png)

![image-20240628172720999](C++57期STL部分.assets/image-20240628172720999.png)

#### 7、函数类型

![image-20240628172658377](C++57期STL部分.assets/image-20240628172658377.png)

#### 8、bind绑定数据成员

![image-20240628173529630](C++57期STL部分.assets/image-20240628173529630.png)



## Day24

### 一、问题回顾

1、lambda表达式的格式是什么样的？捕获列表有哪几种捕获方式？全局（静态）变量需要捕获吗？

```C++
//捕获列表
int a = 10;
function<void(int)> f = [&a](int value){ cout << value << "  ";};

int add(int x, int y)
{
    
}

add(a, a);
```

2、lambda的使用误区？

```C++
捕获局部变量的引用
```

3、remove_if能将满足条件的元素删除吗？

4、什么是引用折叠？

5、bind可以绑定那些函数？什么是占位符？占位符代表什么？bind默认采用什么传递方式？

```C++
bind可以绑定普通函数、成员函数；
int add(int x, int y);
//int( int)
function<int(int)>  f = bind(add, 1, _2)
```

6、什么是函数类型或者函数标签？函数类型可以使用什么进行存储？



### 二、bind绑定成员函数传参问题（==重要==）

可以采用地址传递与值传递的方式

![image-20240629102554220](C++57期STL部分.assets/image-20240629102554220.png)

### 三、bind与function的结合使用

![image-20240629115319451](C++57期STL部分.assets/image-20240629115319451.png)

![image-20240629115351717](C++57期STL部分.assets/image-20240629115351717.png)

![image-20240629115422971](C++57期STL部分.assets/image-20240629115422971.png)



## Day25

### 一、成员函数适配器

![image-20240701100520698](C++57期STL部分.assets/image-20240701100520698.png)

![image-20240701100600544](C++57期STL部分.assets/image-20240701100600544.png)

### 二、函数对象

所有可以与小括号进行结合，体现函数含义的都称为函数对象。

- 重载了函数调用运算符的类创建的对象

- 函数名

- 函数指针

  

### 三、空间配置器（==重难点==）

#### 1、为何会将空间的申请与对象的构建分开？

因为在STL中，需要的对象是批量的（大量的），如果每创建一个对象都申请一块空间，那么空间的申请就会比较频繁，那么有可能会导致内存碎片的问题或者效率不高，所以就将空间的申请与对象的构建分开。



#### 2、函数接口

```C++
//申请空间
T* allocate( std::size_t n );

//释放空间
void deallocate( T* p, std::size_t n );

//构建对象
void construct( pointer p, const_reference val );

//销毁对象
void destroy( pointer p );
```



#### 3、原理

![image-20240701143719638](C++57期STL部分.assets/image-20240701143719638.png)