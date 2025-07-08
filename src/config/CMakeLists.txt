set(CONFIG_FILES_BASE
    ConfigKeys.h
    ConfigKeys.cpp
    ConfigPaths.h
    ConfigPaths.cpp
    ConfigCenter.h
    ConfigCenter.cpp
)

# 创建带前缀的文件列表Relative
set(RELATIVE_CONFIG_SOURCES "")
set(CONFIG_SOURCES "")
foreach(SOURCES_FILE ${CONFIG_FILES_BASE})
    list(APPEND RELATIVE_CONFIG_SOURCES "src/config/${SOURCES_FILE}")
    list(APPEND CONFIG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES_FILE}")
endforeach()

# 导出变量到父作用域
set(RELATIVE_CONFIG_SOURCES ${RELATIVE_CONFIG_SOURCES} PARENT_SCOPE)
set(CONFIG_SOURCES ${CONFIG_SOURCES} PARENT_SCOPE)


