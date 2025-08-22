# 收集 QML 文件
set(QML_FILES_BASE
    common/EConstants.qml

    common/Colors.qml
    themes/DarkTheme.qml
    themes/LightTheme.qml
    #themes/ThemeManager.qml
    
    # 组件文件
    components/EMenu.qml
    components/EMenuBar.qml
    components/EDialogs.qml
    components/ResizeButton.qml
    
    # 视图文件
    views/EEditor.qml
    views/ETextEditor.qml
    views/EFileView.qml
    views/ESidebar.qml
    views/ESidePanel.qml
    views/ETabView.qml
    views/EOutlineView.qml
    
    # 工具类文件
    handlers/WindowDragHandler.qml

    # 操作文件
    actions/MenuActions.js

    Main.qml
)

# 创建带前缀的文件列表
set(QML_FILES "")
foreach(QML_FILE ${QML_FILES_BASE})
    list(APPEND QML_FILES "qml/${QML_FILE}")
endforeach()

# 导出变量到父作用域
set(QML_FILES ${QML_FILES} PARENT_SCOPE)