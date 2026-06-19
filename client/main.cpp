/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * BntechEyeFriend - 摄像头实时画面与语音交互界面
 * 基于 Qt 6.6 构建，支持跨平台运行
 */

#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BntechEyeFriend");
    app.setApplicationVersion("1.0.0");

#ifdef Q_OS_WASM
    // 使用系统默认字体，避免尝试加载不存在的字体文件
    // 这将防止在外部服务器部署时按钮文字无法显示的问题
    qDebug() << "Using system default font for WebAssembly";
    QFont font("Arial"); // 使用常见的系统字体
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);
#endif

    MainWindow window;
    window.show();

    return app.exec();
}
