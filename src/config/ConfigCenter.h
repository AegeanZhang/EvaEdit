#ifndef __CONFIG_CENTER_H__
#define __CONFIG_CENTER_H__

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>
#include <QString>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QtQml/qqmlregistration.h>
#include <QSize>
#include <QPoint>
#include "ConfigKeys.h"
#include "ConfigPaths.h"

class ConfigCenter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static ConfigCenter* instance();

    Q_ENUM(ConfigType)

    // 通用配置访问方法
    Q_INVOKABLE QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString& key, const QVariant& value, ConfigType type = ConfigType::UserSettings);

    // 检查配置是否存在
    Q_INVOKABLE bool hasKey(const QString& key, ConfigType type = ConfigType::UserSettings);

    // 删除配置项
    Q_INVOKABLE void removeKey(const QString& key, ConfigType type = ConfigType::UserSettings);

    // 重置到系统默认设置
    Q_INVOKABLE void resetToSystemDefaults();

    // 保存和加载配置
    Q_INVOKABLE void saveConfig(ConfigType type = ConfigType::UserSettings);
    Q_INVOKABLE void loadConfig(ConfigType type = ConfigType::UserSettings);
    Q_INVOKABLE void loadAllConfigs();

    // 提供给QML直接使用的配置项属性
    // 窗口配置
    Q_PROPERTY(QSize windowSize READ windowSize WRITE setWindowSize NOTIFY windowSizeChanged)
    Q_PROPERTY(QPoint windowPosition READ windowPosition WRITE setWindowPosition NOTIFY windowPositionChanged)
    Q_PROPERTY(bool isFullScreen READ isFullScreen WRITE setIsFullScreen NOTIFY isFullScreenChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle NOTIFY windowTitleChanged)

    // 编辑器配置
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY showLineNumbersChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontFamilyChanged)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
    Q_PROPERTY(int tabSize READ tabSize WRITE setTabSize NOTIFY tabSizeChanged)

    // 文件配置
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath WRITE setCurrentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(bool restoreSession READ restoreSession WRITE setRestoreSession NOTIFY restoreSessionChanged)

    // 配置属性的访问器
    QSize windowSize() const;
    void setWindowSize(const QSize& size);

    QPoint windowPosition() const;
    void setWindowPosition(const QPoint& pos);

    bool isFullScreen() const;
    void setIsFullScreen(bool fullscreen);

    QString windowTitle() const;
    void setWindowTitle(const QString& title);

    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);

    int fontSize() const;
    void setFontSize(int size);

    QString fontFamily() const;
    void setFontFamily(const QString& family);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    int tabSize() const;
    void setTabSize(int size);

    QStringList recentFiles() const;

    QString currentFilePath() const;
    void setCurrentFilePath(const QString& path);

    bool restoreSession() const;
    void setRestoreSession(bool restore);

    // 会话状态方法
    Q_INVOKABLE void addRecentFile(const QString& filePath);
    Q_INVOKABLE void clearRecentFiles();
    Q_INVOKABLE QVariantList getOpenFiles() const;
    Q_INVOKABLE void setOpenFiles(const QVariantList& files);
    Q_INVOKABLE int getActiveTabIndex() const;
    Q_INVOKABLE void setActiveTabIndex(int index);

signals:
    void configChanged(const QString& key, ConfigType type);

    // 特定配置的信号
    void windowSizeChanged();
    void windowPositionChanged();
    void isFullScreenChanged();
    void windowTitleChanged();
    void showLineNumbersChanged();
    void fontSizeChanged();
    void fontFamilyChanged();
    void wordWrapChanged();
    void tabSizeChanged();
    void recentFilesChanged();
    void currentFilePathChanged();
    void restoreSessionChanged();

private:
    explicit ConfigCenter(QObject* parent = nullptr);
    ~ConfigCenter();

    // 获取配置文件路径
    QString getConfigFilePath(ConfigType type) const;

    // 根据嵌套路径获取 JSON 值
    QJsonValue getNestedValue(const QJsonObject& obj, const QString& path) const;
    void setNestedValue(QJsonObject& obj, const QString& path, const QJsonValue& value);

    // 初始化默认系统配置
    void initializeSystemDefaults();

    static ConfigCenter* m_instance;

    // 配置数据
    QJsonObject m_systemSettings;
    QJsonObject m_userSettings;
    QJsonObject m_stateData;

    // 最大最近文件数
    const int MAX_RECENT_FILES = 10;
};

#endif // __CONFIG_CENTER_H__