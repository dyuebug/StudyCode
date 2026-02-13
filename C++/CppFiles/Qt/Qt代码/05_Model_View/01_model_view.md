## mainwindow.h
```C++
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/QtWidgets>
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
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QTableView * view;
};
#endif // MAINWINDOW_H
```

## mainwindow.cpp
```C++
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);//这句代码执行以后，再构建其他的对象
    model = new QStandardItemModel(3,4,this);
    QStringList titles = {"id","name","age","image"};
    model->setHorizontalHeaderLabels(titles);
    QStandardItem * id = new QStandardItem("1001");
    model->setItem(0,0,id);
    QStandardItem * name = new QStandardItem("Kobe");
    model->setItem(0,1,name);
    QStandardItem * age = new QStandardItem("46");
    model->setItem(0,2,age);
    QStandardItem * image = new QStandardItem(QIcon(":/new/prefix1/images/001_kobe.jpeg"),"Kobe");
    model->setItem(0,3,image);
    view = new QTableView(this);
    view->setModel(model);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(view);
    layout->addWidget(ui->pushButton);
    ui->centralwidget->setLayout(layout);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QStandardItem * id = new QStandardItem("1002");
    QStandardItem * name = new QStandardItem("Wuyifan");
    QStandardItem * age = new QStandardItem("30");
    QStandardItem * image = new QStandardItem(QIcon(":/new/prefix1/images/002_wuyifan.jpeg"),"Wuyifan");
    QList<QStandardItem *> row;
    row << id << name << age << image;
    model->appendRow(row);
}
```
## 演示结果
![[Pasted image 20240720170149.png]]
