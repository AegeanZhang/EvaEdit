import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.platform as Platform

import Logger
import "actions/MenuActions.js" as MenuActions

import EvaEdit

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: root

    property bool expandPath: false
    property bool showLineNumbers: true
    property string currentFilePath: ""
    property bool sidePanelVisible: true

    function getInfoText() : string {
        /* let out = root.currentFilePath
        if (!out)
            return qsTr("EvaEdit")
        return root.expandPath ? out : out.substring(out.lastIndexOf("/") + 1, out.length)
        */
        return qsTr("EvaEdit - 版本 1.0")
    }

    function toggleSidePanel() {
        root.sidePanelVisible = !root.sidePanelVisible
    }

    // 使用可用桌面空间而不是整个屏幕
    width: Screen.desktopAvailableWidth
    height: Screen.desktopAvailableHeight

    // 设置窗口位置
    x: Screen.desktopAvailableWidth / 2 - root.width / 2
    y: Screen.desktopAvailableHeight / 2 - root.height / 2

    //width: 1100
    //height: 600
    minimumWidth: 200
    minimumHeight: 100
    visible: true
    color: Colors.background
    flags: Qt.Window | Qt.FramelessWindowHint
    title: qsTr("EvaEdit")

    menuBar: EMenuBar {
        dragWindow: root
        infoText: root.getInfoText()
        onShowAboutDialog: aboutDialog.open()
    }

    Component.onCompleted: {
        Logger.info("窗口加载完成")
    }

    RowLayout {
        //anchors.fill: parent
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        spacing: 0

        // Stores the buttons that navigate the application.
        ESidebar {
            id: sidebar
            dragWindow: root
            Layout.preferredWidth: 50
            Layout.fillHeight: true

            onToggleSidePanel: root.toggleSidePanel()
        }

        // Allows resizing parts of the UI.
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Customized handle to drag between the Navigation and the Editor.
            handle: Rectangle {
                implicitWidth: 10
                color: SplitHandle.pressed ? Colors.color2 : Colors.background
                border.color: SplitHandle.hovered ? Colors.color2 : Colors.background
                opacity: SplitHandle.hovered || navigationView.width < 15 ? 1.0 : 0.0

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 1400
                    }
                }
            }

            ESidePanel {
                id: navigationView
                //currentFilePath: root.currentFilePath
                visible: root.sidePanelVisible
                SplitView.preferredWidth: root.sidePanelVisible ? 250 : 0
            }

            ETabView {
                id: tabView

                showLineNumbers: root.showLineNumbers
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                
                onFilePathChanged: function(filePath) {
                    root.currentFilePath = filePath;
                }
            }
        }
    }

    // 状态栏
    Rectangle {
        id: statusBar

        height: 24
        color: Colors.background
        //color: "#FFFFFF"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            color: Colors.text
            text: qsTr("就绪")
        }
        ResizeButton {
            resizeWindow: root
        }
    }

    EAboutDialog {
        id: aboutDialog
    }
}
