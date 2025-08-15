#ifndef __TAB_CONTROLLER_H__
#define __TAB_CONTROLLER_H__

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qqmlengine.h>

class TabController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // 属性
    Q_PROPERTY(QStringList openFiles READ openFiles NOTIFY openFilesChanged)
    Q_PROPERTY(int currentTabIndex READ currentTabIndex WRITE setCurrentTabIndex NOTIFY currentTabIndexChanged)
    Q_PROPERTY(QString currentFilePath READ currentFilePath NOTIFY currentFilePathChanged)
    Q_PROPERTY(int tabCount READ tabCount NOTIFY tabCountChanged)

public:
    static TabController* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return instance();
    }

    static TabController* instance();

    // 只读属性访问器
    QStringList openFiles() const { return m_openFiles; }
    int currentTabIndex() const { return m_currentTabIndex; }
    QString currentFilePath() const;
    int tabCount() const { return m_openFiles.size(); }

    // 写属性访问器
    void setCurrentTabIndex(int index);

    // 业务逻辑方法
    Q_INVOKABLE int addNewTab(const QString& filePath);
    Q_INVOKABLE int addNewTab(); // 【新增】创建新文件的便捷方法
    Q_INVOKABLE bool closeTab(int index);
    Q_INVOKABLE QString getTabFileName(int index) const;
    Q_INVOKABLE QString getTabFilePath(int index) const;
    Q_INVOKABLE int findTabByFilePath(const QString& filePath) const;
    Q_INVOKABLE bool isValidTabIndex(int index) const;

    // 【新增】新建文件相关方法
    Q_INVOKABLE bool isNewFile(const QString& filePath) const;
    Q_INVOKABLE bool isNewFile(int index) const;
    Q_INVOKABLE QString getDisplayName(const QString& filePath) const;
    Q_INVOKABLE bool saveFileAs(int index, const QString& newFilePath); // 【新增】另存为功能

    // 焦点管理
    Q_INVOKABLE void requestFocusForCurrentTab();

signals:
    void openFilesChanged();
    void currentTabIndexChanged();
    void currentFilePathChanged();
    void tabCountChanged();
    void tabAdded(int index, const QString& filePath);
    void tabClosed(int index);
    void focusRequested(int tabIndex);
    // 【新增】文件路径更新信号
    void filePathUpdated(int index, const QString& oldPath, const QString& newPath);

private:
    explicit TabController(QObject* parent = nullptr);
    QString generateNewTabName() const;
    // 【新增】生成新文件URL
    QString generateNewFileUrl() const;
    // 【新增】从URL提取文件名
    QString extractNewFileName(const QString& url) const;

    static TabController* m_instance;
    QStringList m_openFiles;
    int m_currentTabIndex = -1;

    // 【新增】新建文件相关成员变量
    int m_newFileCounter = 1;
    static const QString NEW_FILE_SCHEME; // "new://"
};

#endif // __TAB_CONTROLLER_H__