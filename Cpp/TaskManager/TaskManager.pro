#-------------------------------------------------
# 个人任务管理器项目文件
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TaskManager
TEMPLATE = app

# C++11 支持
CONFIG += c++11

# 源文件
SOURCES += \
    main.cpp \
    MainWindow.cpp \
    Task.cpp \
    Category.cpp \
    DatabaseManager.cpp \
    TaskDialog.cpp

# 头文件
HEADERS += \
    MainWindow.h \
    Task.h \
    Category.h \
    DatabaseManager.h \
    TaskDialog.h

# 资源文件
RESOURCES += \
    resources.qrc

# 默认部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
