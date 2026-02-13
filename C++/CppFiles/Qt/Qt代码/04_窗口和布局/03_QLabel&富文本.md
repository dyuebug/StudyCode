## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QLabel>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.resize(800,600);
    QLabel * label = new QLabel(&w);
    label->resize(700,500);
    label->move(50,50);
    label->setFrameStyle(QFrame::Panel|QFrame::Plain);
    label->setLineWidth(3);
    label->setMidLineWidth(3);
    // 平凡文本
    // label->setText("Hello world!");
    // 富文本 markdown
    label->setTextFormat(Qt::MarkdownText);
    label->setText("# Hello world! \n## nihao \n- 1\n- 2\n");
    w.show();
    return a.exec();
}
```

## 演示结果
![[Pasted image 20240720163414.png]]
