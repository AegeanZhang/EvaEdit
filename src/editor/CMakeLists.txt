set(EDITOR_FILES_BASE
    core/UndoSystem.h
    core/UndoSystem.cpp
    core/DocumentModel.h
    core/DocumentModel.cpp
    core/TextStorage.h
    core/TextStorage.cpp
    service/SyntaxHighlighter.h
    service/SyntaxHighlighter.cpp
    service/SearchService.h
    service/SearchService.cpp
    service/LayoutEngine.h
    service/LayoutEngine.cpp
    interaction/InputManager.h
    interaction/InputManager.cpp
    interaction/CursorManager.h
    interaction/CursorManager.cpp
    interaction/SelectionManager.h
    interaction/SelectionManager.cpp
    render/MiniMapRenderer.h
    render/MiniMapRenderer.cpp
    render/TextRenderer.h
    render/TextRenderer.cpp
)

# 创建带前缀的文件列表Relative
set(RELATIVE_EDITOR_SOURCES "")
set(EDITOR_SOURCES "")
foreach(SOURCES_FILE ${EDITOR_FILES_BASE})
    list(APPEND RELATIVE_EDITOR_SOURCES "src/editor/${SOURCES_FILE}")
    list(APPEND EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES_FILE}")
endforeach()

# 导出变量到父作用域
set(RELATIVE_EDITOR_SOURCES ${RELATIVE_EDITOR_SOURCES} PARENT_SCOPE)
set(EDITOR_SOURCES ${EDITOR_SOURCES} PARENT_SCOPE)


