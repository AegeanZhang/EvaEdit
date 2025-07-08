#set(MODEL_SOURCES
#    ${CMAKE_CURRENT_SOURCE_DIR}/LineNumberModel.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/LineNumberModel.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystemModel.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystemModel.cpp
#)

# 让上级目录可以访问这些源文件
#set(MODEL_SOURCES ${MODEL_SOURCES} PARENT_SCOPE)

set(MODEL_FILES_BASE
    LineNumberModel.h
    LineNumberModel.cpp
    FileSystemModel.h
    FileSystemModel.cpp
)

# 创建带前缀的文件列表Relative
set(RELATIVE_MODEL_SOURCES "")
set(MODEL_SOURCES "")
foreach(SOURCES_FILE ${MODEL_FILES_BASE})
    list(APPEND RELATIVE_MODEL_SOURCES "src/model/${SOURCES_FILE}")
    list(APPEND MODEL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCES_FILE}")
endforeach()

# 导出变量到父作用域
set(RELATIVE_MODEL_SOURCES ${RELATIVE_MODEL_SOURCES} PARENT_SCOPE)
set(MODEL_SOURCES ${MODEL_SOURCES} PARENT_SCOPE)