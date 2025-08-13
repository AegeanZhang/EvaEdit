import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

import Logger

import EvaEdit

MenuBar {
    id: root

    required property ApplicationWindow dragWindow
    property alias infoText: windowInfo.text

    delegate: MenuBarItem {
        id: menuBarItem

        contentItem: Text {
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            text: menuBarItem.text
            font: menuBarItem.font
            elide: Text.ElideRight
            color: menuBarItem.highlighted ? Colors.textFile : Colors.text
            opacity: enabled ? 1.0 : 0.3
        }

        background: Rectangle {
            id: background

            color: menuBarItem.highlighted ? Colors.selection : "transparent"
            Rectangle {
                id: indicator

                width: 0; height: 3
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom

                color: Colors.color1
                states: State {
                    name: "active"
                    when: menuBarItem.highlighted
                    PropertyChanges {
                        indicator.width: background.width - 2
                    }
                }
                transitions: Transition {
                    NumberAnimation {
                        properties: "width"
                        duration: 175
                    }
                }
            }
        }
    }
    // We use the contentItem property as a place to attach our window decorations. Beneath
    // the usual menu entries within a MenuBar, it includes a centered information text, along
    // with the minimize, maximize, and close buttons.
    contentItem: RowLayout {
        id: windowBar

        Layout.fillWidth: true
        Layout.fillHeight: true

        spacing: root.spacing
        Repeater {
            id: menuBarItems

            Layout.alignment: Qt.AlignLeft
            model: root.contentModel
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Text {
                id: windowInfo

                width: parent.width; height: parent.height
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                leftPadding: windowActions.width
                color: Colors.text
                clip: true
            }
        }

        RowLayout {
            id: windowActions

            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true

            spacing: 0

            component InteractionButton: Rectangle {
                id: interactionButton

                signal action()
                property alias hovered: hoverHandler.hovered

                Layout.fillHeight: true
                Layout.preferredWidth: height

                color: hovered ? Colors.background : "transparent"
                HoverHandler {
                    id: hoverHandler
                }
                TapHandler {
                    id: tapHandler
                    onTapped: interactionButton.action()
                }
            }

            InteractionButton {
                id: minimize

                onAction: root.dragWindow.showMinimized()
                Rectangle {
                    anchors.centerIn: parent
                    color: parent.hovered ? Colors.iconIndicator : Colors.icon
                    height: 2
                    width: parent.height - 14
                }
            }

            InteractionButton {
                id: maximize

                onAction: root.dragWindow.showMaximized()
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 7
                    border.color: parent.hovered ? Colors.iconIndicator : Colors.icon
                    border.width: 2
                    color: "transparent"
                }
            }

            InteractionButton {
                id: close

                color: hovered ? "#ec4143" : "transparent"
                onAction: root.dragWindow.close()
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.height - 8; height: 2

                    rotation: 45
                    antialiasing: true
                    transformOrigin: Item.Center
                    color: parent.hovered ? Colors.iconIndicator : Colors.icon

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.height
                        height: parent.width

                        antialiasing: true
                        color: parent.color
                    }
                }
            }
        }
    }

    background: Rectangle {
        color: Colors.surface2
        // Make the empty space drag the specified root window.
        WindowDragHandler {
            dragWindow: root.dragWindow
        }
    }

    EMenu {
        title: qsTr("文件(<u>F</u>)")
        Action { 
            //text: qsTr("新建(&N)")
            text: AppConstants.newFile
            shortcut: "Ctrl+N"
            onTriggered: TabController.addNewTab();
        }
        Action { 
            text: qsTr("打开")
            shortcut: "Ctrl+O"
            onTriggered: dialogs.openFileDialog()
        }
        Action {
            text: qsTr("保存")
            shortcut: "Ctrl+S"
            onTriggered: ()=> {
                if(!FileController.saveFile()) {
                    dialogs.openSaveAsDialog()
                }                    
            }
        }
        Action {
            text: qsTr("另存为")
            shortcut: "Ctrl+Shift+S"
            onTriggered: dialogs.openSaveAsDialog()
            }
        MenuSeparator {}
        Action { 
            text: qsTr("打开目录") 
            //onTriggered: folderDialog.open()
            onTriggered: dialogs.openFolderDialog() 
        }
        MenuSeparator {}
        Action { text: qsTr("退出"); onTriggered: Qt.quit() }
    }
    EMenu {
        title: qsTr("编辑")
        Action { text: qsTr("撤销") }
        Action { text: qsTr("重做") }
    }
    EMenu {
        title: qsTr("外观")
        // 二级菜单
        EMenu {
            title: qsTr("主题")
            Action { 
                text: qsTr("Dark")
                onTriggered: Colors.setTheme(DarkTheme) 
            }
            Action { 
                text: qsTr("Light")
                onTriggered: Colors.setTheme(LightTheme) 
            }
        }
    }

    // 对话框组件
    EDialogs {
        id: dialogs
        
        onFolderSelected: function(folderPath) {
            Logger.info("文件夹已选择: " + folderPath)
            FileSystemModel.setDirectory(folderPath)
            // 切换到文件浏览标签页
            sidebar.currentTabIndex = 0
        }
        
        onFileSelected: function(filePath) {
            Logger.info("文件已选择: " + filePath)
            // 使用 TabController 打开文件
            TabController.addNewTab(filePath)
        }
        
        onSaveAsSelected: function(filePath) {
            Logger.info("另存为路径已选择: " + filePath)
            // 处理另存为逻辑
            FileController.saveAsFile(filePath)
        }
    }
}