## mainwindow.h
```C++
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
    void do_timer_timerout();
private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
```
## mainwindow.cpp
```C++
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>
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

void MainWindow::do_timer_timerout(){
    QTime now = QTime::currentTime();

    ui->textBrowser->setText(QString("current is %1").arg(now.toString()));
}
```
## main.cpp
```C++
#include "mainwindow.h"

#include <QApplication>
#include <QTimer>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QTimer timer;
    timer.setInterval(3000);
    timer.start();
    QObject::connect(&timer,&QTimer::timeout,&w,&MainWindow::do_timer_timerout);
    w.show();
    return a.exec();
}
```
## 演示结果
![[Pasted image 20240720170651.png]]
