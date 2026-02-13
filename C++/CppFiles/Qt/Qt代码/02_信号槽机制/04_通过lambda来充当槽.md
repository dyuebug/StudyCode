## mainwindow.cpp
```c++
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->pushButton,&QPushButton::clicked,
                     [=](){
        ui->label->setText("Hello");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
```
## 演示结果
### 初始状态
![[Pasted image 20240717210239.png|400]]
### 点击后

![[Pasted image 20240717210449.png]]
