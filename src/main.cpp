#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

#include "config/ConfigCenter.h"
#include "logger/Logger.h"
#include "logger/QmlLoggerWrapper.h"

#include "model/FileSystemModel.h"
#include "model/LineNumberModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 可选配置
    /*Logger::instance().setLogDir(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));*/
    //Logger::instance().setLogToConsole(true);
	// 设置日志目录为当前目录
	Logger::instance().setLogDir(QDir::currentPath());
    Logger::instance().setLogToConsole(false);
    Logger::instance().setLogLevel(Logger::Debug); 

    LOG_INFO("程序启动中 ...");

    ConfigCenter::instance()->loadAllConfigs();

    QQmlApplicationEngine engine;

    qmlRegisterSingletonType<QmlLoggerWrapper>("Logger", 1, 0, "Logger",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return new QmlLoggerWrapper;
        });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // 连接程序退出信号，记录退出日志
    QObject::connect(
        &app,
        &QGuiApplication::aboutToQuit,
        []() {
            LOG_INFO("程序正在退出...");
            // 如果需要，可以在这里添加额外的清理工作
        });

    engine.loadFromModule("EvaEdit", "Main");

    return app.exec();
}
