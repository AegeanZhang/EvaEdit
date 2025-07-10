#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

#include "logger/Logger.h"

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

    LOG_INFO("程序启动成功");
    LOG_DEBUG("调试信息");
    LOG_WARN("警告示例");
    LOG_ERROR("错误日志");
    // LOG_FATAL("致命错误"); // 会终止程序

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("EvaEdit", "Main");

    return app.exec();
}
