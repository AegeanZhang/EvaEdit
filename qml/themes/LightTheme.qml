import QtQuick

pragma Singleton

QtObject {
    // 主背景颜色，包括菜单等
    readonly property color background: "#F8F9FA"
    readonly property color surface1: "#FFFFFF"
    // 侧边栏和编辑器背景颜色
    readonly property color surface2: "#F1F3F4"
    readonly property color viewBorder: "#454545"
    // 字体的颜色，深色
    readonly property color text: "#202124"
    readonly property color textFile: "#1A1A1A"
    readonly property color disabledText: "#9AA0A6"
    readonly property color selection: "#E8F0FE"
    //readonly property color active: "#E8F0FE"
    readonly property color active: "#2160BB"
    readonly property color activeText: "#fefefe"
    readonly property color inactive: "#F8F9FA"
    readonly property color folder: "#DADCE0"
    readonly property color icon: "#5F6368"
    readonly property color iconIndicator: "#1A73E8"
    readonly property color color1: "#34A853"  // 绿色主题色
    readonly property color color2: "#EA4335"  // 红色主题色
}