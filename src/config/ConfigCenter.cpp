#include "ConfigCenter.h"
#include <QDebug>
#include <QCoreApplication>
#include <QJsonDocument>

#include "../logger/Logger.h"

#include "ConfigKeys.h"
#include "ConfigPaths.h"

ConfigCenter* ConfigCenter::m_instance = nullptr;

ConfigCenter* ConfigCenter::instance()
{
    if (!m_instance)
        m_instance = new ConfigCenter();
    return m_instance;
}

ConfigCenter::ConfigCenter(QObject* parent)
    : QObject(parent)
{
    // 加载所有配置文件
    loadAllConfigs();

    // 如果系统设置为空，初始化默认值
    if (m_systemSettings.isEmpty()) {
        initializeSystemDefaults();
        saveConfig(ConfigType::SystemSettings);
    }
}

ConfigCenter::~ConfigCenter()
{
    // 保存用户设置和状态
    saveConfig(ConfigType::UserSettings);
    saveConfig(ConfigType::StateData);
}

void ConfigCenter::loadConfig(ConfigType type)
{
    QString filePath = ConfigPaths::getConfigFilePath(type);
    QFile file(filePath);

	LOG_DEBUG("加载 [" + configTypeToString(type) + "] 配置文件: " + filePath);

    if (!file.exists() && type == ConfigType::SystemSettings) {
        // 系统配置文件不存在，初始化默认配置
        initializeSystemDefaults();
        saveConfig(ConfigType::SystemSettings);
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN("无法打开配置文件: " + filePath);
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        LOG_WARN("配置文件格式无效: " + filePath);
        return;
    }

    switch (type) {
    case ConfigType::SystemSettings:
        m_systemSettings = doc.object();
        break;
    case ConfigType::UserSettings:
        m_userSettings = doc.object();
        break;
    case ConfigType::StateData:
        m_stateData = doc.object();
        break;
    }
}

void ConfigCenter::saveConfig(ConfigType type)
{
    QString filePath = ConfigPaths::getConfigFilePath(type);
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_WARN("无法写入配置文件: " + filePath);
        return;
    }

    QJsonDocument doc;
    switch (type) {
    case ConfigType::SystemSettings:
        doc = QJsonDocument(m_systemSettings);
        break;
    case ConfigType::UserSettings:
        doc = QJsonDocument(m_userSettings);
        break;
    case ConfigType::StateData:
        doc = QJsonDocument(m_stateData);
        break;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

void ConfigCenter::loadAllConfigs()
{
    // 使用静态标志确保只加载一次
    static bool configsLoaded = false;
    if (configsLoaded) {
        LOG_DEBUG("配置已加载过，跳过重复加载");
        return;
    }

    loadConfig(ConfigType::SystemSettings);
    loadConfig(ConfigType::UserSettings);
    loadConfig(ConfigType::StateData);

    configsLoaded = true;
    LOG_DEBUG("已完成所有配置加载");
}

void ConfigCenter::initializeSystemDefaults()
{
    // 窗口默认设置
    m_systemSettings[ConfigKeys::WINDOW_SIZE] = QJsonObject{ {"width", 1100}, {"height", 600} };
    m_systemSettings[ConfigKeys::WINDOW_IS_FULL_SCREEN] = false;
    m_systemSettings[ConfigKeys::WINDOW_TITLE] = "${activeFileName} - EvaEdit";

    // 编辑器默认设置
    m_systemSettings[ConfigKeys::EDITOR_SHOW_LINE_NUMBERS] = true;
    m_systemSettings[ConfigKeys::EDITOR_FONT_SIZE] = 14;
    m_systemSettings[ConfigKeys::EDITOR_FONT_FAMILY] = "Consolas, 'Courier New', monospace";
    m_systemSettings[ConfigKeys::EDITOR_WORD_WRAP] = false;
    m_systemSettings[ConfigKeys::EDITOR_TAB_SIZE] = 4;

    // 文件默认设置
    m_systemSettings[ConfigKeys::FILES_RESTORE_SESSION] = true;
    m_systemSettings[ConfigKeys::FILES_MAX_RECENT_FILES] = MAX_RECENT_FILES;
}

QJsonValue ConfigCenter::getNestedValue(const QJsonObject& obj, const QString& path) const
{
    QStringList parts = path.split('.');
    QJsonObject current = obj;

    // 遍历路径的每一部分直到倒数第二级
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i]) || !current[parts[i]].isObject())
            return QJsonValue();
        current = current[parts[i]].toObject();
    }

    // 获取最后一级的值
    QString lastPart = parts.last();
    if (!current.contains(lastPart))
        return QJsonValue();

    return current[lastPart];
}

void ConfigCenter::setNestedValue(QJsonObject& obj, const QString& path, const QJsonValue& value)
{
    int dotPos = path.indexOf('.');
    if (dotPos == -1) {
        // 没有更多嵌套，直接设置值
        obj[path] = value;
        return;
    }

    // 获取当前级别的键和剩余路径
    QString currentKey = path.left(dotPos);
    QString remainingPath = path.mid(dotPos + 1);

    // 确保当前键存在且是对象
    if (!obj.contains(currentKey) || !obj[currentKey].isObject()) {
        obj[currentKey] = QJsonObject();
    }

    // 获取子对象，修改它，然后重新放回
    QJsonObject childObj = obj[currentKey].toObject();
    setNestedValue(childObj, remainingPath, value);
    obj[currentKey] = childObj;
}

QVariant ConfigCenter::getValue(const QString& key, const QVariant& defaultValue) const
{
    // 首先检查用户设置
    QJsonValue userValue = getNestedValue(m_userSettings, key);
    if (!userValue.isUndefined() && !userValue.isNull())
        return userValue.toVariant();

    // 然后检查系统设置
    QJsonValue systemValue = getNestedValue(m_systemSettings, key);
    if (!systemValue.isUndefined() && !systemValue.isNull())
        return systemValue.toVariant();

    // 如果是状态相关的键
    if (ConfigKeys::isStateKey(key)) {
        QString stateKey = ConfigKeys::extractStateKey(key);
        QJsonValue stateValue = getNestedValue(m_stateData, stateKey);
        if (!stateValue.isUndefined() && !stateValue.isNull())
            return stateValue.toVariant();
    }

    // 返回默认值
    return defaultValue;
}

void ConfigCenter::setValue(const QString& key, const QVariant& value, ConfigType type)
{
    QJsonObject* targetObj = nullptr;
    QString targetKey = key;

    switch (type) {
    case ConfigType::SystemSettings:
        targetObj = &m_systemSettings;
        break;
    case ConfigType::UserSettings:
        targetObj = &m_userSettings;
        break;
    case ConfigType::StateData:
        if (ConfigKeys::isStateKey(key)) {
            targetKey = ConfigKeys::extractStateKey(key);
        }
        targetObj = &m_stateData;
        break;
    }

    if (targetObj) {
        QJsonValue jsonValue = QJsonValue::fromVariant(value);
        setNestedValue(*targetObj, targetKey, jsonValue);

        // 发出信号
        emit configChanged(key, type);

        // 发出特定信号
        if (key == ConfigKeys::WINDOW_SIZE) emit windowSizeChanged();
        else if (key == ConfigKeys::WINDOW_POSITION) emit windowPositionChanged();
        else if (key == ConfigKeys::WINDOW_IS_FULL_SCREEN) emit isFullScreenChanged();
        else if (key == ConfigKeys::WINDOW_TITLE) emit windowTitleChanged();
        else if (key == ConfigKeys::EDITOR_SHOW_LINE_NUMBERS) emit showLineNumbersChanged();
        else if (key == ConfigKeys::EDITOR_FONT_SIZE) emit fontSizeChanged();
        else if (key == ConfigKeys::EDITOR_FONT_FAMILY) emit fontFamilyChanged();
        else if (key == ConfigKeys::EDITOR_WORD_WRAP) emit wordWrapChanged();
        else if (key == ConfigKeys::EDITOR_TAB_SIZE) emit tabSizeChanged();
        else if (key == ConfigKeys::STATE_RECENT_FILES) emit recentFilesChanged();
        else if (key == ConfigKeys::STATE_CURRENT_FILE_PATH) emit currentFilePathChanged();
        else if (key == ConfigKeys::FILES_RESTORE_SESSION) emit restoreSessionChanged();

        // 如果是用户设置或状态数据，自动保存
        if (type == ConfigType::UserSettings || type == ConfigType::StateData) {
            saveConfig(type);
        }
    }
}

bool ConfigCenter::hasKey(const QString& key, ConfigType type)
{
    switch (type) {
    case ConfigType::SystemSettings:
        return !getNestedValue(m_systemSettings, key).isUndefined();
    case ConfigType::UserSettings:
        return !getNestedValue(m_userSettings, key).isUndefined();
    case ConfigType::StateData:
        if (ConfigKeys::isStateKey(key)) {
            QString stateKey = ConfigKeys::extractStateKey(key);
            return !getNestedValue(m_stateData, stateKey).isUndefined();
        }
        return !getNestedValue(m_stateData, key).isUndefined();
    }
    return false;
}

void ConfigCenter::removeKey(const QString& key, ConfigType type)
{
    // 这个方法的实现较为复杂，需要递归删除键
    // 简化版本可以只支持顶级键的删除
    QStringList parts = key.split('.');
    QString firstPart = parts.first();

    switch (type) {
    case ConfigType::SystemSettings:
        if (m_systemSettings.contains(firstPart))
            m_systemSettings.remove(firstPart);
        break;
    case ConfigType::UserSettings:
        if (m_userSettings.contains(firstPart))
            m_userSettings.remove(firstPart);
        break;
    case ConfigType::StateData:
        if (ConfigKeys::isStateKey(key)) {
            QString stateKey = ConfigKeys::extractStateKey(key);
            QStringList stateParts = stateKey.split('.');
            QString stateFirstPart = stateParts.first();
            if (m_stateData.contains(stateFirstPart))
                m_stateData.remove(stateFirstPart);
        }
        else if (m_stateData.contains(firstPart)) {
            m_stateData.remove(firstPart);
        }
        break;
    }

    saveConfig(type);
}

void ConfigCenter::resetToSystemDefaults()
{
    // 备份一些可能需要保留的状态
    QJsonObject stateBackup = m_stateData;

    // 清空用户设置
    m_userSettings = QJsonObject();
    saveConfig(ConfigType::UserSettings);

    // 恢复状态数据
    m_stateData = stateBackup;
    saveConfig(ConfigType::StateData);

    // 发出所有配置变更信号
    emit windowSizeChanged();
    emit windowPositionChanged();
    emit isFullScreenChanged();
    emit windowTitleChanged();
    emit showLineNumbersChanged();
    emit fontSizeChanged();
    emit fontFamilyChanged();
    emit wordWrapChanged();
    emit tabSizeChanged();
    emit restoreSessionChanged();
}

// 以下是各配置项的访问器实现

QSize ConfigCenter::windowSize() const
{
    QVariant value = getValue(ConfigKeys::WINDOW_SIZE);
    if (value.isValid() && value.canConvert<QVariantMap>()) {
        QVariantMap sizeMap = value.toMap();
        int width = sizeMap["width"].toInt();
        int height = sizeMap["height"].toInt();
        return QSize(width, height);
    }
    return QSize(1100, 600); // 默认值
}

void ConfigCenter::setWindowSize(const QSize& size)
{
    QVariantMap sizeMap;
    sizeMap["width"] = size.width();
    sizeMap["height"] = size.height();
    setValue(ConfigKeys::WINDOW_SIZE, sizeMap, ConfigType::UserSettings);
}

QPoint ConfigCenter::windowPosition() const
{
    QVariant value = getValue(ConfigKeys::WINDOW_POSITION);
    if (value.isValid() && value.canConvert<QVariantMap>()) {
        QVariantMap posMap = value.toMap();
        int x = posMap["x"].toInt();
        int y = posMap["y"].toInt();
        return QPoint(x, y);
    }
    return QPoint(100, 100); // 默认值
}

void ConfigCenter::setWindowPosition(const QPoint& pos)
{
    QVariantMap posMap;
    posMap["x"] = pos.x();
    posMap["y"] = pos.y();
    setValue(ConfigKeys::WINDOW_POSITION, posMap, ConfigType::UserSettings);
}

bool ConfigCenter::isFullScreen() const
{
    return getValue(ConfigKeys::WINDOW_IS_FULL_SCREEN, false).toBool();
}

void ConfigCenter::setIsFullScreen(bool fullscreen)
{
    setValue(ConfigKeys::WINDOW_IS_FULL_SCREEN, fullscreen, ConfigType::UserSettings);
}

QString ConfigCenter::windowTitle() const
{
    return getValue(ConfigKeys::WINDOW_TITLE, "${activeFileName} - EvaEdit").toString();
}

void ConfigCenter::setWindowTitle(const QString& title)
{
    setValue(ConfigKeys::WINDOW_TITLE, title, ConfigType::UserSettings);
}

bool ConfigCenter::showLineNumbers() const
{
    return getValue(ConfigKeys::EDITOR_SHOW_LINE_NUMBERS, true).toBool();
}

void ConfigCenter::setShowLineNumbers(bool show)
{
    setValue(ConfigKeys::EDITOR_SHOW_LINE_NUMBERS, show, ConfigType::UserSettings);
}

int ConfigCenter::fontSize() const
{
    return getValue(ConfigKeys::EDITOR_FONT_SIZE, 14).toInt();
}

void ConfigCenter::setFontSize(int size)
{
    setValue(ConfigKeys::EDITOR_FONT_SIZE, size, ConfigType::UserSettings);
}

QString ConfigCenter::fontFamily() const
{
    return getValue(ConfigKeys::EDITOR_FONT_FAMILY, "Consolas, 'Courier New', monospace").toString();
}

void ConfigCenter::setFontFamily(const QString& family)
{
    setValue(ConfigKeys::EDITOR_FONT_FAMILY, family, ConfigType::UserSettings);
}

bool ConfigCenter::wordWrap() const
{
    return getValue(ConfigKeys::EDITOR_WORD_WRAP, false).toBool();
}

void ConfigCenter::setWordWrap(bool wrap)
{
    setValue(ConfigKeys::EDITOR_WORD_WRAP, wrap, ConfigType::UserSettings);
}

int ConfigCenter::tabSize() const
{
    return getValue(ConfigKeys::EDITOR_TAB_SIZE, 4).toInt();
}

void ConfigCenter::setTabSize(int size)
{
    setValue(ConfigKeys::EDITOR_TAB_SIZE, size, ConfigType::UserSettings);
}

QStringList ConfigCenter::recentFiles() const
{
    return getValue(ConfigKeys::STATE_RECENT_FILES, QStringList()).toStringList();
}

void ConfigCenter::addRecentFile(const QString& filePath)
{
    if (filePath.isEmpty())
        return;

    QStringList files = recentFiles();

    // 如果已存在，先移除
    files.removeAll(filePath);

    // 添加到列表开头
    files.prepend(filePath);

    // 保持列表长度
    int maxFiles = getValue(ConfigKeys::FILES_MAX_RECENT_FILES, MAX_RECENT_FILES).toInt();
    while (files.size() > maxFiles)
        files.removeLast();

    setValue(ConfigKeys::STATE_RECENT_FILES, files, ConfigType::StateData);
}

void ConfigCenter::clearRecentFiles()
{
    setValue(ConfigKeys::STATE_RECENT_FILES, QStringList(), ConfigType::StateData);
}

QString ConfigCenter::currentFilePath() const
{
    return getValue(ConfigKeys::STATE_CURRENT_FILE_PATH, "").toString();
}

void ConfigCenter::setCurrentFilePath(const QString& path)
{
    setValue(ConfigKeys::STATE_CURRENT_FILE_PATH, path, ConfigType::StateData);

    if (!path.isEmpty()) {
        addRecentFile(path);
    }
}

bool ConfigCenter::restoreSession() const
{
    return getValue(ConfigKeys::FILES_RESTORE_SESSION, true).toBool();
}

void ConfigCenter::setRestoreSession(bool restore)
{
    setValue(ConfigKeys::FILES_RESTORE_SESSION, restore, ConfigType::UserSettings);
}

QVariantList ConfigCenter::getOpenFiles() const
{
    return getValue(ConfigKeys::STATE_OPEN_FILES, QVariantList()).toList();
}

void ConfigCenter::setOpenFiles(const QVariantList& files)
{
    setValue(ConfigKeys::STATE_OPEN_FILES, files, ConfigType::StateData);
}

int ConfigCenter::getActiveTabIndex() const
{
    return getValue(ConfigKeys::STATE_ACTIVE_TAB_INDEX, 0).toInt();
}

void ConfigCenter::setActiveTabIndex(int index)
{
    setValue(ConfigKeys::STATE_ACTIVE_TAB_INDEX, index, ConfigType::StateData);
}