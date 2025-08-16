import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Logger
import EvaEdit

pragma ComponentBehavior: Bound

Rectangle {
    id: root

    required property string currentFilePath
    required property bool showLineNumbers
    property alias text: textArea
    property int currentLineNumber: -1

    property int rowHeight: lineNumberModel.calculateRowHeight(textArea.font)

    function calculateDebounceDelay() {
        var textLength = textArea.text.length;
        var estimatedSize = textLength * 2; // UTF-16 大致估算
        
        if (estimatedSize < 1024 * 1024) return 100;        // < 1MB
        if (estimatedSize < 10 * 1024 * 1024) return 300;   // < 10MB  
        if (estimatedSize < 64 * 1024 * 1024) return 500;   // < 64MB
        return 1000;                                         // >= 64MB
    }

    function performContentUpdate() {
        contentUpdateTimer.stop();
        throttleTimer.stop();
        
        if (root.currentFilePath) {
            FileController.updateEditorContent(root.currentFilePath, textArea.text);
            Logger.debug("内容已同步到 FileController: " + root.currentFilePath + 
                        ", 长度: " + textArea.text.length);
        }
    }

    function scheduleContentUpdate() {
        // 动态计算延迟
        var newDelay = calculateDebounceDelay();
        contentUpdateTimer.interval = newDelay;
        contentUpdateTimer.restart();
        
        // 如果节流定时器没有运行，启动它
        if (!throttleTimer.running) {
            throttleTimer.start();
        }
    }

    function forceUpdateContent() {
        contentUpdateTimer.stop();
        throttleTimer.stop();
        performContentUpdate();
    }

    color: Colors.background

    onWidthChanged: textArea.update()
    onHeightChanged: textArea.update()

    Connections {
        target: FileController
        
        function onForceContentUpdateRequested(filePath) {
            if (filePath === root.currentFilePath) {
                root.forceUpdateContent();
            }
        }
    }

    // 监听textArea.font的变化
    Connections {
        target: textArea
        function onFontChanged() {
            // 重新计算行高
            rowHeight = Math.ceil(fontMetrics.lineSpacing) + fontMetrics.leading;
        }
    }

    // 防抖定时器
    Timer {
        id: contentUpdateTimer
        interval: 300 // 默认值，会被动态调整
        repeat: false
        onTriggered: root.performContentUpdate()
    }

    // 节流定时器 - 强制最大延迟
    Timer {
        id: throttleTimer
        interval: 2000 // 2秒强制更新
        repeat: false
        onTriggered: {
            contentUpdateTimer.stop();
            root.performContentUpdate();
        }
    }

    RowLayout {
        anchors.fill: parent
        // We use a flickable to synchronize the position of the editor and
        // the line numbers. This is necessary because the line numbers can
        // extend the available height.
        Flickable {
            id: lineNumbers

            // Calculate the width based on the logarithmic scale.
            Layout.preferredWidth: fontMetrics.averageCharacterWidth
                * (Math.floor(Math.log10(textArea.lineCount)) + 1) + 10
            Layout.fillHeight: true
            Layout.fillWidth: false

            interactive: false
            contentY: editorFlickable.contentY
            visible: textArea.text !== "" && root.showLineNumbers
            clip: true

            Column {
                anchors.fill: parent
                Repeater {
                    id: repeatedLineNumbers

                    model: LineNumberModel {
                        id: lineNumberModel
                        lineCount: textArea.text !== "" ? textArea.lineCount : 0
                    }

                    delegate: Item {
                        required property int index

                        width: parent.width
                        height: root.rowHeight
                        Label {
                            id: numbers

                            text: parent.index + 1

                            width: parent.width
                            height: parent.height
                            //horizontalAlignment: Text.AlignLeft
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter

                            color: (root.currentLineNumber === parent.index)
                                    ? Colors.iconIndicator : Qt.darker(Colors.text, 2)
                        }
                        Rectangle {
                            id: indicator

                            anchors.left: numbers.right
                            width: 1
                            height: parent.height
                            color: Qt.darker(Colors.text, 3)
                        }
                    }
                }
            }
        }

        Flickable {
            id: editorFlickable

            property alias textArea: textArea

            // We use an inline component to customize the horizontal and vertical
            // scroll-bars. This is convenient when the component is only used in one file.
            component MyScrollBar: ScrollBar {
                id: scrollBar
                background: Rectangle {
                    implicitWidth: scrollBar.interactive ? 8 : 4
                    implicitHeight: scrollBar.interactive ? 8 : 4

                    opacity: scrollBar.active && scrollBar.size < 1.0 ? 1.0 : 0.0
                    color: Colors.background
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 500
                        }
                    }
                }
                contentItem: Rectangle {
                    implicitWidth: scrollBar.interactive ? 8 : 4
                    implicitHeight: scrollBar.interactive ? 8 : 4
                    opacity: scrollBar.active && scrollBar.size < 1.0 ? 1.0 : 0.0
                    color: Colors.color1
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 1000
                        }
                    }
                }
            }

            Layout.fillHeight: true
            Layout.fillWidth: true
            ScrollBar.horizontal: MyScrollBar {}
            ScrollBar.vertical: MyScrollBar {}

            boundsBehavior: Flickable.StopAtBounds

            TextArea.flickable: TextArea {
                id: textArea
                anchors.fill: parent

                focus: false
                topPadding: 0
                leftPadding: 10

                text: ""
                tabStopDistance: fontMetrics.averageCharacterWidth * 4

                font.family: "Consolas"
                font.pixelSize: 14

                Component.onCompleted: {
                    Logger.debug("创建了一个TextArea: " + root.currentFilePath)
                    if(TabController.isNewFile(root.currentFilePath)) {
                        // If the file is new, we create an empty text area.
                        text = "123";
                    } else {
                        // If the file is not new, we read the content from the file system.
                        text = FileSystemModel.readFile(root.currentFilePath);
                    }

                    lineNumberModel.setFixedLineHeight(textArea.textDocument, root.rowHeight);
                    // 初始化时立即更新内容到 FileController
                    root.forceUpdateContent();
                }

                // 监听行高变化
                /*Connections {
                    target: root
                    function onRowHeightChanged() {
                        lineNumberModel.setFixedLineHeight(textArea.textDocument, root.rowHeight);
                    }
                }*/

                // Grab the current line number from the C++ interface.
                onCursorPositionChanged: {
                    root.currentLineNumber = FileSystemModel.currentLineNumber(
                        textArea.textDocument, textArea.cursorPosition)
                }
                
                // 文本变化时使用防抖+节流机制
                onTextChanged: {
                    root.scheduleContentUpdate();
                }

                color: Colors.textFile
                selectedTextColor: Colors.textFile
                selectionColor: Colors.selection

                textFormat: TextEdit.PlainText
                renderType: Text.QtRendering
                selectByMouse: true
                antialiasing: true
                background: null
            }

            FontMetrics {
                id: fontMetrics
                font: textArea.font
            }
        }
    }
}