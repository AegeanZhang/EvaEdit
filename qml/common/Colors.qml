import QtQuick

import EvaEdit

pragma Singleton

/*QtObject {
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
    readonly property color selection: "#4B4A4A"
    readonly property color active: "#292828"
    readonly property color inactive: "#383737"
    readonly property color folder: "#383737"
    readonly property color icon: "#383737"
    readonly property color iconIndicator: "#D5B35D"
    readonly property color color1: "#A7B464"
    readonly property color color2: "#D3869B"
}*/

QtObject {
    id: root

    // 直接定义属性而非别名
    property color background
    property color surface1
    property color surface2
    property color text
    property color textFile
    property color disabledText
    property color selection
    property color active
    property color inactive
    property color folder
    property color icon
    property color iconIndicator
    property color color1
    property color color2

    property QtObject currentTheme: null
    
    // 初始化时从ThemeManager获取颜色
    Component.onCompleted: {        
        // 监听主题变化
        //ThemeManager.themeChanged.connect(updateColorsFromTheme)

        //currentTheme = DarkTheme;
        //currentTheme = LightTheme;
        //updateColorsFromTheme()

                // 从配置中加载保存的主题
        var savedTheme = ConfigCenter.currentTheme
        if (savedTheme === "Dark") {
            currentTheme = DarkTheme
        } else if (savedTheme === "Light") {
            currentTheme = LightTheme  // 默认使用浅色主题
        } else {
            currentTheme = DarkTheme  // 如果没有保存的主题，使用深色主题
        }
        updateColorsFromTheme()
    }

    function setTheme(theme) {
        // 设置当前主题
        currentTheme = theme;
        updateColorsFromTheme();
        
        // 保存主题到配置文件
        var themeName = ""
        if (theme === DarkTheme) {
            themeName = "Dark"
        } else if (theme === LightTheme) {
            themeName = "Light"
        }
        
        if (themeName !== "") {
            ConfigCenter.currentTheme = themeName
        }
    }
    
    // 从ThemeManager更新颜色的函数
    function updateColorsFromTheme() {
        background = root.currentTheme.background
        surface1 = root.currentTheme.surface1
        surface2 = root.currentTheme.surface2
        text = root.currentTheme.text
        textFile = root.currentTheme.textFile
        disabledText = root.currentTheme.disabledText
        selection = root.currentTheme.selection
        active = root.currentTheme.active
        inactive = root.currentTheme.inactive
        folder = root.currentTheme.folder
        icon = root.currentTheme.icon
        iconIndicator = root.currentTheme.iconIndicator
        color1 = root.currentTheme.color1
        color2 = root.currentTheme.color2
    }
}