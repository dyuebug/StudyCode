## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QHash>

class Good{
    QString name;
    QString company;
    double price;
};
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    //QMap<QString,int> myMap;
    QHash<QString,int> myMap;
    myMap.insert("Jordan",23); //insert时不需要一个pair
    myMap.insert("Messi",10);
    myMap.insert("Kobe",8);
    //myMap.insert("Kobe",24);
    qDebug() << myMap;
    myMap["Kobe"] = 24;
    qDebug() << myMap;
    myMap.remove("Kobe");
    qDebug() << myMap;
    qDebug() << myMap.contains("Wuyifan");
    qDebug() << myMap.contains("Messi");
    return a.exec();
}

```

## 演示结果
### QMap演示
![[Pasted image 20240717171639.png]]

### QHas演示（无序)
![[Pasted image 20240717171853.png]]
