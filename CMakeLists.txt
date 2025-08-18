cmake_minimum_required(VERSION 3.16)

project(EvaEdit VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(src)
add_subdirectory(qml)

set(PROJECT_SOURCES
    ${SRC_SOURCES}
)

qt_add_executable(EvaEdit
    ${PROJECT_SOURCES}
)

target_include_directories(EvaEdit 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controller
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/interaction
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/controller
    ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/render
)

qt_add_qml_module(EvaEdit
    URI EvaEdit
    VERSION 1.0
    QML_FILES
        ${QML_FILES}
        qml/qmldir
    RESOURCES
        # SideBar部分的icon
        resources/icons/light_bulb.svg
        resources/icons/read.svg
        resources/icons/globe.svg
        resources/icons/info_sign.svg
        resources/icons/outline.svg
        # FileView部分的icons
        resources/icons/folder_closed.svg
        resources/icons/folder_open.svg
        resources/icons/generic_file.svg
        resources/icons/resize.svg
        # TabView 部分的icons
        resources/icons/close_btn.svg
        resources/icons/add_btn.svg
        # 菜单部分的icons
        resources/icons/check.svg
    SOURCES
        ${RELATIVE_MODEL_SOURCES}
        ${RELATIVE_CONTROLLER_SOURCES}
        ${RELATIVE_CONFIG_SOURCES}
        ${RELATIVE_LOGGER_SOURCES}
        ${RELATIVE_COMMON_SOURCES}
        ${RELATIVE_EDITOR_SOURCES}
)

set_target_properties(EvaEdit PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(EvaEdit
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS EvaEdit
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
