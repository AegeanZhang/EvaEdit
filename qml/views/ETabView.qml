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
    readonly property int tabButtonHeight: 36

    // 公共属性
    property bool showLineNumbers: true

    // 信号
    signal filePathChanged(string filePath)

    color: Colors.surface1
    // 添加边框
    border.width: EConstants.borderWidth
    border.color: Colors.viewBorder
  
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
            Qt.callLater(function() {
                if (tabIndex >= 0 && tabIndex < editorStack.count) {
                    var editor = editorStack.itemAt(tabIndex);
                    if (editor && editor.setFocus) {
                        editor.setFocus();
                    }
                }
            });
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
        anchors.margins: root.border.width
        spacing: 0
        
        // 标签栏
        TabBar {
            id: tabBar

            Layout.fillWidth: true
            Layout.preferredHeight: root.tabBarHeight
            
            background: Rectangle {
                color: Colors.surface2
            }
            
            // 标签模型
            ListModel {
                id: tabModel
            }

            // 监听 TabController 的索引变化并更新 TabBar
            Connections {
                target: TabController
                function onCurrentTabIndexChanged() {
                    tabBar.currentIndex = TabController.currentTabIndex;
                }
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

                // 初始化 TabBar 的当前索引
                tabBar.currentIndex = TabController.currentTabIndex;
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
                    //anchors.verticalCenter: parent.verticalCenter
                    anchors.bottom: parent.bottom
                    
                    contentItem: RowLayout {
                        Text {
                            Layout.fillWidth: true
                            text: tabButton.tabTitle
                            elide: Text.ElideRight
                            color: tabBar.currentIndex === tabButton.tabIndex ? Colors.textFile : Colors.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolButton {
                            //implicitWidth: 16
                            //implicitHeight: 16
                            width: 16
                            height: 16
    
                            icon.source: "../../resources/icons/close_btn.svg"
                            icon.width: 14
                            icon.height: 14
                            icon.color: Colors.iconButton
    
                            display: AbstractButton.IconOnly
                            background: Rectangle {
                                anchors.fill: parent
                                radius: 8
                                color: parent.hovered ? Colors.color2 : "transparent"
                            }
    
                            onClicked: TabController.closeTab(tabButton.tabIndex)
                            hoverEnabled: true
                        }
                    }
                    
                    background: Rectangle {
                        anchors.fill: parent  // 确保背景填满按钮区域
                        
                        color: tabBar.currentIndex === tabButton.tabIndex ? Colors.surface1 : Colors.surface2

                        Rectangle {
                            width: parent.width
                            height: 3
                            anchors.top: parent.top
                            //color: tabBar.currentIndex === tabButton.tabIndex ? Colors.color1 : "transparent"
                            color: tabBar.currentIndex === tabButton.tabIndex ? Colors.active : "transparent"
                        }
                    }
                    
                    onClicked: {
                        if (TabController.currentTabIndex !== tabButton.tabIndex) {
                            TabController.currentTabIndex = tabButton.tabIndex;
                        }
                    }
                }
            }
            
            // 添加按钮
            TabButton {
                id: addTabButton

                width: 30
                height: root.tabButtonHeight

                anchors.verticalCenter: parent.verticalCenter

                onClicked: TabController.addNewTab()

                icon.source: "../../resources/icons/add_btn.svg"
                icon.width: 14
                icon.height: 14
                icon.color: Colors.iconButton  // 直接设置图标颜色

                display: AbstractButton.IconOnly  // 只显示图标
                
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
                /*delegate: EEditor {
                    required property int index
                    required property var model
                    
                    color: Colors.surface1
                    showLineNumbers: root.showLineNumbers
                    currentFilePath: model.filePath
                }*/
                delegate: ETextEditor {
                    required property int index
                    required property var model
                    
                    currentFilePath: model.filePath
                    showLineNumbers: root.showLineNumbers
                    color: Colors.surface1
                    
                    // 处理另存为请求
                    onSaveAsRequested: {
                        // 这里可以触发文件对话框
                        // FileController.showSaveAsDialog(model.filePath);
                    }
                    
                    // 监听内容修改
                    onContentModified: {
                        // 可以更新标签页的修改指示器
                        // 例如在文件名后加 "*" 
                    }
                }
            }
        }
    }
}