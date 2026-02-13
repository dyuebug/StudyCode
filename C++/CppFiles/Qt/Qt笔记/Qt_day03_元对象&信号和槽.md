# QMap & QHash
#QMap #QHash #容器（Qt）
<font color="#2DC26B">QMap 底层是红黑树，是有序的</font>
![[Pasted image 20240716222208.png]]

<font color="#2DC26B">QHash 底层是hash，是无序的</font>

![[Pasted image 20240716222218.png]]
两者的插入（insert）和删除（remove）都是相同的
改用[]下标.
查询是否有这个元素，用contains

# 元对象
## 什么是元对象
在Qt中，元对象（Meta Object）是一个关键的概念，<span style="background:#d3f8b6">用于提供在运行时动态地访问类的信息</span>。每个QObject派生类都有一个对应的元对象，<span style="background:#d3f8b6">它包含了关于类的信息，比如类名、父类名、信号（signals）、槽（slots）等</span>。

<span style="background:#d3f8b6">元对象系统使得在运行时可以动态地访问和操作类的成员，而不需要在编译时知道这些信息</span>。这种机制<span style="background:#d3f8b6">允许Qt实现信号与槽的连接、属性系统、反射等高级特性</span>。

具体来说，元对象通过使用<font color="#2DC26B">Qt元对象宏（如Q_OBJECT、Q_PROPERTY、Q_SIGNALS、Q_SLOTS等）</font>来进行声明和定义，Qt的元对象编译器（MOC）会处理这些宏并生成相关的元对象代码，使得类能够在运行时进行信号槽连接、属性访问等操作。

总结来说，元对象是Qt中用来描述和管理类的元信息的机制，它使得Qt能够实现许多强大的运行时特性和功能。

## 为什么使用元对象
使用元对象（Meta Object）在Qt中有几个重要的理由和优势：

1. **信号与槽机制**: 元对象系统使得在运行时能够动态地建立和管理对象之间的信号与槽连接。这种机制使得Qt中的事件处理和通信变得非常灵活和强大，无需硬编码连接关系。
    
2. **动态属性系统**: Qt的元对象系统支持动态属性的定义和访问。这使得可以在运行时动态添加、修改和查询对象的属性，而无需修改源代码。
    
3. **反射（Reflection）**: 元对象使得在运行时能够动态地查询和调用类的成员函数和属性，这种能力被称为反射。这种机制在很多应用中非常有用，比如对象的序列化与反序列化、动态生成用户界面等。
    
4. **类型安全的连接**: Qt的信号与槽连接是类型安全的，而不是依赖于字符串或者函数指针。这意味着编译器可以帮助捕获连接错误，并提供更好的代码安全性和可维护性。
    
5. **对象系统的自省（Introspection）**: 元对象系统使得可以在运行时查询类的信息，比如类的名称、父类信息、成员函数列表等。这对于编写通用算法、实现插件系统等有很大帮助。
    

总体来说，元对象系统使得Qt在运行时能够实现更高级、更动态的编程模式，同时提高了代码的灵活性和可扩展性。通过元对象，Qt能够在静态编译时保持高效性和类型安全性，同时在运行时提供丰富的动态特性和功能。

## 怎么使用元对象
在Qt中使用元对象（Meta Object）主要涉及以下几个方面：

1. **使用Q_OBJECT宏**: 对于需要使用信号与槽机制的类，需要在类的定义中添加`Q_OBJECT`宏。这个宏告诉Qt元对象编译器（MOC）需要为这个类生成元对象代码。
    
```c++
class MyClass : public QObject
{
    Q_OBJECT

public:
    explicit MyClass(QObject *parent = nullptr);
    // other declarations
};
```
    
2. **声明信号与槽**: 在类的声明中使用`signals`和`slots`关键字声明信号和槽函数。这些函数可以是任意的成员函数，不需要特殊的修饰符。
    
```c++
class MyClass : public QObject
{
    Q_OBJECT

public:
    explicit MyClass(QObject *parent = nullptr);

signals:
    void mySignal();

public slots:
    void mySlot();
};

    
```
3. **使用元对象操作**: 一旦在类中添加了`Q_OBJECT`宏，并声明了信号与槽，就可以在运行时使用Qt的元对象系统进行操作。比如连接信号与槽、查询类的元信息等。
    
```c++
MyClass obj1, obj2;
QObject::connect(&obj1, SIGNAL(mySignal()), &obj2, SLOT(mySlot()));

```
    
4. **属性系统**: 如果需要在类中定义动态属性，可以使用`Q_PROPERTY`宏。这允许在运行时添加和查询对象的属性。
    
```c++
class MyClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int myProperty READ getMyProperty WRITE setMyProperty NOTIFY myPropertyChanged)

public:
    explicit MyClass(QObject *parent = nullptr);

    int getMyProperty() const;
    void setMyProperty(int value);

signals:
    void mySignal();
    void myPropertyChanged(int newValue);

public slots:
    void mySlot();

private:
    int m_myProperty;
};

```
    
5. **使用动态调用**: 使用Qt的反射功能，可以在运行时动态调用对象的函数和访问属性。
```c++
// 示例：动态调用槽函数
MyClass obj;
QMetaObject::invokeMethod(&obj, "mySlot");

```
    

总之，Qt的元对象系统提供了丰富的工具和宏来处理对象的元信息，使得在运行时能够实现信号与槽连接、动态属性、反射等高级功能。通过适当使用这些工具，可以使Qt应用程序更加灵活和强大。

### Qt框架中使用元对象系统
1. 继承QObject，并且第一个继承
2. 使用一个宏 Q_OBJECT
3. Qt框架提供了一个MOC（元对象编辑器）
4. <span style="background:#d3f8b6">元对象类定义不能和main函数放在一个文件，弄一个新的文件</span>
#### 生成代码流程
![[Pasted image 20240717001214.png]]

### 元对象系统有什么用
|                                                                                        |
| -------------------------------------------------------------------------------------- |
| 1. 动态属性&动态方法(<font color="#2DC26B">自省和反射</font>) --> 可以在运行过程中，动态添加一个属性or方法，并且访问属性、调用方法 |
| <font color="#2DC26B">2. 信号和槽</font>                                                   |
| 3. 运行时类型转换                                                                             |
| 4. 对象树 内存管理                                                                            |
|                                                                                        |
<font color="#2DC26B">自省 运行过程中，对象获取自身的结构信息</font>
<font color="#2DC26B">反射 运行过程中，对象可以调整自身的结构</font>

#### 实例
![[Pasted image 20240717001735.png]]
![[Pasted image 20240717001750.png]]

#### 应用场景
1. 导表工具（做游戏的策划填的表给开发人员）
2. 路由工具

## 信号槽机制
信号槽机制是Qt中一种强大的通信机制<span style="background:#d3f8b6">，用于对象间的事件处理和通信</span>。它的<span style="background:#d3f8b6">核心思想是解耦事件发送者和接收者，使得它们之间不需要直接调用彼此的方法，而是通过信号和槽函数进行间接通信</span>。

### 信号槽机制是什么？

在Qt中，<span style="background:#d3f8b6">信号（signals）是特殊的成员函数，用于声明对象发生了某个事件</span>。<span style="background:#d3f8b6">信号函数没有具体的实现，只是声明了事件的发生</span>。<span style="background:#d3f8b6">槽（slots）是普通的成员函数，用于响应信号的发生</span>。<span style="background:#d3f8b6">信号可以带有参数，而槽函数可以接收这些参数</span>。

### 为什么使用信号槽机制？

1. **松耦合**: 信号槽机制可以<span style="background:#d3f8b6">将事件发送者（发出信号的对象）和事件接收者（处理信号的对象）解耦</span>。这意味着<span style="background:#d3f8b6">对象之间不需要直接调用对方的方法，从而降低了耦合度</span>，使得代码更加模块化和易于维护。
    
2. **线程安全**: Qt的信号槽机制保证了线程安全。信号的发出和槽函数的调用是<font color="#2DC26B">在接收信号的对象的线程上同步执行的</font>，因此可以安全地进行跨线程通信。
    
3. **灵活性**: 信号槽机制使得可以动态地在运行时建立和取消连接，而不需要在编译时静态确定连接关系。这种灵活性特别适用于需要动态添加或移除事件处理逻辑的情况。
    

### 如何使用信号槽机制？

1. **声明信号和槽**: 在QObject派生类中使用`signals`和`slots`关键字声明信号和槽函数。
    
```c++
class MyObject : public QObject
{
    Q_OBJECT

public:
    MyObject(QObject *parent = nullptr);

signals:
	//声明信号
    void valueChanged(int newValue);

public slots:
	//声明槽
    void setValue(int newValue);
};

```
    
2. **连接信号与槽**: 使用`QObject::connect`静态函数连接信号和槽。
```c++
//创建对象
MyObject obj1, obj2;
//连接信号和槽
//这里是用字符串的方式，最好使用函数指针的方式
QObject::connect(&obj1, SIGNAL(valueChanged(int)), &obj2, SLOT(setValue(int)));

```
    
    这段代码将`obj1`对象的`valueChanged`信号连接到`obj2`对象的`setValue`槽函数上。当`obj1`发出`valueChanged`信号时，`obj2`的`setValue`槽函数将被调用，并且传递`int`类型的参数。
    
3. **发出信号**: 在对象中使用`emit`关键字来发出信号。
    
```c++
void MyObject::setValue(int newValue)
{
    if (m_value != newValue) {
        m_value = newValue;
        //发射信号
        emit valueChanged(newValue);
    }
}

```
    
    当`setValue`函数中的`m_value`值发生变化时，通过`emit valueChanged(newValue);`语句发出`valueChanged`信号，通知所有连接到这个信号的槽函数。
    

通过以上几点，可以看出信号槽机制在Qt中的使用方式和优势，它使得Qt程序设计更加灵活、可维护，并且支持多线程环境下的安全通信。

### 信号与槽总结
#### 如何设计一个信号
1. 支持元对象系统的类的成员函数
2. 访问权限控制符 是一个伪关键字 signals
3. 只写声明，不写定义 （其函数定义是框架自动生成）

![[Pasted image 20240717002959.png]]
#### 如何发射一个信号
emit伪关键字，直接调用信号的函数
![[Pasted image 20240717003008.png]]


#### 如何设计一个槽函数
1、槽函数依然是一个成员函数
2、访问权限控制符 private slots/ public slots/
protected slots
3、其他的和普通成员函数一样
![[Pasted image 20240717003125.png]]

#### 关联 QObject::connect
![[Pasted image 20240717003220.png]]
#### connect的另一种重载形式

![[Pasted image 20240717003255.png]]
用成员函数的指针
#### 两种connect的对比
![[Pasted image 20240717003621.png]]

#### 槽函数的调用是同步的
![[Pasted image 20240717003425.png]]

#### 信号和槽在同一个类内部
![[Pasted image 20240717003456.png]]

#### 信号和槽的参数和返回值的限制
1 返回值都必须是void
2 信号和槽可以携带参数
a 信号和槽参数一致
b 信号的参数比槽多
c 信号的参数比槽少（这个是不可以的）
![[Pasted image 20240717003539.png]]

#### 信号和槽遇到函数重载
![[Pasted image 20240717003649.png]]
##### QOverload来解决重载的问题
![[Pasted image 20240717003716.png]]

#### 同一个信号 关联了同一个槽 多次
![[Pasted image 20240717003751.png]]

#### 同一个信号依次关联不同的槽
![[Pasted image 20240717003818.png]]
![[Pasted image 20240717003829.png]]

#### 信号和槽交换地位
槽不能当信号用，信号的函数定义是框架生成的，是空的

<span style="background:#d3f8b6">信号可以当槽（public）用</span>
![[Pasted image 20240717004009.png]]

