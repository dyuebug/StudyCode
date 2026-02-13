## a.h
```c++
#ifndef A_H
#define A_H

#include <QObject>

class A : public QObject
{
    Q_OBJECT
public:
    explicit A(QObject *parent = nullptr);

signals:
    void signalA();
};

#endif // A_H
```
## a.cpp
```c++
#include "a.h"

A::A(QObject *parent) : QObject(parent)
{

}

```

## b.h
```c++
#ifndef B_H
#define B_H

#include <QObject>
#include <QDebug>
class B : public QObject
{
    Q_OBJECT
public:
    explicit B(QObject *parent = nullptr);

signals:
    void signalB();
public slots:
    void slotB(){
        qDebug() << "I am slot B";
    }
};

#endif // B_H
```
## b.cpp
```c++
#include "b.h"

B::B(QObject *parent) : QObject(parent)
{

}
```

## c.h
```c++
#ifndef C_H
#define C_H

#include <QObject>
#include <QDebug>
class C : public QObject
{
    Q_OBJECT
public:
    explicit C(QObject *parent = nullptr);

signals:
    void signalC();
public slots:
    void slotC(){
        qDebug() << "I am slot C";
    }
};

#endif // C_H
```
## c.cpp
```c++
#include "c.h"

C::C(QObject *parent) : QObject(parent)
{

}
```

## main.cpp
```c++
#include "mainwindow.h"
#include "a.h"
#include "b.h"
#include "c.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//    MainWindow w;
//    w.show();
    A a;
    B b;
    C c;
    QObject::connect(&a,&A::signalA,&b,&B::slotB);
    QObject::connect(&a,&A::signalA,&c,&C::slotC);
    qDebug() << "emit signal A";
    emit a.signalA();
    QObject::connect(&b,&B::signalB,&c,&C::slotC);
    qDebug() << "emit signal B";
    emit b.signalB();
    QObject::connect(&c,&C::signalC,&a,&A::signalA);
    qDebug() << "emit signal C";
    emit c.signalC();
    return app.exec();
}
```
## 演示结果
![[Pasted image 20240717203758.png]]
