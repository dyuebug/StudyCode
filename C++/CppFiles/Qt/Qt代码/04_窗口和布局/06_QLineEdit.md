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
    void changeCurrent(const QString &text);
    void changeHistory();
private:
    Ui::MainWindow *ui;
    QString currentInput;
    QString historyInput;
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

void MainWindow::changeCurrent(const QString &text){
    currentInput = text;
    ui->textBrowser->setText(QString("current = %1\nhistory = %2").arg(currentInput).arg(historyInput));
}
void MainWindow::changeHistory(){
    historyInput.append(currentInput);
    currentInput.clear();
    ui->textBrowser->setText(QString("current = %1\nhistory = %2").arg(currentInput).arg(historyInput));
}
```

## main.cpp
```C++
#include "mainwindow.h"

#include <QApplication>
#include <QLineEdit>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QLineEdit *lineEdit = new QLineEdit(&w);
    lineEdit->resize(720,30);
    lineEdit->move(40,20);

    QObject::connect(lineEdit,&QLineEdit::textChanged,&w,&MainWindow::changeCurrent);
    QObject::connect(lineEdit,&QLineEdit::returnPressed,&w,&MainWindow::changeHistory);
    w.show();
    return a.exec();
}
```

## 演示结果
![[Pasted image 20240720165135.png]]

