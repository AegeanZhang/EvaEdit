# 收集 QML 文件
set(QML_FILES_BASE
    Main.qml

    common/Colors.qml
    
    # 组件文件
    components/EMenu.qml
    components/EMenuBar.qml
    components/EDialogs.qml
    
    # 视图文件
    views/EEditor.qml
    views/EFileView.qml
    views/ESidebar.qml
    views/ETabView.qml
    
    # 工具类文件
    handlers/ResizeButton.qml
    handlers/WindowDragHandler.qml

    # 操作文件
    actions/MenuActions.js
)

# 创建带前缀的文件列表
set(QML_FILES "")
foreach(QML_FILE ${QML_FILES_BASE})
    list(APPEND QML_FILES "qml/${QML_FILE}")
endforeach()

# 导出变量到父作用域
set(QML_FILES ${QML_FILES} PARENT_SCOPE)