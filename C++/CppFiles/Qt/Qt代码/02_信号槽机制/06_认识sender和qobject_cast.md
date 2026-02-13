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
private slots:
    //定义一个公共槽
    void publicSlot();
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
```
## mainwindow.cpp
```c++
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->btn1,&QPushButton::clicked,this,&MainWindow::publicSlot);
    QObject::connect(ui->btn2,&QPushButton::clicked,this,&MainWindow::publicSlot);
    QObject::connect(ui->btn3,&QPushButton::clicked,this,&MainWindow::publicSlot);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::publicSlot(){
    QPushButton *btn = qobject_cast<QPushButton *>(this->sender());
    qDebug() << "I clicked btn " << btn->text();
}
```
## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QLabel>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
    {
        QLabel parent;
//        QLabel *pchild1 = new QLabel;
//        delete pchild1;
        
//        QLabel *pchild1 = new QLabel;
//        pchild1->setParent(&parent);
        
        QLabel *pchild1 = new QLabel(&parent);
    }
    
    return a.exec();
}
```

## 演示结果
![[Pasted image 20240717211308.png]]
