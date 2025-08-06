import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml.Models
import QtQuick.Effects

import Logger

import EvaEdit

pragma ComponentBehavior: Bound

Rectangle {
    id: root

    readonly property int tabBarHeight: 36
    readonly property int tabButtonHeight: 32

    // 公共属性
    property bool showLineNumbers: true

    // 信号
    signal filePathChanged(string filePath)

    // 🔥 纯UI函数（不包含业务逻辑）
    function setFocusToTab(tabIndex) {
        Qt.callLater(function() {
            if (tabIndex >= 0 && tabIndex < editorRepeater.count) {
                var editor = editorRepeater.itemAt(tabIndex);
                if (editor && editor.setFocus) {
                    editor.setFocus();
                }
            }
        });
    }

    color: Colors.surface1
  
    Connections {
        target: TabController

        function onFilePathUpdated(index, oldPath, newPath) {
            // 1. 更新数据模型
            tabModel.set(index, {
                "filePath": newPath, 
                "fileName": TabController.getTabFileName(index)
            });
        }

        function onCurrentFilePathChanged() {
            root.filePathChanged(TabController.currentFilePath);
        }
        
        function onFocusRequested(tabIndex) {
            setFocusToTab(tabIndex);
        }

        function onTabAdded(index, filePath) {
            tabModel.append({
                "filePath": filePath,
                "fileName": TabController.getTabFileName(index)
            });
        }

        function onTabClosed(index) {
            tabModel.remove(index);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标签栏
        TabBar {
            id: tabBar

            Layout.fillWidth: true
            Layout.preferredHeight: root.tabBarHeight

            // 🔥 绑定到控制器
            currentIndex: TabController.currentTabIndex
            onCurrentIndexChanged: TabController.currentTabIndex = currentIndex
            
            background: Rectangle {
                color: Colors.surface2

                //border.width: 2
                //border.color: "red"
            }
            
            // 标签模型
            ListModel {
                id: tabModel
            }
            
            Component.onCompleted: {
                // 如果控制器中没有标签，添加一个空白标签
                if (TabController.tabCount === 0) {
                    TabController.addNewTab();
                } else {
                    // 同步现有标签
                    for (let i = 0; i < TabController.tabCount; i++) {
                        tabModel.append({
                            "filePath": TabController.getTabFilePath(i),
                            "fileName": TabController.getTabFileName(i)
                        });
                    }
                }
            }
            
            Repeater {
                id: tabRepeater
                model: tabModel
                
                TabButton {
                    id: tabButton

                    // 这里使用required属性来明确接收model
                    required property var model

                    property int tabIndex: model.index
                    property string tabTitle: model.fileName
                    
                    width: Math.max(implicitWidth, 120)
                    height: root.tabButtonHeight

                    // 添加垂直居中锚点
                    anchors.verticalCenter: parent.verticalCenter
                    
                    contentItem: RowLayout {
                        Text {
                            Layout.fillWidth: true
                            text: tabButton.tabTitle
                            elide: Text.ElideRight
                            color: tabBar.currentIndex === tabButton.tabIndex ? Colors.textFile : Colors.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        // 关闭按钮
                        Rectangle {
                            width: 16
                            height: 16
                            radius: 8
                            color: closeMouseArea.containsMouse ? Colors.color2 : "transparent"
                            
                            Image {
                                id: closeIcon
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                source: "../../resources/icons/close_btn.svg"
                                sourceSize: Qt.size(12, 12)
                                visible: true 
                            }

                            MouseArea {
                                id: closeMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: TabController.closeTab(tabButton.tabIndex)
                            }
                        }
                    }
                    
                    background: Rectangle {
                        anchors.fill: parent  // 确保背景填满按钮区域
                        color: tabBar.currentIndex === tabButton.tabIndex ? Colors.surface1 : Colors.surface2

                        //border.width: 2
                        //border.color: "green"

                        Rectangle {
                            width: parent.width
                            height: 2
                            anchors.bottom: parent.bottom
                            color: tabBar.currentIndex === tabButton.tabIndex ? Colors.color1 : "transparent"
                        }
                    }
                    
                    // 🔥 简化点击处理
                    onClicked: TabController.currentTabIndex = tabButton.tabIndex
                }
            }
            
            // 添加按钮
            TabButton {
                id: addTabButton

                width: 30
                height: root.tabButtonHeight

                anchors.verticalCenter: parent.verticalCenter

                onClicked: TabController.addNewTab()

                contentItem: Rectangle {
                    color: "transparent"
                    anchors.fill: parent

                    //border.width: 2
                    //border.color: "yellow"
        
                    Image {
                        id: addIcon
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        source: "../../resources/icons/add_btn.svg"
                        sourceSize: Qt.size(14, 14)
                        visible: true
                    }
                }
                
                background: Rectangle {
                    anchors.fill: parent
                    color: parent.hovered ? Colors.background : Colors.surface2
                }
            }
        }
        
        // 编辑器区域
        StackLayout {
            id: editorStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            
            Repeater {
                model: tabModel
                delegate: EEditor {
                    required property int index
                    required property var model
                    
                    color: Colors.surface1
                    showLineNumbers: root.showLineNumbers
                    currentFilePath: model.filePath
                }
            }
        }
    }
}