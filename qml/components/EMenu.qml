import QtQuick
import QtQuick.Controls.Basic

import EvaEdit

pragma ComponentBehavior: Bound

Menu {
    id: root

    readonly property int menuItemHeight: 30
    readonly property int checkIndicatorWidth: 20
    readonly property int iconWidth: 20  // 图标宽度

    delegate: MenuItem {
        id: menuItem
        //padding: 2
        height: root.menuItemHeight

        // 自定义左侧图标/指示器区域
        indicator: Rectangle {
            implicitWidth: root.iconWidth
            implicitHeight: root.menuItemHeight
            color: "transparent"
            
            // 显示选中标记（当菜单项是可勾选且被勾选的状态）
            Text {
                anchors.centerIn: parent
                text: "✓"
                font.pixelSize: 14
                color: Colors.text
                visible: menuItem.checkable && menuItem.checked
            }

            // 如果有图标则显示图标
            Image {
                id: menuIcon
                anchors.centerIn: parent
                source: menuItem.icon.source
                width: root.iconWidth - 4
                height: width
                visible: menuItem.icon.source !== ""
            }
        }

        contentItem: Item {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                //anchors.leftMargin: 5
                anchors.leftMargin: root.checkIndicatorWidth
                anchors.right: parent.right
                anchors.rightMargin: 5

                text: menuItem.text
                //color: enabled ? Colors.text : Colors.disabledText
                color: menuItem.highlighted ? Colors.activeText : (enabled ? Colors.text : Colors.disabledText)
            }
        }
        background: Rectangle {
            anchors.fill: parent
            anchors.margins: 2  // 让每个菜单项的背景小一圈
            radius: 6
            color: menuItem.highlighted ? Colors.active : "transparent"
        }
    }

    background: Rectangle {
        implicitWidth: 210
        implicitHeight: root.menuItemHeight
        color: Colors.surface2

        // 设置圆角和边框,让菜单弹出时，更好的和其他View区分
        radius: 3
        border.width: 0.5
        border.color: "#454545"
    }
}