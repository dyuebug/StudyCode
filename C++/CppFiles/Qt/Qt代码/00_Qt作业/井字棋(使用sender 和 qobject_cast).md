## mainwindow.h
```c++
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QStack>
#include <QList>
#include <QDebug>

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
    //连接信号槽
    void connectButtons();

    void on_claearButton_clicked();

    void on_undoButton_clicked();

private:
    Ui::MainWindow *ui;
    enum class Player { X, O };
    Player currentPlayer;
    QList<QPushButton *> buttonList;

    QStack<int> opStack; //记录操作

    void initializeButtons();  // 初始化按钮数组
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
    ,currentPlayer(Player::X)
{
    ui->setupUi(this);
    buttonList << ui->button1
               << ui->button2
               << ui->button3
               << ui->button4
               << ui->button5
               << ui->button6
               << ui->button7
               << ui->button8
               << ui->button9;
      connectButtons(); // 连接所有按钮的点击事件到公共槽函数
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::connectButtons()
{
    foreach (QPushButton *button, buttonList) {
        QObject::connect(button, &QPushButton::clicked, this,   &MainWindow::publicSlot);
    }
}

void MainWindow::publicSlot(){
    QPushButton *btn = qobject_cast<QPushButton*>(this->sender()); // 获取发送信号的按钮
    if (btn && btn->text().isEmpty()) { // 检查按钮不为空且文本为空
        int index = buttonList.indexOf(btn); // 获取按钮在列表中的索引
        if (index != -1) { // 确保找到了按钮在列表中的位置
            opStack.push(index); // 将按钮的索引入栈，记录操作步骤

            if (currentPlayer == Player::X) {
                btn->setText("X");
                currentPlayer = Player::O;
            } else {
                btn->setText("O");
                currentPlayer = Player::X;
            }
        }
    }
}

void MainWindow::on_claearButton_clicked()
{
    foreach (QPushButton *button, buttonList) {
           button->setText("");
       }
       currentPlayer = Player::X;  // 重置当前玩家
}

void MainWindow::on_undoButton_clicked()
{
    if (opStack.empty()) {
           return;
       }

       int top = opStack.top();
       opStack.pop();
       buttonList[top]->setText("");
       currentPlayer = (currentPlayer == Player::O) ? Player::X : Player::O;
}
```

## 演示结果
![[Pasted image 20240717204530.png]]
