import QtQuick

pragma Singleton

QtObject {
    // 主背景颜色，包括菜单等
    readonly property color background: "#292828"
    readonly property color surface1: "#171819"
    // 侧边栏和编辑器背景颜色
    readonly property color surface2: "#1F1F1F"
    // 字体的颜色，浅灰色
    readonly property color text: "#D9D9D6"
    //readonly property color textFile: "#E1D2B7"
    readonly property color textFile: "#D9D9D6"
    readonly property color disabledText: "#2C313A"

    readonly property color selection: "#4B4A4A"  // 灰色
    //readonly property color selection: "#2B79D7"  // 蓝色

    // 菜单项等，鼠标悬停时的颜色
    //readonly property color active: "#292828"
    readonly property color active: "#2B79D7"
    readonly property color activeText: "#fefefe"

    readonly property color inactive: "#383737"
    readonly property color folder: "#383737"
    readonly property color icon: "#383737"
    readonly property color iconIndicator: "#D5B35D"
    readonly property color color1: "#A7B464"
    readonly property color color2: "#D3869B"
}