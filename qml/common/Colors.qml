import QtQuick

import EvaEdit

pragma Singleton

QtObject {
    id: root

    // 主背景颜色，包括菜单等
    property color background
    // 编辑区背景颜色, TabButton选中时也是这个颜色
    property color surface1
    property color surface2
    // 各个视图的边框颜色, 方便与背景区分
    property color viewBorder
    // 字体的颜色
    property color text
    property color textFile
    property color disabledText
    // 选中时的颜色，比如TreeView的目录或是文件被选中时的颜色
    property color selection
    // 菜单项和TreeView中目录或文件悬停，标签页被激活时的颜色
    property color active
    // 鼠标悬停时的文字颜色，和active一般要有反差，比如active是蓝色, activeText可以是白色
    property color activeText
    property color inactive
    property color folder
    property color icon
    property color iconIndicator
    // 定义使用icon作为Button的颜色
    property color iconButton
    // 滚动条激活时的颜色
    property color scrollBarActive
    // TODO 这两个颜色的定义不清晰， 要逐渐替换掉
    property color color1
    // TODO 这个颜色是TreeView 文件夹打开，即TabButton 关闭按钮悬停时的颜色
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
        iconButton = root.currentTheme.iconButton
        scrollBarActive = root.currentTheme.scrollBarActive
        color1 = root.currentTheme.color1
        color2 = root.currentTheme.color2
    }
}