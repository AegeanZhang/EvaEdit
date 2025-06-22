import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    menuBar: MenuBar {
        Menu {
            title: qsTr("文件")
            Action { text: qsTr("新建") }
            Action { text: qsTr("打开") }
            Action { text: qsTr("保存") }
            MenuSeparator {}
            Action { text: qsTr("退出"); onTriggered: Qt.quit() }
        }
        Menu {
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

    // 主内容区
    Rectangle {
        id: mainContent
        color: "#f0f0f0"
        anchors {
            top: toolBar.bottom
            left: parent.left
            right: parent.right
            bottom: statusBar.top
        }
    }

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
