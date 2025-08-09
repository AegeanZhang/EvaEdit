#ifndef __FILE_CONTROLLER_H__
#define __FILE_CONTROLLER_H__

#include <QObject>
#include <QStringList>
#include <QDateTime>
#include <QHash>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qqmlengine.h>
#include <QElapsedTimer>

class ConfigCenter;

class FileController : public QObject
{
    Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

public:
    static FileController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);
    static FileController* instance();

    // 文件操作
    Q_INVOKABLE bool openFile(const QString& filePath);
    Q_INVOKABLE bool saveFile(const QString& filePath = "");
    Q_INVOKABLE bool saveAsFile(const QString& filePath);
    Q_INVOKABLE bool newFile();

    // 文件状态
    Q_INVOKABLE bool isFileModified(const QString& filePath) const;
    Q_INVOKABLE QString getFileEncoding(const QString& filePath) const;
    Q_INVOKABLE qint64 getFileSize(const QString& filePath) const;

    // 最近文件
    Q_INVOKABLE QStringList getRecentFiles() const;
    Q_INVOKABLE void addToRecentFiles(const QString& filePath);
    Q_INVOKABLE void clearRecentFiles();

    // 文件修改状态管理
    Q_INVOKABLE void markFileModified(const QString& filePath, bool modified = true);

    // 工具方法
    Q_INVOKABLE bool fileExists(const QString& filePath);
    Q_INVOKABLE QString getFileName(const QString& filePath);
    Q_INVOKABLE QString getFileDirectory(const QString& filePath);
    Q_INVOKABLE QDateTime getFileLastModified(const QString& filePath);

    // 【新增】编辑器内容管理
    Q_INVOKABLE void updateEditorContent(const QString& filePath, const QString& content);

signals:
    void fileOpened(const QString& filePath, const QString& content);
    void fileSaved(const QString& filePath);
    void fileModifiedChanged(const QString& filePath, bool modified);
    void recentFilesChanged();

    // 【新增】强制内容更新信号
    void forceContentUpdateRequested(const QString& filePath);

private:
    explicit FileController(QObject* parent = nullptr);
    ~FileController();

    // 内部辅助方法
    bool saveFileContent(const QString& filePath, const QString& content);
    QString detectFileEncoding(const QString& filePath);
    QString convertFromEncoding(const QByteArray& data, const QString& encoding); // 【新增】Qt 6 编码转换
    bool isSupportedFileType(const QString& filePath);
    QString getCurrentFileContent(const QString& filePath);
    void createBackup(const QString& filePath);
    void cleanupBackups(const QString& originalFilePath);

    // 单例实例
    static FileController* m_instance;

    // 成员变量
    ConfigCenter* m_configCenter;
    QHash<QString, bool> m_fileModifiedStatus;      // 文件修改状态映射
    QHash<QString, QString> m_fileEncodings;        // 文件编码映射
    QHash<QString, QString> m_editorContents;       // 【新增】编辑器当前内容缓存

    // 【新增】性能监控
    QHash<QString, QElapsedTimer> m_updateTimers;   // 更新计时器
    QHash<QString, int> m_updateCounts;             // 更新计数器

    // 常量
    static const qint64 MAX_FILE_SIZE;               // 最大文件大小限制
    static const QStringList SUPPORTED_TEXT_EXTENSIONS; // 支持的文本文件扩展名
};

#endif // __FILE_CONTROLLER_H__