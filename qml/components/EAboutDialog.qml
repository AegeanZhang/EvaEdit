import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import EvaEdit

Dialog {
    id: root
    modal: true
    title: qsTr("关于 EvaEdit")

    // 显式设置对话框尺寸，避免绑定循环
    width: 500
    height: 300

    // 设置窗口位置
    x: Screen.desktopAvailableWidth / 2 - root.width / 2
    y: Screen.desktopAvailableHeight / 2 - root.height / 2

    standardButtons: Dialog.Ok

    contentItem: ColumnLayout {
        spacing: 12
        //width: 320
        anchors.fill: parent
        anchors.margins: 20

        Label {
            text: qsTr("EvaEdit - 轻量级文本编辑器")
            font.pixelSize: 18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("版本 1.0\n\nCopyright © 2025 AegeanZhang\n\n项目地址: https://github.com/AegeanZhang/EvaEdit")
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
        }
    }
}