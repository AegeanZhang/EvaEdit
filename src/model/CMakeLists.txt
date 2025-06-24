set(MODEL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/LineNumberModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LineNumberModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystemModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileSystemModel.cpp
)

# 让上级目录可以访问这些源文件
set(MODEL_SOURCES ${MODEL_SOURCES} PARENT_SCOPE)