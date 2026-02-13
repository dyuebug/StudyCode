## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QList>
#include <QDebug>
#include <QStack>
#include <QQueue>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    QList<int> list;
    list << 1 << 2 << 3 << 3 << 2 << 1;
    qDebug() << list;
    list.removeAll(3);
    qDebug() << list;
    list.removeFirst();
    qDebug() << list;
    qDebug() << list[2];
    return a.exec();
}

```
### 演示结果

![[Pasted image 20240717170826.png]]
