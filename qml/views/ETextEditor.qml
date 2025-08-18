import QtQuick
import QtQuick.Controls

import Logger
import EvaEdit

pragma ComponentBehavior: Bound

Rectangle {
    id: root

    // 必需属性
    required property string currentFilePath
    required property bool showLineNumbers

    // 提供与原 EEditor 兼容的接口
    property alias textRenderer: textRenderer
    property alias document: documentModel

    // 焦点管理
    function setFocus() {
        textRenderer.forceActiveFocus();
    }

    // 文件操作
    function saveFile() {
        return documentModel.saveToFile();
    }

    function saveAsFile(filePath) {
        return documentModel.saveToFile(filePath);
    }

    // 状态信息
    readonly property bool isModified: documentModel.isModified
    readonly property bool isReadOnly: documentModel.isReadOnly
    readonly property int lineCount: documentModel.lineCount
    readonly property string textContent: documentModel.getFullText()

    color: Colors.background

    // 文档模型
    DocumentModel {
        id: documentModel
        
        onTextChanged: function(change) {
            // 文档内容改变时，TextRenderer 会自动更新显示
            // 由于数据在 C++ 侧，不需要手动同步
            Logger.debug("文档内容变化: 位置=" + change.position + 
                        ", 删除长度=" + change.removedLength + 
                        ", 插入文本长度=" + change.insertedText.length);
        }
        
        onModifiedChanged: function(modified) {
            Logger.debug("文档修改状态: " + modified + " - " + root.currentFilePath);
        }
        
        /*Component.onCompleted: {
            Logger.debug("DocumentModel 创建完成: " + root.currentFilePath);
            initializeDocument();
        }
        
        // 初始化文档内容
        function initializeDocument() {
            if (TabController.isNewFile(root.currentFilePath)) {
                // 新文件，设置为空文档
                setModified(false);
                Logger.debug("初始化新文件: " + root.currentFilePath);
            } else {
                // 从文件加载内容
                if (loadFromFile(root.currentFilePath)) {
                    Logger.debug("从文件加载成功: " + root.currentFilePath);
                } else {
                    Logger.error("从文件加载失败: " + root.currentFilePath);
                    // 如果 DocumentModel 加载失败，尝试用 FileSystemModel 读取
                    try {
                        var content = FileSystemModel.readFile(root.currentFilePath);
                        replaceText(0, textLength, content);
                        setModified(false);
                        Logger.debug("使用 FileSystemModel 读取成功: " + root.currentFilePath);
                    } catch (error) {
                        Logger.error("文件读取完全失败: " + root.currentFilePath + " - " + error);
                    }
                }
            }
            
            // 设置文件路径
            setFilePath(root.currentFilePath);
        }*/
        Component.onCompleted: {
            Logger.debug("DocumentModel 创建完成: " + root.currentFilePath);
            Qt.callLater(initializeDocument);
        }
        
        // 初始化文档内容
        function initializeDocument() {
            Logger.debug("开始初始化文档: " + root.currentFilePath);
            
            if (!root.currentFilePath || root.currentFilePath === "") {
                Logger.error("文件路径为空，无法初始化文档");
                return;
            }

            // 设置文件路径
            documentModel.filePath = root.currentFilePath;

            if (TabController.isNewFile(root.currentFilePath)) {
                // 新文件，设置为空文档
                documentModel.isModified = false;
                Logger.debug("初始化新文件: " + root.currentFilePath);
            } else {
                // 从文件加载内容
                Logger.debug("开始从文件加载: " + root.currentFilePath);
                if (documentModel.loadFromFile(root.currentFilePath)) {
                    Logger.debug("从文件加载成功: " + root.currentFilePath);
                } else {
                    Logger.error("DocumentModel 加载失败，尝试 FileSystemModel: " + root.currentFilePath);
                    // 备用方案：使用 FileSystemModel
                    try {
                        var content = FileSystemModel.readFile(root.currentFilePath);
                        if (content !== undefined && content !== null) {
                            documentModel.replaceText(0, documentModel.textLength, content);
                            documentModel.isModified = false;
                            Logger.debug("使用 FileSystemModel 读取成功: " + root.currentFilePath);
                        } else {
                            Logger.error("FileSystemModel 返回空内容: " + root.currentFilePath);
                        }
                    } catch (error) {
                        Logger.error("文件读取完全失败: " + root.currentFilePath + " - " + error);
                    }
                }
            }
        }
    }

    // 监听文件路径变化
    onCurrentFilePathChanged: {
        Logger.debug("文件路径变化: " + currentFilePath);
        if (documentModel && currentFilePath) {
            documentModel.initializeDocument();
        }
    }

    // 容器包含 TextRenderer 和滚动条
    Item {
        id: container
        anchors.fill: parent
        // 主要的文本渲染器
        TextRenderer {
            id: textRenderer
            anchors.fill: parent
            anchors.rightMargin: verticalScrollBar.visible ? verticalScrollBar.width : 0
            anchors.bottomMargin: horizontalScrollBar.visible ? horizontalScrollBar.height : 0
        
            // 绑定文档模型
            document: documentModel
        
            // 基本属性
            lineNumbers: root.showLineNumbers
            lineNumberSeparatorColor: Colors.viewBorder
            lineNumberExtraWidth: 32
            font.family: "Consolas"
            font.pixelSize: 14
            //font.pointSize: 14
            backgroundColor: Colors.surface1
            textColor: Colors.text
        
            // 焦点处理
            focus: true
        
            Component.onCompleted: {
                Logger.debug("TextRenderer 创建完成: " + root.currentFilePath);
            }
        }

        TextEditorController {
            id: editorController
            renderer: textRenderer
            document: documentModel
        }

        // 垂直滚动条
        ScrollBar {
            id: verticalScrollBar
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: horizontalScrollBar.visible ? horizontalScrollBar.top : parent.bottom
            
            active: true
            policy: ScrollBar.AsNeeded
            
            // 计算内容高度和可见区域
            size: {
                if (!documentModel) return 1.0;
                var totalHeight = documentModel.lineCount * 20; // 假设行高为20
                var viewHeight = textRenderer.height;
                return Math.min(1.0, viewHeight / totalHeight);
            }
            
            position: {
                if (!documentModel) return 0.0;
                var totalHeight = documentModel.lineCount * 20;
                var viewHeight = textRenderer.height;
                if (totalHeight <= viewHeight) return 0.0;
                return textRenderer.scrollY / (totalHeight - viewHeight);
            }
            
            onPositionChanged: {
                if (pressed && documentModel) {
                    var totalHeight = documentModel.lineCount * 20;
                    var viewHeight = textRenderer.height;
                    var maxScroll = Math.max(0, totalHeight - viewHeight);
                    textRenderer.scrollY = position * maxScroll;
                }
            }
            
            background: Rectangle {
                implicitWidth: 12
                color: Colors.background
                opacity: 0.3
            }
            
            contentItem: Rectangle {
                implicitWidth: 8
                color: Colors.color1
                opacity: parent.pressed ? 1.0 : 0.7
                radius: 4
            }
        }

        // 水平滚动条
        ScrollBar {
            id: horizontalScrollBar
            anchors.left: parent.left
            anchors.right: verticalScrollBar.visible ? verticalScrollBar.left : parent.right
            anchors.bottom: parent.bottom
            
            active: true
            policy: ScrollBar.AsNeeded
            orientation: Qt.Horizontal
            
            // 计算内容宽度和可见区域
            size: {
                // 这里需要根据实际的内容宽度来计算
                // 可以通过 TextRenderer 提供的方法获取最大行宽度
                var viewWidth = textRenderer.width;
                var maxLineWidth = 1000; // 需要从 TextRenderer 获取实际值
                return Math.min(1.0, viewWidth / maxLineWidth);
            }
            
            position: {
                var viewWidth = textRenderer.width;
                var maxLineWidth = 1000;
                if (maxLineWidth <= viewWidth) return 0.0;
                return textRenderer.scrollX / (maxLineWidth - viewWidth);
            }
            
            onPositionChanged: {
                if (pressed) {
                    var viewWidth = textRenderer.width;
                    var maxLineWidth = 1000;
                    var maxScroll = Math.max(0, maxLineWidth - viewWidth);
                    textRenderer.scrollX = position * maxScroll;
                }
            }
            
            background: Rectangle {
                implicitHeight: 12
                color: Colors.background
                opacity: 0.3
            }
            
            contentItem: Rectangle {
                implicitHeight: 8
                color: Colors.color1
                opacity: parent.pressed ? 1.0 : 0.7
                radius: 4
            }
        }
    }

    // 简化的快捷键支持
    Shortcut {
        sequence: StandardKey.Save
        onActivated: {
            if (root.saveFile()) {
                Logger.debug("文件保存成功: " + root.currentFilePath);
            } else {
                Logger.error("文件保存失败: " + root.currentFilePath);
            }
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+S"
        onActivated: {
            // 另存为功能可以通过信号通知外部处理
            saveAsRequested();
        }
    }

    // 信号
    signal saveAsRequested()
    signal contentModified()

    // 监听修改状态变化
    onIsModifiedChanged: {
        if (isModified) {
            contentModified();
        }
    }
}