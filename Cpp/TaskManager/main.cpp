#include "MainWindow.h"
#include <QApplication>
#include <QFile>

/**
 * @brief main 函数 - 程序入口
 *
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return int 程序退出码
 */
int main(int argc, char *argv[])
{
    // 创建应用程序对象
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("个人任务管理器");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("C++ 学习者");

    // 加载样式表
    QFile styleFile(":/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.show();

    // 进入事件循环
    return app.exec();
}
