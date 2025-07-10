#ifndef __QML_LOGGER_WRAPPER_H_
#define __QML_LOGGER_WRAPPER_H_

#include <QObject>
#include "Logger.h"

class QmlLoggerWrapper : public QObject
{
    Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

public:
    explicit QmlLoggerWrapper(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void debug(const QString& msg) {
        Logger::instance().log(Logger::Debug, msg, "QML", -1, "qml");
    }

    Q_INVOKABLE void info(const QString& msg) {
        Logger::instance().log(Logger::Info, msg, "QML", -1, "qml");
    }

    Q_INVOKABLE void warn(const QString& msg) {
        Logger::instance().log(Logger::Warning, msg, "QML", -1, "qml");
    }

    Q_INVOKABLE void error(const QString& msg) {
        Logger::instance().log(Logger::Error, msg, "QML", -1, "qml");
    }

    Q_INVOKABLE void fatal(const QString& msg) {
        Logger::instance().log(Logger::Fatal, msg, "QML", -1, "qml");
    }
};

#endif // __QML_LOGGER_WRAPPER_H_
