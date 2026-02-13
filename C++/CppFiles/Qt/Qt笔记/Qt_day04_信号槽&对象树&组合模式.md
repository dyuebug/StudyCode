## connect的另一种重载形式
![[Pasted image 20240717211657.png]]

第三个参数是用的lambda表达式

## 计算器程序存在的问题
![[Pasted image 20240717211829.png]]
第一个版本的计算器程序，太多的槽函数，要想办法用一个通用槽函数进行接收数字
[[计算器（2个版本）#版本2（使用sender 和 qobject_cast）]]

## 在槽函数里面调用sender函数
![[Pasted image 20240717212152.png|300]]

## 复习动态多态
[[8.多态#**动态多态（虚函数机制）被激活的条件**（重点*）]]
![[Pasted image 20240717212356.png|400]]
动态多态（Dynamic Polymorphism）是面向对象编程中的一个重要概念，指的是通过基类指针或引用调用派生类对象的方法时，根据实际对象的类型来决定调用哪个类的方法。它是通过虚函数（virtual function）和运行时类型信息（RTTI）来实现的。<span style="background:#d3f8b6">它允许不同的类以相同的接口执行操作，具体的实现方式依赖于运行时所调用的对象类型。动态多态通常通过**继承**和**方法重写**来实现。</span>

### 为什么使用动态多态？

动态多态性的主要目的是实现代码的灵活性、可维护性和可扩展性：

1. **灵活性和扩展性**：通过基类指针或引用调用派生类对象的方法，可以在运行时决定调用哪个方法，而不需要在编译时确定。这使得程序能够根据实际情况动态地处理不同类型的对象，从而更加灵活和可扩展。<span style="background:#d3f8b6">可以轻松地添加新类，而无需修改现有代码，遵循开闭原则（Open-Closed Principle）。（扩展性）</span><font color="#2DC26B">允许不同的对象以相同的方式处理，从而使代码更加灵活和通用。（灵活性）</font>
    
2. **代码重用**：通过基类的统一接口管理派生类对象，可以避免重复编写相似的代码，提高代码的重用性和可维护性。<span style="background:#d3f8b6">通过继承，可以在多个类之间共享代码。</span>
    
3. **多态性**：多态性是面向对象编程的核心概念之一，能够根据对象的实际类型来调用对应的方法，从而实现不同对象对同一消息的不同响应。这种能力使得代码更加灵活和动态，能够更好地适应变化。
    

### 如何使用动态多态？

要使用动态多态性，需要遵循以下关键步骤：

1. **定义虚函数**：在基类中通过将函数声明为虚函数（使用 `virtual` 关键字），来使得这些函数可以被派生类重写。
```c++
//定义一个基类（父类）和若干个派生类（子类）
#include <iostream>
using namespace std;

class Animal {
public:
    virtual void makeSound() {
        cout << "Animal sound" << endl;
    }
};

class Dog : public Animal {
public:
    void makeSound() override {
        cout << "Woof" << endl;
    }
};

class Cat : public Animal {
public:
    void makeSound() override {
        cout << "Meow" << endl;
    }
};
```
2. **重写虚函数**：在派生类中重写（覆盖）基类的虚函数，提供特定于派生类对象的实现。
    
```c++
class Derived : public Base {
public:
    void polymorphic() override {
        // 派生类中对虚函数的实现
    }
};
```
    
3. **通过基类指针或引用使用动态多态**：通过基类指针或引用来操作派生类对象，可以实现动态多态性。
    
```c++
//使用指向基类的指针或引用来调用方法
void playSound(Animal* animal) {
    animal->makeSound();
}

int main() {
    Animal* myDog = new Dog();
    Animal* myCat = new Cat();

    playSound(myDog); // 输出: Woof
    playSound(myCat); // 输出: Meow

    delete myDog;
    delete myCat;

    return 0;
}
```

在上述代码中，`playSound` 函数接受一个 `Animal` 类型的指针参数，但实际上传递给它的可以是 `Dog` 或 `Cat` 对象的指针。通过调用 `animal->makeSound()`，实际调用的是具体对象（`Dog` 或 `Cat`）的 `makeSound` 方法，而不是 `Animal` 类的 `makeSound` 方法。这就是动态多态的表现。
#### 使用虚函数（virtual function）

在基类中定义虚函数，允许子类重写它们。虚函数的存在是实现动态多态的关键。当通过基类指针或引用调用虚函数时，实际调用的是子类的实现。

总结来说，动态多态通过虚函数和运行时绑定（runtime binding）实现，允许程序在运行时决定调用哪个类的实现，使代码更加灵活和可扩展。

## Qt框架提供了动态类型转换的接口
![[Pasted image 20240717213204.png|500]]
## 在槽函数里面获取发射方的信息
![[Pasted image 20240717213300.png]]

## 总结一下信号和槽
### 优势：
1. 松耦合 在发送方代码里面，不需要出现接收方的信息
2. 类型安全
3. 关联自由
4. 使用上会使用继承机制新添槽函数or信号
### 劣势：
1. 性能差 调用槽函数的准备时间 是 非虚函数的 10倍

## 内存管理
![[Pasted image 20240717213520.png]]

## 对象树
### 什么是对象树？

在 Qt 中，对象树（Object Tree）是一个层次结构，用于组织和管理对象之间的父子关系。对象树是通过 QObject 类的父子关系实现的，每个 QObject 对象都可以有一个父对象和若干个子对象。这个层次结构可以自动管理对象的生命周期，并提供了一种方便的方式来访问和操作对象。
![[Pasted image 20240717213839.png|400]]

### 为什么使用对象树？

使用对象树有以下几个重要的优点：

- **自动管理内存**：Qt 对象树通过父子关系自动管理内存，删除一个对象时，其所有子对象也会被删除，防止内存泄漏。
- **组织结构清晰**：对象树提供了一种直观的方式来组织和管理对象，使得代码结构清晰、易于维护。
- **事件传播**：在对象树中，事件可以从子对象传播到父对象，方便事件处理。
- **统一访问接口**：通过对象树，可以方便地遍历和访问对象层次结构中的任意对象。
#### 对象树的作用
![[Pasted image 20240717215159.png|400]]

### 如何使用对象树？

在Qt中，使用对象树的基本原则如下：

#### 1. 创建对象树

在 Qt 中，创建对象树非常简单，只需在创建对象时指定其父对象即可。例如：
    
```c++
#include <QApplication>
#include <QWidget>
#include <QPushButton>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 创建父对象
    QWidget *window = new QWidget();
    window->setWindowTitle("Object Tree Example");

    // 创建子对象，并指定父对象
    QPushButton *button1 = new QPushButton("Button 1", window);
    QPushButton *button2 = new QPushButton("Button 2", window);

    // 设置子对象的位置
    button1->move(50, 50);
    button2->move(50, 100);

    // 显示窗口
    window->resize(200, 200);
    window->show();

    return app.exec();
}
```
在上述代码中，`button1` 和 `button2` 是 `window` 的子对象。Qt 会自动管理这些对象的生命周期，当 `window` 被删除时，`button1` 和 `button2` 也会被自动删除。

#### 2. 访问对象树

通过 QObject 类提供的方法，可以方便地访问和遍历对象树。例如：
```c++
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QDebug>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget *window = new QWidget();
    window->setWindowTitle("Object Tree Example");

    QPushButton *button1 = new QPushButton("Button 1", window);
    QPushButton *button2 = new QPushButton("Button 2", window);

    button1->move(50, 50);
    button2->move(50, 100);

    window->resize(200, 200);
    window->show();

    // 获取 window 的子对象列表
    const QObjectList &children = window->children();
    for (QObject *child : children) {
        qDebug() << "Child object:" << child->objectName();
    }

    return app.exec();
}
```
在这里，我们通过 `window->children()` 方法获取 `window` 的子对象列表，并遍历打印出子对象的名称。

#### 3. 信号和槽

对象树还可以用来简化信号和槽机制。例如，当一个子对象发出信号时，父对象可以捕获并处理该信号：
```c++
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QWidget *window = new QWidget();
    window->setWindowTitle("Object Tree Example");

    QPushButton *button1 = new QPushButton("Button 1", window);
    QPushButton *button2 = new QPushButton("Button 2", window);

    button1->move(50, 50);
    button2->move(50, 100);

    window->resize(200, 200);

    QObject::connect(button1, &QPushButton::clicked, []() {
        QMessageBox::information(nullptr, "Info", "Button 1 clicked");
    });

    QObject::connect(button2, &QPushButton::clicked, []() {
        QMessageBox::information(nullptr, "Info", "Button 2 clicked");
    });

    window->show();

    return app.exec();
}
```
在这个例子中，我们连接了两个按钮的 `clicked` 信号到相应的槽函数。当用户点击按钮时，会显示消息框。

总结来说，Qt 中的对象树提供了一种简洁、高效的方式来管理对象及其生命周期，同时也增强了代码的可维护性和灵活性。
## 组合模式
![[Pasted image 20240717215815.png|400]]
在Qt中，对象树和组合模式是两个相关但不完全相同的概念。让我们分别解释它们：

### 对象树（Object Tree）在Qt中的含义

在Qt中，对象树是指对象之间的层次结构，其中每个对象可以拥有零个或多个子对象，并且对象之间通过父子关系进行管理。对象树的根是通常是一个 `QMainWindow` 或 `QWidget`，例如一个窗口或一个部件，这些部件可以包含其他部件，从而形成一个层次化的结构。

### 组合模式在Qt中的应用

组合模式是一种设计模式，允许客户端统一对待单个对象和对象组合。在Qt中，对象树结构可以看作是组合模式的应用：

1. **为什么使用组合模式**：
    
    - **统一接口**：组合模式允许客户端使用统一的接口处理单个对象和对象组合，无需区分它们的具体类型。
    - **层次化管理**：通过组合模式，可以将对象按照层次化结构进行管理和操作，使得代码更加灵活和可扩展。
    - **代码重用**：将相同的操作应用于单个对象和对象组合，避免重复编写代码，提高代码的可维护性和可读性。
2. **如何使用组合模式**：
    
    在Qt中，组合模式的使用体现在对象树的管理上：

- **创建对象树**：通过创建和设置对象的父子关系，形成一个对象树结构。Qt中的对象通过 `setParent()` 方法来设置父对象。
```c++
QWidget *parentWidget = new QWidget(); // 创建一个父部件
QWidget *childWidget = new QWidget(parentWidget); // 创建一个子部件，并设置其父部件

```

- **管理对象树**：通过父子关系，Qt自动管理对象的内存释放。当父对象被销毁时，其所有子对象也会被自动销毁，无需手动释放内存。
    
- **操作对象树**：可以使用Qt提供的方法，如 `findChild()` 和 `findChildren()` 来查找对象树中的子对象，进一步操作和管理它们。

```c++
// 查找对象树中的子对象
QWidget *childWidget = parentWidget->findChild<QWidget*>("childWidgetName");

```
    
	总之，在Qt中，对象树和组合模式结合使用，可以帮助管理和操作复杂的GUI结构或对象层次，提升代码的灵活性和可维护性。通过统一的父子关系管理，可以有效地组织和操作大型的对象集合，使得程序设计更加清晰和高效。
### 不能用组合模式的场景
![[Pasted image 20240717215858.png|400]]
### 可以用组合模式的场景
![[Pasted image 20240717215940.png|400]]

