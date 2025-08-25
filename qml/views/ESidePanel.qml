import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

import EvaEdit

Rectangle {
    id: root

    property string currentFilePath: ""

    border.width: EConstants.borderWidth
    border.color: Colors.viewBorder

    color: Colors.surface1
    SplitView.preferredWidth: 250
    SplitView.fillHeight: true

    // 添加宽度变化的动画
    Behavior on SplitView.preferredWidth {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    // 添加透明度动画
    Behavior on opacity {
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    // The stack-layout provides different views, based on the
    // selected buttons inside the sidebar.
    StackLayout {
        anchors.fill: parent
        anchors.margins: navigationView.border.width
        currentIndex: sidebar.currentTabIndex

        // Shows the files on the file system.
        EFileView {
            id: fileSystemView

            color: Colors.surface1
            onFileClicked: path => {
                root.currentFilePath = path;
                //tabView.addNewTab(path);
                TabController.addNewTab(path);
            }
        }

        EOutlineView {
            id: outlineView

            //color: "#ffffff"
        }

        // Shows the help text.
        Text {
            text: qsTr("显示EvaEdit的简介")
            wrapMode: TextArea.Wrap
            color: Colors.text
        }
    }
}