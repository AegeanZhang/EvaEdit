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

    // 公共属性
    property bool showLineNumbers: true

    // 信号
    signal filePathChanged(string filePath)

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

    // 🔥 向外暴露的接口（保持兼容性）
    //function addNewTab(filePath) {
    //    return TabController.addNewTab(filePath);
    //}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标签栏
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            // 🔥 绑定到控制器
            currentIndex: TabController.currentTabIndex
            onCurrentIndexChanged: TabController.currentTabIndex = currentIndex
            
            background: Rectangle {
                color: Colors.surface2
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
                            
                            /*Text {
                                anchors.centerIn: parent
                                text: "×"
                                color: closeMouseArea.containsMouse ? Colors.iconIndicator : Colors.text
                            }*/
                            Image {
                                id: closeIcon
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                source: "../../resources/icons/close_btn.svg"
                                sourceSize: Qt.size(12, 12)
                                visible: false // 隐藏原始图像
                            }

                            MultiEffect {
                                source: closeIcon
                                //anchors.fill: closeIcon
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                colorization: 1.0
                                colorizationColor: closeMouseArea.containsMouse ? "#ffffff" : "#888888"  // 悬停时白色，正常时灰色
        
                                // 添加平滑的颜色过渡动画
                                Behavior on colorizationColor {
                                    ColorAnimation {
                                        duration: 200
                                        easing.type: Easing.OutCubic
                                    }
                                }
                            }

                            MouseArea {
                                id: closeMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                //onClicked: root.closeTab(tabButton.tabIndex)
                                onClicked: TabController.closeTab(tabButton.tabIndex)
                            }
                        }
                    }
                    
                    background: Rectangle {
                        color: tabBar.currentIndex === tabButton.tabIndex ? Colors.surface1 : Colors.surface2
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
                width: 40
                text: "+"
                //onClicked: addNewTab("")
                onClicked: TabController.addNewTab()
                
                contentItem: Text {
                    text: parent.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Colors.text
                }
                
                background: Rectangle {
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