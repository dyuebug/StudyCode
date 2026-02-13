## main.cpp
```c++
#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QString>
#include <string>
#include <QLabel>
#include <QMap>
struct Product{
    QString name;
    QString company;
    int price;
};
void getProduct(){
    QString query("name=iPhone15&company=Apple&price=7999");
    QStringList qstrList = query.split("&");
    qDebug() << qstrList;
    QMap<QString,QString> myMap;
    for(int i = 0; i < qstrList.size(); ++i){
        QStringList kv = qstrList[i].split("=");
        myMap[kv[0]] = kv[1];
    }
    Product product;
    product.name = myMap["name"];
    product.company = myMap["company"];
    product.price = myMap["price"].toInt();

    return;
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();
//    char str[] = "hello世界"; // 字符串字面值常量是采用utf-8编码的
//    qDebug() << "size of str = " << sizeof(str);
//    qDebug() << "str = " << str;

//    QString qstr(str);
//    qDebug() << "qstr = " << qstr;

//    QString qstr1("Hello");
//    QString qstr2 = qstr1;
//    qDebug() << "qstr1 data = " << qstr1.data_ptr();
//    qDebug() << "qstr2 data = " << qstr2.data_ptr();
//    qstr2[0] = 'h';
//    qDebug() << "after change";
//    qDebug() << "qstr1 data = " << qstr1.data_ptr();
//    qDebug() << "qstr2 data = " << qstr2.data_ptr();

//    std::string str = "hello";
//    QString qstr = QString::fromStdString(str);

//    QString qstr = QString("My name is %1. I am %2 years old!").arg("Caixukun").arg(25,5,16,QChar('0'));
//    //QString qstr = QString("My name is %2. I am %4 years old!").arg("Caixukun").arg(25);
//    qDebug() << qstr;

//    QString qstr = "12345";
//    qDebug() << qstr.toInt()+100;

//    QString qstr = "hello";
//    qstr[0] = 'H';
//    //qstr.at(0) = 'H'; 这是错误的写法
//    qDebug() << qstr;
//    qDebug() << qstr.front();
//    qDebug() << qstr.back();

//    QString qstr = "Caixukun";
//    qstr.prepend("USA ").append(" Baseketball").insert(13,"Rap ");
//    qDebug() << qstr;

//    QString filepath = "/home/liao/.cargo/env.pdf";
//    int dotIndex = filepath.lastIndexOf(".");
//    QString filetype = filepath.mid(dotIndex+1);
//    qDebug() << filetype;
//    int slashIndex = filepath.lastIndexOf("/");
//    QString fullname = filepath.mid(slashIndex+1);
//    qDebug() << fullname;
//    QString filename = filepath.mid(slashIndex+1,dotIndex-slashIndex-1);
//    qDebug() << filename;

//    QLabel label(filename);
//    label.show();
    getProduct();
    return a.exec();
}

```
### getProduct()演示结果
![[Pasted image 20240717164906.png]]
### 字符串演示结果
#### 编码
![[Pasted image 20240717162440.png]]

#### 字符串地址和更改
![[Pasted image 20240717162729.png]]

#### %1 占位符
![[Pasted image 20240717163401.png]]

#### 字符串转int
![[Pasted image 20240717163557.png]]
#### 通过下标改字符串 和 查看字符串内容
![[Pasted image 20240717163741.png]]
#### 字符串的插入
![[Pasted image 20240717164358.png]]
#### 字符串下标访问
![[Pasted image 20240717164638.png]]
