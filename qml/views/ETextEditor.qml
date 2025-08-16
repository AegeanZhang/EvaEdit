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
        
        Component.onCompleted: {
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
        }
    }

    // 主要的文本渲染器
    TextRenderer {
        id: textRenderer
        anchors.fill: parent
        
        // 绑定文档模型
        document: documentModel
        
        // 基本属性
        lineNumbers: root.showLineNumbers
        font.family: "Consolas"
        font.pixelSize: 14
        backgroundColor: Colors.background
        textColor: Colors.textFile
        
        // 焦点处理
        focus: true
        
        Component.onCompleted: {
            Logger.debug("TextRenderer 创建完成: " + root.currentFilePath);
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