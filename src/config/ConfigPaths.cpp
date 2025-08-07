#include "ConfigPaths.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

#include "../logger/Logger.h"

// 配置文件名常量
const QString ConfigPaths::SYSTEM_SETTINGS_FILENAME = "settings.json";
const QString ConfigPaths::USER_SETTINGS_FILENAME = "user_settings.json";
const QString ConfigPaths::STATE_DATA_FILENAME = "state.json";

// 开发环境标记文件
const QString ConfigPaths::DEV_ENV_MARKER_FILE = ".dev_environment";

QString ConfigPaths::getConfigFileName(ConfigType type)
{
    switch (type) {
    case ConfigType::SystemSettings:
        return SYSTEM_SETTINGS_FILENAME;
    case ConfigType::UserSettings:
        return USER_SETTINGS_FILENAME;
    case ConfigType::StateData:
        return STATE_DATA_FILENAME;
    default:
        return "";
    }
}

QString ConfigPaths::getConfigFilePath(ConfigType type)
{
    if (isDevelopmentEnvironment()) {
        return getDevConfigFilePath(type);
    } else {
        return getProdConfigFilePath(type);
    }
}

QString ConfigPaths::getDevConfigFilePath(ConfigType type)
{
    // 开发环境下，所有配置文件放在程序目录
    QString configDir = QCoreApplication::applicationDirPath();
    QString fileName = getConfigFileName(type);
    
    ensureConfigDirExists(configDir);
    
    return configDir + "/" + fileName;
}

QString ConfigPaths::getProdConfigFilePath(ConfigType type)
{
    QString configDir;
    QString fileName = getConfigFileName(type);
    
    //switch (type) {
    //case ConfigType::SystemSettings:
    //    // 系统设置存放在 AppData/EvaEdit 目录
    //    configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    //    break;
    //    
    //case ConfigType::UserSettings:
    //case ConfigType::StateData:
    //    // 用户设置和状态数据存放在用户配置目录
    //    configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    //    break;
    //}

    // 开发阶段，使用环境变量EVAEDIT_CONFIG_PATH来读取配置文件
    configDir = qgetenv("EVAEDIT_CONFIG_PATH");
    configDir = QDir(configDir).canonicalPath();
    
    ensureConfigDirExists(configDir);

    return configDir + "/" + fileName;
}

bool ConfigPaths::isDevelopmentEnvironment()
{
    // 检查开发环境标记文件是否存在
    QString markerFilePath = QCoreApplication::applicationDirPath() + "/" + DEV_ENV_MARKER_FILE;
    return QFile::exists(markerFilePath);
    
    // 其他判断方法：
    // 1. 使用编译标记: 
    // #ifdef _DEBUG
    //     return true;
    // #else
    //     return false;
    // #endif
    
    // 2. 检查是否存在源码目录结构
    // QString srcPath = QCoreApplication::applicationDirPath() + "/../src";
    // return QDir(srcPath).exists();
}

void ConfigPaths::ensureConfigDirExists(const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!dir.mkpath(dirPath)) {
            qWarning() << "无法创建配置目录:" << dirPath;
        }
    }
}