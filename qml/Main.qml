import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import EvaEdit

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: root

    property bool expandPath: false
    property bool showLineNumbers: true
    property string currentFilePath: ""

    width: 1100
    height: 600
    minimumWidth: 200
    minimumHeight: 100
    visible: true
    color: Colors.background
    flags: Qt.Window | Qt.FramelessWindowHint
    title: qsTr("EvaEdit")

    function getInfoText() : string {
        /* let out = root.currentFilePath
        if (!out)
            return qsTr("EvaEdit")
        return root.expandPath ? out : out.substring(out.lastIndexOf("/") + 1, out.length)
        */
        return qsTr("EvaEdit - 版本 1.0")
    }

    menuBar: EMenuBar {
        dragWindow: root
        infoText: root.getInfoText()
        EMenu {
            title: qsTr("文件")
            Action { text: qsTr("新建") }
            Action { text: qsTr("打开") }
            Action { text: qsTr("保存") }
            MenuSeparator {}
            Action { text: qsTr("退出"); onTriggered: Qt.quit() }
        }
        EMenu {
            title: qsTr("编辑")
            Action { text: qsTr("撤销") }
            Action { text: qsTr("重做") }
        }
    }

    ToolBar {
        id: toolBar
        RowLayout {
            anchors.fill: parent
            ToolButton { text: qsTr("新建") }
            ToolButton { text: qsTr("打开") }
            ToolButton { text: qsTr("保存") }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Stores the buttons that navigate the application.
        ESidebar {
            id: sidebar
            dragWindow: root
            Layout.preferredWidth: 50
            Layout.fillHeight: true
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

            Rectangle {
                id: navigationView
                color: Colors.surface1
                SplitView.preferredWidth: 250
                SplitView.fillHeight: true
                // The stack-layout provides different views, based on the
                // selected buttons inside the sidebar.
                StackLayout {
                    anchors.fill: parent
                    currentIndex: sidebar.currentTabIndex

                    // Shows the help text.
                    Text {
                        text: qsTr("This example shows how to use and visualize the file system.\n\n"
                                 + "Customized Qt Quick Components have been used to achieve this look.\n\n"
                                 + "You can edit the files but they won't be changed on the file system.\n\n"
                                 + "Click on the folder icon to the left to get started.")
                        wrapMode: TextArea.Wrap
                        color: Colors.text
                    }

                    // Shows the files on the file system.
                    EFileView {
                        id: fileSystemView
                        color: Colors.surface1
                        onFileClicked: path => root.currentFilePath = path
                    }
                }
            }

            // The main view that contains the editor.
            EEditor {
                id: editor
                showLineNumbers: root.showLineNumbers
                currentFilePath: root.currentFilePath
                SplitView.fillWidth: true
                SplitView.fillHeight: true
            }
        }
    }

    // 主内容区
    /* Rectangle {
        id: mainContent
        color: "#f0f0f0"
        anchors {
            top: toolBar.bottom
            left: parent.left
            right: parent.right
            bottom: statusBar.top
        }
    } */

    // 状态栏
    Rectangle {
        id: statusBar
        height: 24
        color: "#e0e0e0"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            text: qsTr("就绪")
        }
    }
}
