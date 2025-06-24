add_subdirectory(model)

set(SRC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# 让上级目录可以访问这些源文件
set(SRC_SOURCES ${SRC_SOURCES} PARENT_SCOPE)