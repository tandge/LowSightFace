/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * BntechEyeFriend - 摄像头实时画面与语音交互界面
 * 基于 Qt 6.6 构建，支持跨平台运行
 */

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BntechEyeFriend");
    app.setApplicationVersion("1.0.0");

    MainWindow window;
    window.show();

    return app.exec();
}
