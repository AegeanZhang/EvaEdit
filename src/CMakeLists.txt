add_subdirectory(model)
add_subdirectory(config)
add_subdirectory(logger)

set(SRC_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# 让上级目录可以访问这些源文件
set(SRC_SOURCES ${SRC_SOURCES} PARENT_SCOPE)
set(RELATIVE_MODEL_SOURCES ${RELATIVE_MODEL_SOURCES} PARENT_SCOPE)
set(RELATIVE_CONFIG_SOURCES ${RELATIVE_CONFIG_SOURCES} PARENT_SCOPE)
set(RELATIVE_LOGGER_SOURCES ${RELATIVE_LOGGER_SOURCES} PARENT_SCOPE)