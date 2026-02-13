## main.cpp
```C++
#include "mainwindow.h"

#include <QApplication>
#include <QtWidgets/QtWidgets>
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//    MainWindow w;
//    w.resize(800,600);
//    //QWidget *container1 = new QWidget(&w);
//    QGroupBox *container1 = new QGroupBox(&w);
//    container1->move(50,50);
//    container1->resize(700,220);
//    QRadioButton *a = new QRadioButton("a",container1);
//    QRadioButton *b = new QRadioButton("b",container1);
//    b->move(0,70);
//    QRadioButton *c = new QRadioButton("c",container1);
//    c->move(0,140);
//    //QWidget *container2 = new QWidget(&w);
//    QGroupBox *container2 = new QGroupBox(&w);
//    container2->move(50,290);
//    container2->resize(700,220);
//    QRadioButton *d = new QRadioButton("d",container2);
//    QRadioButton *e = new QRadioButton("e",container2);
//    e->move(0,70);
//    QRadioButton *f = new QRadioButton("f",container2);
//    f->move(0,140);
//    w.show();

    QWidget w;
    QRadioButton *a = new QRadioButton("a");
    QRadioButton *b = new QRadioButton("b");
    QRadioButton *c = new QRadioButton("c");
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(a);
    layout->addWidget(b);
    layout->addWidget(c);
    w.setLayout(layout);
    w.show();
    return app.exec();
}

```

## 演示结果
![[Pasted image 20240720165453.png]]
