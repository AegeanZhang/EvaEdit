import QtQuick

import EvaEdit

pragma Singleton

QtObject {
    // 菜单 - 文件
    property string menuFile: qsTr("File")
    property string menuFileNew: qsTr("New")
    property string menuFileOpen: qsTr("Open...")
    property string menuFileSave: qsTr("Save")
    property string menuFileSaveAs: qsTr("Save As...")
    property string menuFileOpenFolder: qsTr("Open Folder...")
    property string menuFileOpenResent: qsTr("Open Resent")
    property string menuFileExit: qsTr("Exit")

    // 菜单 - 编辑
    property string menuEdit: qsTr("Edit")
    property string menuEditUndo: qsTr("Undo")
    property string menuEditRedo: qsTr("Redo")
    property string menuEditCut: qsTr("Cut")
    property string menuEditCopy: qsTr("Copy")
    property string menuEditPaste: qsTr("Paste")
    property string menuEditFind: qsTr("Find")
    property string menuEditReplace: qsTr("Replace")

    // 菜单 - 视图
    property string menuView: qsTr("View")
    property string menuViewTheme: qsTr("Theme")
    property string menuViewThemeDark: qsTr("Dark Theme")
    property string menuViewThemeLight: qsTr("Light Theme")
    property string menuViewShowLineNumbers: qsTr("Show Line Numbers")
    property string menuViewTogglePreview: qsTr("Toggle Preview")
    property string menuViewSplitView: qsTr("Split View")

    // 菜单 - 帮助
    property string menuHelp: qsTr("Help")
    property string menuHelpCheckForUpdate: qsTr("Check For Update")
    property string menuHelpAbout: qsTr("About")

    // UI 常量
    readonly property real borderWidth: 0.5
}