import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.platform as Platform

import Logger
import "actions/MenuActions.js" as MenuActions

//import EvaEdit

pragma ComponentBehavior: Bound

ApplicationWindow {
    id: root

    property bool expandPath: false
    property bool showLineNumbers: true
    property string currentFilePath: ""

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

    Component.onCompleted: {
        Logger.info("窗口加载完成")
    }

    // 添加文件夹选择对话框
    Platform.FolderDialog {
        id: folderDialog
        title: qsTr("选择文件夹")
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        
        onAccepted: {
            // 将选中的文件夹设置为文件浏览器的根目录
            const folderPath = folderDialog.folder.toString();
            // 将 file:/// 前缀移除 (如果需要)
            const cleanPath = folderPath.replace(/^(file:\/{2,3})/, "");
            FileSystemModel.setDirectory(cleanPath);
            
            // 切换到文件浏览标签页
            //sidebar.currentTabIndex = 1;
            sidebar.currentTabIndex = 0;
        }
    }

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
            title: qsTr("文件(<u>F</u>)")
            Action { 
                text: qsTr("新建(&N)")
                shortcut: "Ctrl+N"
                onTriggered: MenuActions.newFile(tabView);
            }
            Action { text: qsTr("打开") }
            Action { text: qsTr("保存") }
            Action { text: qsTr("另存为") }
            MenuSeparator {}
            Action { 
                text: qsTr("打开目录") 
                onTriggered: folderDialog.open()
            }
            MenuSeparator {}
            Action { text: qsTr("退出"); onTriggered: Qt.quit() }
        }
        EMenu {
            title: qsTr("编辑")
            Action { text: qsTr("撤销") }
            Action { text: qsTr("重做") }
        }
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

                    // Shows the files on the file system.
                    EFileView {
                        id: fileSystemView
                        color: Colors.surface1
                        onFileClicked: path => {
                            root.currentFilePath = path;
                            tabView.addNewTab(path);
                        }
                    }

                    // Shows the help text.
                    Text {
                        text: qsTr("This example shows how to use and visualize the file system.\n\n"
                                 + "Customized Qt Quick Components have been used to achieve this look.\n\n"
                                 + "You can edit the files but they won't be changed on the file system.\n\n"
                                 + "Click on the folder icon to the left to get started.")
                        wrapMode: TextArea.Wrap
                        color: Colors.text
                    }
                }
            }

            // The main view that contains the editor.
            /*EEditor {
                id: editor
                color: Colors.surface1
                showLineNumbers: root.showLineNumbers
                currentFilePath: root.currentFilePath
                SplitView.fillWidth: true
                SplitView.fillHeight: true
            }*/
            // 替换单个编辑器为标签式编辑器
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



}
