## mywidget.h
```C++
#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QPainter>
class MyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MyWidget(QWidget *parent = nullptr);
    void paintEvent(QPaintEvent *ev) override{
        QPixmap pixmap(":/new/prefix1/test1.png");
        QPainter painter(this);
        painter.drawPixmap(QPoint(100,100),pixmap);
        QWidget::paintEvent(ev);
    }
signals:

};

#endif // MYWIDGET_H

```
## mywidget.cpp
```C++
#include "mywidget.h"

MyWidget::MyWidget(QWidget *parent) : QWidget(parent)
{

}
```

## main.cpp
```C++
#include "mainwindow.h"
#include "mywidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MyWidget myw;
    myw.show();
    return a.exec();
}
```

## 演示结果
![[Pasted image 20240719220827.png]]
