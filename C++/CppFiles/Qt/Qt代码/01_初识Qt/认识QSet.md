## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QSet>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    QSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    set.insert(4);
    set.insert(5);
    set.insert(5); //重复插入无效的
    qDebug() << set; // 无序的数据
    set.remove(5);
    qDebug() << set;
    qDebug() << set.contains(4);
    qDebug() << set.contains(5);
    return a.exec();
}

```
## 演示结果
![[Pasted image 20240717171102.png]]
