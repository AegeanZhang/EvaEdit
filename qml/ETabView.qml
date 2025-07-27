import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQml.Models

import Logger

import EvaEdit

pragma ComponentBehavior: Bound

Rectangle {
    id: root

    // 公共属性
    property bool showLineNumbers: true
    property var openFiles: []  // 存储打开的文件路径
    property int currentTabIndex: tabBar.currentIndex
    property string currentFilePath: currentTabIndex >= 0 && currentTabIndex < openFiles.length 
                                  ? openFiles[currentTabIndex] : ""

    // 信号
    signal filePathChanged(string filePath)

    color: Colors.surface1

    // 添加新标签页的函数
    function addNewTab(filePath) {
        // filePath 是否为空字符串
        if (filePath) {
            // 如果文件已打开，切换到该标签
            for (let i = 0; i < openFiles.length; i++) {
                if (openFiles[i] === filePath) {
                    tabBar.currentIndex = i;
                    return;
                }
            }

            openFiles.push(filePath);
            tabModel.append({
                "filePath": filePath,
                "fileName": filePath.substring(filePath.lastIndexOf("/") + 1)
            });
        } else {
            // 新建空白标签页
            const newTabId = "新标签页 " + (tabModel.count + 1);
            openFiles.push("");
            tabModel.append({
                "filePath": "",
                "fileName": newTabId
            });
        }

        tabBar.currentIndex = tabModel.count - 1;
        root.filePathChanged(root.currentFilePath);
    }

    // 关闭标签页
    function closeTab(index) {
        openFiles.splice(index, 1);
        tabModel.remove(index);
        
        // 如果没有标签页，创建一个新的空白页
        if (tabModel.count === 0) {
            addNewTab("");
        }
        
        // 防止索引越界
        if (tabBar.currentIndex >= tabModel.count) {
            tabBar.currentIndex = tabModel.count - 1;
        }
        
        root.filePathChanged(root.currentFilePath);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标签栏
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            
            background: Rectangle {
                color: Colors.surface2
            }
            
            // 标签模型
            ListModel {
                id: tabModel
            }
            
            // 如果没有标签页，自动添加一个空白页
            Component.onCompleted: {
                if (tabModel.count === 0) {
                    root.addNewTab("");
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
                            
                            Text {
                                anchors.centerIn: parent
                                text: "×"
                                color: closeMouseArea.containsMouse ? Colors.iconIndicator : Colors.text
                            }
                            
                            MouseArea {
                                id: closeMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.closeTab(tabButton.tabIndex)
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
                    
                    onClicked: {
                        tabBar.currentIndex = tabButton.tabIndex;
                        root.filePathChanged(root.currentFilePath);
                    }
                }
            }
            
            // 添加按钮
            TabButton {
                width: 40
                text: "+"
                onClicked: addNewTab("")
                
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
            
            // 当前索引变化时触发事件
            onCurrentIndexChanged: {
                if (currentIndex >= 0 && currentIndex < openFiles.length) {
                    root.filePathChanged(root.currentFilePath);
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