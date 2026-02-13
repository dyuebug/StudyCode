# 纯代码的方式添加ui元素
![[Pasted image 20240717091457.png|400]]
![[Pasted image 20240717091521.png|400]]

## 结论
在生成界面的时候
- ui文件绘制 一般适用于画框架图
- 纯代码 一般适用于绘制细节
![[Pasted image 20240717091617.png|400]]

# 预设的信号 自定义的槽
![[Pasted image 20240717091757.png|400]]
## 自定义的槽函数
![[Pasted image 20240717091826.png]]
### 总结
1. 在设计师模式新建一个对象 QPushButton
2. 右键点击 转到槽
3. 在MainWindow类里面会多一个成员函数
	- 函数名是on_对象名_信号名
	-  访问权限 private slots(伪关键字)
4. 实现这个成员函数， 即槽函数

## 纯代码的方式加一个槽函数
![[Pasted image 20240717092114.png|500]]

## 增加资源
1、 找到资源 拷贝到项目文件所在的目录
2、 在项目当中添加一个 Qt 资源文件
![[Pasted image 20240717092206.png|500]]

3、 add prefix --> add file

## Qt的数据结构
![[Pasted image 20240717220122.png|500]]
## Qt中和几何相关的数据结构
![[Pasted image 20240717220205.png|500]]
## 编码
![[Pasted image 20240717220248.png|500]]
### Utf-8 Utf-16
![[Pasted image 20240717220330.png|500]]
## Qt当中的字符串QString
![[Pasted image 20240717220412.png|500]]
### 整理
![[Pasted image 20240717220457.png|500]]
### 隐式共享（写时复制）
![[Pasted image 20240717220544.png|500]]

### 构造QString
![[Pasted image 20240717220636.png|500]]
### arg方法 格式化构造字符串
![[Pasted image 20240717220720.png]]

#### arg的后面的参数
![[Pasted image 20240717220800.png|500]]

### 将字符串转换成整数
![[Pasted image 20240717220851.png|500]]
### QString的增删改查
[[认识Qstring#通过下标改字符串 和 查看字符串内容]]
- 增 append prepend insert +=
![[Pasted image 20240717221427.png|500]]

- 删 chop remove clear
![[Pasted image 20240717221453.png|500]]

- 改 []
![[Pasted image 20240717221538.png|500]]
- 查 根据位置找元素 [] at

- 提取子串 left right mid
![[Pasted image 20240717221633.png|500]]
- 根据元素找位置 indexof lastIndexof contains count
![[Pasted image 20240717221710.png|500]]

#### 给出一个文件的路径 求文件的名字 文件所在的目录名
![[Pasted image 20240717221744.png]]

- <font color="#2DC26B"> 分割 split</font>
![[Pasted image 20240717222052.png|600]]

#### 用户发一个请求 name=iPhone15&company=Apple&price=8888
![[Pasted image 20240717222114.png|600]]


## Qt 支持的容器
![[Pasted image 20240717222257.png|600]]
### 关联容器
![[Pasted image 20240717222339.png|600]]
#### QList的增删改查
##### 增 append prepend insert <<
![[Pasted image 20240717222651.png|500]]
##### 删 removeAt removeFirst removeLast removeAll removeOne
![[Pasted image 20240717222716.png|500]]
##### 查 位置 [] at first last
![[Pasted image 20240717222738.png|500]]
##### 查 元素 indexOf lastIndexOf contains count
![[Pasted image 20240717222755.png|500]]
### 栈
![[Pasted image 20240717223124.png|500]]
### 队列
![[Pasted image 20240717223152.png|500]]
### QSet
![[Pasted image 20240717223220.png|500]]


