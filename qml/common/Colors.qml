import QtQuick

import EvaEdit

pragma Singleton

QtObject {
    id: root

    // 主背景颜色，包括菜单等
    property color background
    // 编辑区背景颜色
    property color surface1
    property color surface2
    property color viewBorder
    // 字体的颜色
    property color text
    property color textFile
    property color disabledText
    property color selection
    // 菜单项等，鼠标悬停时的颜色
    property color active
    property color activeText
    property color inactive
    property color folder
    property color icon
    property color iconIndicator
    property color color1
    property color color2

    property QtObject currentTheme: null
    
    // 初始化时从ThemeManager获取颜色
    Component.onCompleted: {        
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
        viewBorder = root.currentTheme.viewBorder
        text = root.currentTheme.text
        textFile = root.currentTheme.textFile
        disabledText = root.currentTheme.disabledText
        selection = root.currentTheme.selection
        active = root.currentTheme.active
        activeText = root.currentTheme.activeText
        inactive = root.currentTheme.inactive
        folder = root.currentTheme.folder
        icon = root.currentTheme.icon
        iconIndicator = root.currentTheme.iconIndicator
        color1 = root.currentTheme.color1
        color2 = root.currentTheme.color2
    }
}