#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>

class Logger : public QObject
{
    Q_OBJECT
public:
    enum LogLevel { Debug, Info, Warning, Error, Fatal };
    Q_ENUM(LogLevel)

    static Logger& instance();
    void log(LogLevel level, const QString& message,
        const char* file, int line, const char* function);

    void setLogToConsole(bool enabled);
    void setLogDir(const QString& dirPath); // 默认当前目录
    void setLogLevel(LogLevel level);

    Logger::LogLevel getLogLevel() const;

private:
    explicit Logger(QObject* parent = nullptr);
    QString logFilePath();
    QString levelToString(LogLevel level);

    QFile m_file;
    QTextStream m_stream;
    QMutex m_mutex;
    QString m_logDir;
    bool m_logToConsole = true;
    LogLevel m_logLevel = Debug; // 最小日志级别，默认是Debug
};

// 宏简化调用
#define LOG_DEBUG(msg)   Logger::instance().log(Logger::Debug, msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_INFO(msg)    Logger::instance().log(Logger::Info, msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_WARN(msg)    Logger::instance().log(Logger::Warning, msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_ERROR(msg)   Logger::instance().log(Logger::Error, msg, __FILE__, __LINE__, Q_FUNC_INFO)
#define LOG_FATAL(msg)   Logger::instance().log(Logger::Fatal, msg, __FILE__, __LINE__, Q_FUNC_INFO)

#endif // __LOGGER_H__