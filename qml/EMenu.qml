import QtQuick
import QtQuick.Controls.Basic

import EvaEdit

Menu {
    id: root

    delegate: MenuItem {
        id: menuItem
        contentItem: Item {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 5

                text: menuItem.text
                color: enabled ? Colors.text : Colors.disabledText
            }
            Rectangle {
                id: indicator

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                width: 6
                height: parent.height

                visible: menuItem.highlighted
                color: Colors.color2
            }
        }
        background: Rectangle {
            implicitWidth: 210
            implicitHeight: 35
            color: menuItem.highlighted ? Colors.active : "transparent"
        }
    }

    background: Rectangle {
        implicitWidth: 210
        implicitHeight: 35
        color: Colors.surface2
    }
}