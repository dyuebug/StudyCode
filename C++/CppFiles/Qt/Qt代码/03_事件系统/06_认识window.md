## main.cpp
```C++
#include "mainwindow.h"

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QDialog>
#include <QPushButton>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    QWidget w1;
//    //QWidget w2(&w1,Qt::Window|Qt::FramelessWindowHint);
//    //QDialog w2(&w1);
//    QMainWindow w2(&w1);
//    w1.show();
//    w2.show();
    QWidget w1;
    QPushButton *btn = new  QPushButton("clicked",&w1);
    btn->move(300,400);
    QObject::connect(btn,&QPushButton::clicked,[&](){
        //w1.showMinimized();
        //w1.showMaximized();
        w1.showFullScreen();
    });
    w1.show();
    return a.exec();
}
```

## 演示结果
![[Pasted image 20240719221858.png]]


