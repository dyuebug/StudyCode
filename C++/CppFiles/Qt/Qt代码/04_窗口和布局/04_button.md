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

private slots:
    void on_pushButton_clicked();

    void on_toolButton_clicked();

    void on_radioButton_toggled(bool checked);

    void on_checkBox_clicked(bool checked);

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
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    qDebug() << "push button";
}

void MainWindow::on_toolButton_clicked()
{
    qDebug() << "tool button";
}

void MainWindow::on_radioButton_toggled(bool checked)
{
    qDebug() << "checked = " << checked << " radio button";
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    qDebug() << "checked = " << checked << " check box";
}
```

## 演示结果
![[Pasted image 20240720163913.png]]
