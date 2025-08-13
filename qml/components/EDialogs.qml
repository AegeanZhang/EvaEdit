import QtQuick
import QtQuick.Controls.Basic
import Qt.labs.platform as Platform

import Logger
import EvaEdit

Item {
    id: root
    
    // 信号定义
    signal folderSelected(string folderPath)
    signal fileSelected(string filePath)  
    signal saveAsSelected(string filePath)
    
    // 公开方法
    function openFolderDialog() {
        folderDialog.open()
    }
    
    function openFileDialog() {
        fileDialog.open()
    }
    
    function openSaveAsDialog() {
        saveAsDialog.open()
    }
    
    function setInitialFolder(folderPath) {
        if (folderPath && folderPath.length > 0) {
            folderDialog.folder = folderPath
            fileDialog.folder = folderPath
            saveAsDialog.folder = folderPath
        }
    }
    
    // 文件夹选择对话框
    Platform.FolderDialog {
        id: folderDialog
        title: qsTr("选择文件夹")
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        
        onAccepted: {
            const folderPath = folderDialog.folder.toString()
            const cleanPath = folderPath.replace(/^(file:\/{2,3})/, "")
            Logger.info("文件夹选择完成: " + cleanPath)
            root.folderSelected(cleanPath)
        }
        
        onRejected: {
            Logger.debug("文件夹选择已取消")
        }
    }
    
    // 文件打开对话框
    Platform.FileDialog {
        id: fileDialog
        title: qsTr("打开文件")
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        fileMode: Platform.FileDialog.OpenFile
        nameFilters: [
            qsTr("所有支持的文件 (*.txt *.cpp *.h *.hpp *.c *.cc *.cxx *.hxx *.qml *.js "
                 + "*.json *.xml *.html *.css *.py *.java *.cs *.php *.rb *.go *.rs *.md *.log *.ini *.cfg *.conf)"),
            qsTr("文本文件 (*.txt)"),
            qsTr("C/C++文件 (*.cpp *.h *.hpp *.c *.cc *.cxx *.hxx)"),
            qsTr("QML/JavaScript文件 (*.qml *.js)"),
            qsTr("标记语言文件 (*.json *.xml *.html)"),
            qsTr("所有文件 (*.*)")
        ]
        
        onAccepted: {
            const filePath = fileDialog.file.toString()
            const cleanPath = filePath.replace(/^(file:\/{2,3})/, "")
            Logger.info("文件选择完成: " + cleanPath)
            root.fileSelected(cleanPath)
        }
        
        onRejected: {
            Logger.debug("文件选择已取消")
        }
    }
    
    // 另存为对话框
    Platform.FileDialog {
        id: saveAsDialog
        title: qsTr("另存为")
        folder: Platform.StandardPaths.writableLocation(Platform.StandardPaths.DocumentsLocation)
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: [
            qsTr("所有支持的文件 (*.txt *.cpp *.h *.hpp *.c *.cc *.cxx *.hxx *.qml *.js "
                + "*.json *.xml *.html *.css *.py *.java *.cs *.php *.rb *.go *.rs *.md *.log *.ini *.cfg *.conf)"),
            qsTr("文本文件 (*.txt)"),
            qsTr("C/C++文件 (*.cpp *.h *.hpp *.c *.cc *.cxx *.hxx)"),
            qsTr("QML/JavaScript文件 (*.qml *.js)"),
            qsTr("标记语言文件 (*.json *.xml *.html)"),
            qsTr("所有文件 (*.*)")
        ]
        
        onAccepted: {
            const filePath = saveAsDialog.file.toString()
            const cleanPath = filePath.replace(/^(file:\/{2,3})/, "")
            Logger.info("另存为路径选择完成: " + cleanPath)
            root.saveAsSelected(cleanPath)
        }
        
        onRejected: {
            Logger.debug("另存为已取消")
        }
    }
}