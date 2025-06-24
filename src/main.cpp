#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "model/FileSystemModel.h"
#include "model/LineNumberModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("EvaEdit", "Main");

    /*engine.loadFromModule("FileSystemModule", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;*/

    return app.exec();
}
