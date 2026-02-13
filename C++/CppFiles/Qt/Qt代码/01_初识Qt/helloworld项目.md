
## mainwindow.h
```c++
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
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
    void on_changeButton_clicked(); // ui编辑器的转到槽 用 on_ 做前缀
public slots:
    void do_newButton_clicked(); // 纯代码的自定义的槽函数用do_做前缀
private:
    Ui::MainWindow *ui;
    QLabel * myLabel1;
    QPushButton * newButton;
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
    myLabel1 = new QLabel(this);
    myLabel1->setText("Hello Qt too!");
    newButton = new QPushButton("new",this);
    newButton->move(450,390);

    QObject::connect(newButton,SIGNAL(clicked()),this,SLOT(do_newButton_clicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_changeButton_clicked()
{
    ui->myLabel->setText("World");
}
void MainWindow::do_newButton_clicked(){
    if(ui->myLabel->text() == "World"){
        ui->myLabel->setText("Hello");
    }
    else{
        ui->myLabel->setText("World");
    }
}

```

## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

```

## 执行结果
![[Pasted image 20240717160808.png]]
### 按change
![[Pasted image 20240717160837.png]]

### 按new
![[Pasted image 20240717160904.png]]


