import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Basic

import EvaEdit

pragma ComponentBehavior: Bound

Rectangle {
    id: root

    signal fileClicked(string filePath)
    property alias rootIndex: fileSystemTreeView.rootIndex

    // ����ڱ߾����ԣ����Է������
    property int padding: 5

    TreeView {
        id: fileSystemTreeView

        property int lastIndex: -1

        anchors.fill: parent
        anchors.margins: root.padding

        model: FileSystemModel
        rootIndex: FileSystemModel.rootIndex
        boundsBehavior: Flickable.StopAtBounds
        boundsMovement: Flickable.StopAtBounds
        clip: true

        Component.onCompleted: fileSystemTreeView.toggleExpanded(0)

        // The delegate represents a single entry in the filesystem.
        delegate: TreeViewDelegate {
            id: treeDelegate
            indentation: 8
            implicitWidth: fileSystemTreeView.width > 0 ? fileSystemTreeView.width : 250
            implicitHeight: 25

            // Since we have the 'ComponentBehavior Bound' pragma, we need to
            // require these properties from our model. This is a convenient way
            // to bind the properties provided by the model's role names.
            required property int index
            required property url filePath
            required property string fileName

            indicator: Image {
                id: directoryIcon

                x: treeDelegate.leftMargin + (treeDelegate.depth * treeDelegate.indentation)
                anchors.verticalCenter: parent.verticalCenter
                source: treeDelegate.hasChildren ? (treeDelegate.expanded
                            ? "../../resources/icons/folder_open.svg" : "../../resources/icons/folder_closed.svg")
                        : "../../resources/icons/generic_file.svg"
                sourceSize.width: 20
                sourceSize.height: 20
                fillMode: Image.PreserveAspectFit

                smooth: true
                antialiasing: true
                asynchronous: true
            }

            contentItem: Text {
                text: treeDelegate.fileName
                color: hoverHandler.hovered && treeDelegate.index !== fileSystemTreeView.lastIndex
                        ? Colors.activeText
                        : Colors.text
            }

            background: Rectangle {
                color: (treeDelegate.index === fileSystemTreeView.lastIndex)
                    ? Colors.selection
                    : (hoverHandler.hovered ? Colors.active : "transparent")
            }

            // We color the directory icons with this MultiEffect, where we overlay
            // the colorization color ontop of the SVG icons.
            MultiEffect {
                id: iconOverlay

                anchors.fill: directoryIcon
                source: directoryIcon
                colorization: 1.0
                brightness: 1.0
                colorizationColor: {
                    const isFile = treeDelegate.index === fileSystemTreeView.lastIndex
                                    && !treeDelegate.hasChildren;
                    if (isFile)
                        return Qt.lighter(Colors.folder, 3)

                    const isExpandedFolder = treeDelegate.expanded && treeDelegate.hasChildren;
                    if (isExpandedFolder)
                        return Colors.color2
                    else
                        return Colors.folder
                }
            }

            HoverHandler {
                id: hoverHandler
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onSingleTapped: (eventPoint, button) => {
                    switch (button) {
                        case Qt.LeftButton:
                            fileSystemTreeView.toggleExpanded(treeDelegate.row)
                            fileSystemTreeView.lastIndex = treeDelegate.index
                            // If this model item doesn't have children, it means it's
                            // representing a file.
                            if (!treeDelegate.hasChildren)
                                root.fileClicked(treeDelegate.filePath)
                        break;
                        case Qt.RightButton:
                            if (treeDelegate.hasChildren)
                                contextMenu.popup();
                        break;
                    }
                }
            }

            EMenu {
                id: contextMenu
                Action {
                    text: qsTr("Set as root index")
                    onTriggered: {
                        fileSystemTreeView.rootIndex = fileSystemTreeView.index(treeDelegate.row, 0)
                    }
                }
                Action {
                    text: qsTr("Reset root index")
                    onTriggered: fileSystemTreeView.rootIndex = undefined
                }
            }
        }

        // ��� HoverHandler ����������ͣ
        HoverHandler {
            id: treeViewHoverHandler
        }

        // Provide our own custom ScrollIndicator for the TreeView.
        ScrollIndicator.vertical: ScrollIndicator {
            active: true
            implicitWidth: 15

            // ����Ƿ������Ҫ������
            property bool needsScrollBar: {
                if (!fileSystemTreeView.model) return false;
                
                // �������пɼ�����ܸ߶�
                var totalContentHeight = 0;
                var delegateHeight = 25; // �� delegate �� implicitHeight ����һ��
                
                // ��ȡ��ǰ��Ŀ¼�µ���Ŀ����������չ�������
                var visibleItems = getVisibleItemCount();
                totalContentHeight = visibleItems * delegateHeight;
                
                // �Ƚ����ݸ߶����������߶�
                return totalContentHeight > fileSystemTreeView.height;
            }

            // ����ɼ���Ŀ����������չ�����ļ����е���Ŀ��
            function getVisibleItemCount() {
                if (!fileSystemTreeView.model || !fileSystemTreeView.rootIndex.valid) {
                    return 0;
                }
                
                // �򻯼��㣺ʹ�� TreeView �� rows ����
                // TreeView ���Զ�����չ��״̬�µĿɼ�����
                return fileSystemTreeView.rows || 0;
            }

            // ֻ������Ҫ�������������ͣ�����ڹ���ʱ����ʾ
            visible: needsScrollBar

            contentItem: Rectangle {
                implicitWidth: 6
                implicitHeight: 6

                color: Colors.scrollBarActive
                // �޸�͸�����߼��������ͣ�����ڹ���ʱ��ʾ
                opacity: parent.visible 
                    && (treeViewHoverHandler.hovered || fileSystemTreeView.movingVertically) ? 0.5 : 0.0

                Behavior on opacity {
                    OpacityAnimator {
                        duration: 500
                    }
                }
            }
        }
    }
}
