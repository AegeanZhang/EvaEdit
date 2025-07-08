#include "ConfigKeys.h"

// 窗口相关配置
const QString ConfigKeys::WINDOW_SIZE = "eva.window.size";
const QString ConfigKeys::WINDOW_POSITION = "eva.window.position";
const QString ConfigKeys::WINDOW_IS_FULL_SCREEN = "eva.window.isFullScreen";
const QString ConfigKeys::WINDOW_TITLE = "eva.window.title";

// 编辑器相关配置
const QString ConfigKeys::EDITOR_SHOW_LINE_NUMBERS = "eva.editor.showLineNumbers";
const QString ConfigKeys::EDITOR_FONT_SIZE = "eva.editor.fontSize";
const QString ConfigKeys::EDITOR_FONT_FAMILY = "eva.editor.fontFamily";
const QString ConfigKeys::EDITOR_WORD_WRAP = "eva.editor.wordWrap";
const QString ConfigKeys::EDITOR_TAB_SIZE = "eva.editor.tabSize";

// 文件相关配置
const QString ConfigKeys::FILES_RESTORE_SESSION = "eva.files.restoreSession";
const QString ConfigKeys::FILES_MAX_RECENT_FILES = "eva.files.maxRecentFiles";

// 状态相关配置
const QString ConfigKeys::STATE_PREFIX = "eva.state.";
const QString ConfigKeys::STATE_RECENT_FILES = "eva.state.recentFiles";
const QString ConfigKeys::STATE_CURRENT_FILE_PATH = "eva.state.currentFilePath";
const QString ConfigKeys::STATE_OPEN_FILES = "eva.state.openFiles";
const QString ConfigKeys::STATE_ACTIVE_TAB_INDEX = "eva.state.activeTabIndex";

QString ConfigKeys::extractStateKey(const QString& fullKey)
{
    if (isStateKey(fullKey)) {
        return fullKey.mid(STATE_PREFIX.length());
    }
    return fullKey;
}

bool ConfigKeys::isStateKey(const QString& key)
{
    return key.startsWith(STATE_PREFIX);
}