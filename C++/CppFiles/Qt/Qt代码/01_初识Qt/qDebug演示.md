## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QPoint>
#include <QLine>
#include <QRect>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    qDebug() << 123;
    qDebug() << 456;

    QPoint point(300,200);
    qDebug() << point;
    QLine line(100,200,50,200);
    qDebug() << line;
    QRect rect(100,100, 300, 200);
    qDebug() << rect;
    return a.exec();
}

```

## mainwindow.h
```c++
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H

```

## mainwindow.cpp
```c++
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


```

## 运行结果
![[Pasted image 20240717161500.png]]
