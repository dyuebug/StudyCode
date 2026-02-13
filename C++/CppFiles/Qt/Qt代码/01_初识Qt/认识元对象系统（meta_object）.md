## Myclass.h
```c++
#ifndef MYCLASS_H
#define MYCLASS_H

#include <QObject>

class MyClass : public QObject
{
    Q_OBJECT
public:
    explicit MyClass(QObject *parent = nullptr);
    // Q_PROPERTY 指明属性是一个动态属性
    Q_PROPERTY(int health MEMBER m_health)
    Q_PROPERTY(int mana MEMBER m_mana)
signals:
private:
    int m_health;
    int m_mana;
};

#endif // MYCLASS_H
```
## Myclass.cpp
```c++
#include "myclass.h"
//初始化m_health m_mana
MyClass::MyClass(QObject *parent) : QObject(parent), m_health(100), m_mana(20)
{

}
```

## main.cpp
```c++
#include "mainwindow.h"
#include "myclass.h"
#include <QApplication>
#include <QMetaObject>
#include <QDebug>
#include <QMetaProperty>
//class MyClass :public QObject{
//    Q_OBJECT
//  // ...
//};
void lose(QObject *obj, const char * property){
    // property "health" "mana"
    // 用户传入一个字符串，根据字符串的内容，查阅对象的结构信息，然后间接地修改对象的内容
    // 根据对象找到元对象
    const QMetaObject *meta = obj->metaObject();
    // 获取动态属性的个数
    int count = meta->propertyCount();
    qDebug() << count;
    // 按下标访问所有的动态属性
    for(int i = 0; i < count; ++i){
        QMetaProperty metaProperty = meta->property(i);
        qDebug() << "property name = " << metaProperty.name();
    }
    // 已知属性名字 反过来找下标
    int index = meta->indexOfProperty(property);
    QMetaProperty metaProperty = meta->property(index);
    // 间接访问
    int currentValue = metaProperty.read(obj).toInt();
    qDebug() << "current value = " << currentValue;
    --currentValue;
    metaProperty.write(obj,currentValue);
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    MyClass o1;
    lose(&o1,"health");
    lose(&o1,"mana");
    return a.exec();
}

```

## 演示
![[Pasted image 20240717172753.png]]
