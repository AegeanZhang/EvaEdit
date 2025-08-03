#ifndef __APP_CONSTANTS_H__
#define __APP_CONSTANTS_H__

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QJSEngine> 

class AppConstants : public QObject
{
    Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        // 字符串常量
        Q_PROPERTY(QString newFile READ newFile CONSTANT)
        Q_PROPERTY(QString openFile READ openFile CONSTANT)
        Q_PROPERTY(QString saveFile READ saveFile CONSTANT)
        Q_PROPERTY(QString saveFileAs READ saveFileAs CONSTANT)
        Q_PROPERTY(QString openDirectory READ openDirectory CONSTANT)
        Q_PROPERTY(QString exit READ exit CONSTANT)

        // 菜单相关字符串
        Q_PROPERTY(QString fileMenu READ fileMenu CONSTANT)
        Q_PROPERTY(QString editMenu READ editMenu CONSTANT)
        Q_PROPERTY(QString viewMenu READ viewMenu CONSTANT)
        Q_PROPERTY(QString helpMenu READ helpMenu CONSTANT)

        // 编辑相关字符串
        Q_PROPERTY(QString undo READ undo CONSTANT)
        Q_PROPERTY(QString redo READ redo CONSTANT)
        Q_PROPERTY(QString cut READ cut CONSTANT)
        Q_PROPERTY(QString copy READ copy CONSTANT)
        Q_PROPERTY(QString paste READ paste CONSTANT)

        // 整型常量示例
        Q_PROPERTY(int defaultFontSize READ defaultFontSize CONSTANT)
        Q_PROPERTY(int defaultTabSize READ defaultTabSize CONSTANT)
        Q_PROPERTY(int maxRecentFiles READ maxRecentFiles CONSTANT)

        // 浮点常量示例
        Q_PROPERTY(qreal defaultOpacity READ defaultOpacity CONSTANT)

public:
    static AppConstants* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        static AppConstants instance;
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }

    // 字符串常量访问器
    QString newFile() const { return QStringLiteral("新建文件(&N)"); }
    QString openFile() const { return QStringLiteral("打开(&O)"); }
    QString saveFile() const { return QStringLiteral("保存(&S)"); }
    QString saveFileAs() const { return QStringLiteral("另存为(&A)"); }
    QString openDirectory() const { return QStringLiteral("打开目录(&D)"); }
    QString exit() const { return QStringLiteral("退出(&Q)"); }

    // 菜单相关字符串访问器
    QString fileMenu() const { return QStringLiteral("文件(&F)"); }
    QString editMenu() const { return QStringLiteral("编辑(&E)"); }
    QString viewMenu() const { return QStringLiteral("视图(&V)"); }
    QString helpMenu() const { return QStringLiteral("帮助(&H)"); }

    // 编辑相关字符串访问器
    QString undo() const { return QStringLiteral("撤销(&U)"); }
    QString redo() const { return QStringLiteral("重做(&R)"); }
    QString cut() const { return QStringLiteral("剪切(&T)"); }
    QString copy() const { return QStringLiteral("复制(&C)"); }
    QString paste() const { return QStringLiteral("粘贴(&P)"); }

    // 整型常量访问器
    int defaultFontSize() const { return 14; }
    int defaultTabSize() const { return 4; }
    int maxRecentFiles() const { return 10; }

    // 浮点常量访问器
    qreal defaultOpacity() const { return 0.8; }

private:
    AppConstants() = default;
    ~AppConstants() = default;
};

#endif // __APP_CONSTANTS_H__