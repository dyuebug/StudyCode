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
public slots:
    void do_change_item(const QString &text);
private:
    Ui::MainWindow *ui;
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
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_change_item(const QString &text){
    ui->textBrowser->setText(QString("我最喜欢的明星是%1").arg(text));
}
```
## main.cpp 
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <QIntValidator>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QComboBox * comboBox = new QComboBox(&w);
    comboBox->move(150,100);
    comboBox->setEditable(true);
    QStringList items ={"1111","2222","3333","abcd"};
   comboBox->addItems(items);
//    QStringList items ={"Caixukun","Dingzhen","Wangyuan"};
//    comboBox->addItems(items);

    QIntValidator *validator = new QIntValidator(100,3000,&w);
    comboBox->setValidator(validator);
//    QObject::connect(comboBox,&QComboBox::currentTextChanged,
//                     &w,&MainWindow::do_change_item);
    QObject::connect(comboBox,&QComboBox::currentTextChanged,
                     [validator](const QString &text){
        qDebug() << "slot is called! text = " << text;
        QString tmp = text;
        int pos = 0;
        qDebug() << validator->validate(tmp,pos);
    });
    w.show();
    return a.exec();
}
```
## 演示结果
![[Pasted image 20240720164630.png]]
