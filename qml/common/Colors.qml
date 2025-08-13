import QtQuick

import EvaEdit

pragma Singleton

/*QtObject {
    // ��������ɫ�������˵���
    readonly property color background: "#292828"
    readonly property color surface1: "#171819"
    // ������ͱ༭��������ɫ
    readonly property color surface2: "#1F1F1F"
    // �������ɫ��ǳ��ɫ
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

    // ֱ�Ӷ������Զ��Ǳ���
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
    
    // ��ʼ��ʱ��ThemeManager��ȡ��ɫ
    Component.onCompleted: {        
        // ��������仯
        //ThemeManager.themeChanged.connect(updateColorsFromTheme)

        //currentTheme = DarkTheme;
        //currentTheme = LightTheme;
        //updateColorsFromTheme()

                // �������м��ر��������
        var savedTheme = ConfigCenter.currentTheme
        if (savedTheme === "Dark") {
            currentTheme = DarkTheme
        } else if (savedTheme === "Light") {
            currentTheme = LightTheme  // Ĭ��ʹ��ǳɫ����
        } else {
            currentTheme = DarkTheme  // ���û�б�������⣬ʹ����ɫ����
        }
        updateColorsFromTheme()
    }

    function setTheme(theme) {
        // ���õ�ǰ����
        currentTheme = theme;
        updateColorsFromTheme();
        
        // �������⵽�����ļ�
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
    
    // ��ThemeManager������ɫ�ĺ���
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