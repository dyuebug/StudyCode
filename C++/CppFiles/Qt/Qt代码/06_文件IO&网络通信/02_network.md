## mainwindow.h
```C++
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
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
    void do_netstate_changed(QAbstractSocket::SocketState socketState);
    void do_connected();
    void do_readyRead();
private:
    Ui::MainWindow *ui;
    QTcpSocket * socket;
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
    socket = new QTcpSocket(this);
    QObject::connect(socket,&QTcpSocket::stateChanged,
                     this,&MainWindow::do_netstate_changed);
    QObject::connect(socket,&QTcpSocket::connected,
                     this,&MainWindow::do_connected);
    QObject::connect(socket,&QTcpSocket::readyRead,
                     this,&MainWindow::do_readyRead);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    socket->connectToHost("www.jd.com",80);
}
void MainWindow::do_netstate_changed(QAbstractSocket::SocketState socketState){
    qDebug() << "state changed, current = " << socketState;
}
void MainWindow::do_connected(){
    qDebug() << "send";
    QByteArray req("GET / HTTP/1.1\r\n\r\n");
    socket->write(req);
}
void MainWindow::do_readyRead(){
    qDebug() << "recv";
    QByteArray resp = socket->readAll();
    ui->textBrowser->setText(QString(resp));
}
```

## 演示结果
![[Pasted image 20240720171727.png]]
