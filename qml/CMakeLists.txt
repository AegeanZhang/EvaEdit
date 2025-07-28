# 收集 QML 文件
set(QML_FILES_BASE
    Main.qml
    
    # 组件文件
    EMenu.qml
    EMenuBar.qml
    
    # 视图文件
    EEditor.qml
    EFileView.qml
    ESidebar.qml
    ETabView.qml
    
    # 工具类文件
    Colors.qml
    ResizeButton.qml
    WindowDragHandler.qml

    # 操作文件
    actions/MenuActions.js
)

# 如果已经进行了目录重组，可以改用下面的方式（取消注释）
# set(QML_FILES
#     Main.qml
#     qmldir
#     
#     # 通用样式
#     common/Colors.qml
#     
#     # 组件
#     components/EMenu.qml
#     components/EMenuBar.qml
#     
#     # 处理器
#     handlers/ResizeButton.qml
#     handlers/WindowDragHandler.qml
#     
#     # 视图
#     views/EEditor.qml
#     views/EFileView.qml
#     views/ESidebar.qml
#     views/ETabView.qml
# )

# 创建带前缀的文件列表
set(QML_FILES "")
foreach(QML_FILE ${QML_FILES_BASE})
    list(APPEND QML_FILES "qml/${QML_FILE}")
endforeach()

# 导出变量到父作用域
set(QML_FILES ${QML_FILES} PARENT_SCOPE)