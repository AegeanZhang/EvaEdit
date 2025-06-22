import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

import EvaEdit

MenuBar {
    id: root


    background: Rectangle {
        color: Colors.surface2
        // Make the empty space drag the specified root window.
        //WindowDragHandler {
        //    dragWindow: root.dragWindow
        //}
    }
}