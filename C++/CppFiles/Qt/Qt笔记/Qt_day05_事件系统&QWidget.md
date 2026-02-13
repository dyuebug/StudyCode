Qt 的事件系统是 Qt 框架核心的一部分，用于处理用户交互、系统消息以及其他事件。它提供了一种灵活且强大的机制，使得程序可以响应各种事件，例如鼠标点击、键盘输入、窗口关闭等。

## 什么是 Qt 的事件系统？

Qt 的事件系统是一个基于事件循环的机制。事件是由操作系统或应用程序产生的消息，这些消息通知应用程序发生了某种操作，例如用户点击了按钮或移动了鼠标。Qt 使用事件对象（继承自 `QEvent` 类）来封装这些消息，并通过事件循环（`QEventLoop` 类）来分发和处理这些事件。
![[Pasted image 20240718222245.png]]

## 为什么使用 Qt 的事件系统？

1. **解耦事件和处理逻辑**：通过事件系统，事件的产生和处理是解耦的。事件可以在任何地方产生，而处理逻辑则可以在事件处理函数中实现。
2. **灵活性和扩展性**：Qt 的事件系统支持自定义事件，用户可以创建自己的事件类型，以处理特定的应用需求。
3. **跨平台性**：Qt 的事件系统在不同操作系统上提供一致的事件处理接口，简化了跨平台开发。
4. **内置支持**：Qt 提供了丰富的内置事件类型和处理机制，简化了常见 UI 组件的事件处理。

## 如何使用 Qt 的事件系统？

### 1. 事件对象

`QEvent` 是所有事件的基类，Qt 提供了许多预定义的事件类型，例如 `QMouseEvent`、`QKeyEvent`、`QPaintEvent` 等。每个事件类型封装了特定的事件信息。

### 2. 事件循环

`QEventLoop` 负责管理事件队列和事件分发。通常，事件循环在 `QApplication` 或 `QCoreApplication` 对象中自动启动。

### 3. 事件处理

事件处理有两种主要方式：

- **事件过滤器**：可以拦截和处理事件。
- **事件处理函数**：重写 QWidget 或 QObject 的事件处理函数来处理特定事件。

### 示例代码

#### 示例 1：事件处理函数

在下面的示例中，我们创建一个简单的窗口，并重写了鼠标按下和键盘按下事件的处理函数。
```C++
#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

class MyWidget : public QWidget {
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            qDebug() << "Left mouse button pressed at" << event->pos();
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Escape) {
            qDebug() << "Escape key pressed";
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MyWidget widget;
    widget.show();
    return app.exec();
}

```

#### 示例 2：事件过滤器

在下面的示例中，我们创建一个简单的窗口，并使用事件过滤器来拦截和处理事件。
```C++
#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

class MyFilter : public QObject {
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                qDebug() << "Left mouse button pressed at" << mouseEvent->pos();
                return true; // 事件已处理，不继续传递
            }
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                qDebug() << "Escape key pressed";
                return true; // 事件已处理，不继续传递
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QWidget widget;
    MyFilter filter;
    widget.installEventFilter(&filter);
    widget.show();
    return app.exec();
}

```
### 解释

1. **事件处理函数**：重写 `mousePressEvent` 和 `keyPressEvent` 函数，以处理鼠标按下和键盘按下事件。`mousePressEvent` 函数在用户点击窗口时被调用，`keyPressEvent` 函数在用户按下键盘时被调用。
    
2. **事件过滤器**：创建一个自定义的事件过滤器类 `MyFilter`，并重写 `eventFilter` 函数。通过调用 `installEventFilter` 方法，将事件过滤器安装到窗口上。事件过滤器可以拦截事件，并决定是否继续传递事件。
    

### 总结

Qt 的事件系统通过事件对象和事件循环管理和分发事件。它提供了灵活且强大的事件处理机制，使得应用程序可以轻松地响应用户交互和系统消息。通过事件处理函数和事件过滤器，开发者可以定制和扩展事件处理逻辑，以满足特定的应用需求。
## 事件系统的常见使用场景
![[Pasted image 20240718222305.png]]
## 事件系统的处理流程
![[Pasted image 20240718222414.png]]

### 重写event函数
![[Pasted image 20240718222458.png]]
### 场景
![[Pasted image 20240718222534.png]]

### 处理event
![[Pasted image 20240718222557.png]]

#### event handler 
![[Pasted image 20240718222630.png]]
![[Pasted image 20240718222650.png]]

#### 先event 再event_handler 
![[Pasted image 20240718222723.png]]

### 给一个图片 加 点击的功能
![[Pasted image 20240718222748.png]]
### MyLabel的设计
![[Pasted image 20240719085813.png]]

#### 在事件处理过程中，发射信号
![[Pasted image 20240719085850.png]]
#### 事件在多个对象之间传递
![[Pasted image 20240719085932.png]]

![[Pasted image 20240719085943.png|500]]
### 事件系统的处理流程
![[Pasted image 20240719090041.png]]

#### 在event_handler中可以决定是否将事件传递给上级
![[Pasted image 20240719090301.png]]

### 想要去处理事件 代码怎么写
![[Pasted image 20240719090326.png]]
#### 新的问题
![[Pasted image 20240719090348.png]]
#### 一份事件处理流程，同时对多个不同类型的对象生效
![[Pasted image 20240719090415.png]]

### QObject::installEventFilter
![[Pasted image 20240719090443.png]]
### 使用过滤器的流程
![[Pasted image 20240719090523.png]]

### 事件的整体处理流程
![[Pasted image 20240719090552.png|400]]
### 什么情况下适合使用过滤器？
![[Pasted image 20240719090626.png|400]]

## QWidget
![[Pasted image 20240719090708.png]]

### QWidget的本质
![[Pasted image 20240719090755.png]]

### QPaintDevice是一个画布
![[Pasted image 20240719090820.png]]
\
![[Pasted image 20240719090833.png|400]]
### 窗口
![[Pasted image 20240719090914.png]]
设置为子对象，只有一个窗口出现
![[Pasted image 20240719090928.png]]

#### 让一个孩子也是窗口
![[Pasted image 20240719091042.png]]

#### 有父亲&&存在Qt::Window flag
![[Pasted image 20240719091110.png]]

#### 无边框窗口

![[Pasted image 20240719091151.png]]

#### QDialog QMainWindow 以及它们的子类（窗口）
![[Pasted image 20240719091207.png|300]]
![[Pasted image 20240719091245.png]]

### QWidget中关于窗口方法
![[Pasted image 20240719091311.png]]

#### hide会藏成后台进程
![[Pasted image 20240719091344.png]]
#### 改标题
![[Pasted image 20240719091409.png]]
#### 改变显示的大小
![[Pasted image 20240719091449.png]]

#### update
![[Pasted image 20240719091514.png]]

#### 大小和位置
![[Pasted image 20240719091543.png]]



