#include "Logger.h"
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

Logger::Logger(QObject* parent) : QObject(parent)
{
    m_logDir = QDir::currentPath(); // 默认日志目录
}

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogDir(const QString& dirPath)
{
    m_logDir = dirPath;
}

void Logger::setLogToConsole(bool enabled)
{
    m_logToConsole = enabled;
}

void Logger::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() const
{
    return m_logLevel; // 不需要加锁，读取一个整型值是原子的
}

QString Logger::logFilePath()
{
    QString date = QDate::currentDate().toString("yyyyMMdd");
    return m_logDir + QString("/EvaEdit_%1.log").arg(date);
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case Debug:   return "DEBUG";
    case Info:    return "INFO";
    case Warning: return "WARN";
    case Error:   return "ERROR";
    case Fatal:   return "FATAL";
    default:      return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const QString& message, const char* file, int line, const char* function)
{
    // 如果日志级别低于设置的最低级别，直接返回不处理
    if (level < m_logLevel)
        return;

    QMutexLocker locker(&m_mutex);

    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString logLine = QString("[%1] [%2] %3 (%4:%5 in %6)")
        .arg(time, levelToString(level), message,
            QFileInfo(file).fileName()).arg(line).arg(function);

    if (!m_file.isOpen()) {
        m_file.setFileName(logFilePath());
        m_file.open(QIODevice::Append | QIODevice::Text);
        m_stream.setDevice(&m_file);
    }

    m_stream << logLine << "\n";
    m_stream.flush();

    if (m_logToConsole) {
        qDebug().noquote() << logLine;
    }

    if (level == Fatal) {
        abort();
    }
}