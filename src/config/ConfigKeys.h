#ifndef __CONFIG_KEYS_H__
#define __CONFIG_KEYS_H__

#include <QString>

// 配置键常量类，用于集中管理所有配置项的键名
class ConfigKeys
{
public:
    // 窗口相关配置
    static const QString WINDOW_SIZE;
    static const QString WINDOW_POSITION;
    static const QString WINDOW_IS_FULL_SCREEN;
    static const QString WINDOW_TITLE;
    
    // 编辑器相关配置
    static const QString EDITOR_SHOW_LINE_NUMBERS;
    static const QString EDITOR_FONT_SIZE;
    static const QString EDITOR_FONT_FAMILY;
    static const QString EDITOR_WORD_WRAP;
    static const QString EDITOR_TAB_SIZE;
    
    // 文件相关配置
    static const QString FILES_RESTORE_SESSION;
    static const QString FILES_MAX_RECENT_FILES;
    
    // 状态相关配置
    static const QString STATE_PREFIX;
    static const QString STATE_RECENT_FILES;
    static const QString STATE_CURRENT_FILE_PATH;
    static const QString STATE_OPEN_FILES;
    static const QString STATE_ACTIVE_TAB_INDEX;

    // 从完整路径中提取状态键
    static QString extractStateKey(const QString& fullKey);
    
    // 检查是否为状态键
    static bool isStateKey(const QString& key);
};

#endif // __CONFIG_KEYS_H__