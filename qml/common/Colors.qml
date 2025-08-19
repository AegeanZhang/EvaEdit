import QtQuick

import EvaEdit

pragma Singleton

QtObject {
    id: root

    // ��������ɫ�������˵���
    property color background
    // �༭��������ɫ, TabButtonѡ��ʱҲ�������ɫ
    property color surface1
    property color surface2
    // ������ͼ�ı߿���ɫ, �����뱳������
    property color viewBorder
    // �������ɫ
    property color text
    property color textFile
    property color disabledText
    // ѡ��ʱ����ɫ������TreeView��Ŀ¼�����ļ���ѡ��ʱ����ɫ
    property color selection
    // �˵����TreeView��Ŀ¼���ļ���ͣ����ǩҳ������ʱ����ɫ
    property color active
    // �����ͣʱ��������ɫ����activeһ��Ҫ�з������active����ɫ, activeText�����ǰ�ɫ
    property color activeText
    property color inactive
    property color folder
    property color icon
    property color iconIndicator
    // ����ʹ��icon��ΪButton����ɫ
    property color iconButton
    // ����������ʱ����ɫ
    property color scrollBarActive
    // TODO ��������ɫ�Ķ��岻������ Ҫ���滻��
    property color color1
    // TODO �����ɫ��TreeView �ļ��д򿪣���TabButton �رհ�ť��ͣʱ����ɫ
    property color color2

    property QtObject currentTheme: null
    
    // ��ʼ��ʱ��ThemeManager��ȡ��ɫ
    Component.onCompleted: {        
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