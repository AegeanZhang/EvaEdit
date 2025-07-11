#ifndef __CONFIG_PATHS_H__
#define __CONFIG_PATHS_H__

#include <QString>

// 配置类型枚举
enum class ConfigType {
    SystemSettings,  // 系统级配置
    UserSettings,    // 用户级配置
    StateData        // 状态数据
};

inline QString configTypeToString(ConfigType type) {
    switch (type) {
    case ConfigType::SystemSettings: return "SystemSettings";
    case ConfigType::UserSettings: return "UserSettings";
    case ConfigType::StateData: return "StateData";
    default: return "Unknown";
    }
}

// 配置文件路径管理类
class ConfigPaths
{
public:
    // 获取配置文件名
    static QString getConfigFileName(ConfigType type);
    
    // 获取配置文件路径 - 自动检测开发/生产环境
    static QString getConfigFilePath(ConfigType type);
    
    // 获取开发环境下的配置文件路径
    static QString getDevConfigFilePath(ConfigType type);
    
    // 获取生产环境下的配置文件路径
    static QString getProdConfigFilePath(ConfigType type);
    
    // 判断当前是否为开发环境
    static bool isDevelopmentEnvironment();
    
    // 确保配置目录存在
    static void ensureConfigDirExists(const QString& dirPath);
    
private:
    // 配置文件名常量
    static const QString SYSTEM_SETTINGS_FILENAME;
    static const QString USER_SETTINGS_FILENAME;
    static const QString STATE_DATA_FILENAME;
    
    // 开发环境标记文件名
    static const QString DEV_ENV_MARKER_FILE;
};

#endif // __CONFIG_PATHS_H__