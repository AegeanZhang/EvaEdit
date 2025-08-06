#include "TabController.h"
#include <QFileInfo>

TabController* TabController::m_instance = nullptr;
// 【新增】新建文件URL方案
const QString TabController::NEW_FILE_SCHEME = "new://"; 

TabController* TabController::instance()
{
    if (!m_instance)
        m_instance = new TabController();
    return m_instance;
}

TabController::TabController(QObject* parent) 
    : QObject(parent)
{
}

QString TabController::currentFilePath() const
{
    if (isValidTabIndex(m_currentTabIndex)) {
        return m_openFiles[m_currentTabIndex];
    }
    return QString();
}

void TabController::setCurrentTabIndex(int index)
{
    if (m_currentTabIndex != index && isValidTabIndex(index)) {
        m_currentTabIndex = index;
        emit currentTabIndexChanged();
        emit currentFilePathChanged();
        requestFocusForCurrentTab();
    }
}

int TabController::addNewTab(const QString& filePath)
{
    // 【修改】增强对新建文件URL的检查，不仅检查空字符串
    int existingIndex = findTabByFilePath(filePath);
    if (existingIndex != -1 && !isNewFile(filePath)) {
        // 【修改】对于新建文件，即使URL相同也创建新的标签（因为它们是不同的未命名文件）
        setCurrentTabIndex(existingIndex);
        return existingIndex;
    }

    // 添加新标签
    m_openFiles.append(filePath);
    int newIndex = m_openFiles.size() - 1;

    emit openFilesChanged();
    emit tabCountChanged();
    emit tabAdded(newIndex, filePath);

    // 切换到新标签
    setCurrentTabIndex(newIndex);

    return newIndex;
}

// 【新增】创建新文件的便捷方法
int TabController::addNewTab()
{
    QString newFileUrl = generateNewFileUrl();
    return addNewTab(newFileUrl);
}

bool TabController::closeTab(int index)
{
    if (!isValidTabIndex(index)) {
        return false;
    }

    m_openFiles.removeAt(index);
    emit openFilesChanged();
    emit tabCountChanged();
    emit tabClosed(index);

    // 如果没有标签页，创建一个新的空白页
    if (m_openFiles.isEmpty()) {
        //addNewTab(); // 【修改】使用新的方法创建新文件
        return true;
    }

    // 调整当前索引
    if (m_currentTabIndex >= m_openFiles.size()) {
        setCurrentTabIndex(m_openFiles.size() - 1);
    }
    else if (m_currentTabIndex == index) {
        // 如果关闭的是当前标签，触发焦点设置
        requestFocusForCurrentTab();
    }

    return true;
}

QString TabController::getTabFileName(int index) const
{
    if (!isValidTabIndex(index)) {
        return QString();
    }

    const QString& filePath = m_openFiles[index];
    // 【修改】使用新的显示名称逻辑
    return getDisplayName(filePath);
}

QString TabController::getTabFilePath(int index) const
{
    if (!isValidTabIndex(index)) {
        return QString();
    }
    return m_openFiles[index];
}

int TabController::findTabByFilePath(const QString& filePath) const
{
    return m_openFiles.indexOf(filePath);
}

bool TabController::isValidTabIndex(int index) const
{
    return index >= 0 && index < m_openFiles.size();
}

// 【新增】判断是否为新建文件（通过文件路径）
bool TabController::isNewFile(const QString& filePath) const
{
    return filePath.startsWith(NEW_FILE_SCHEME) || filePath.isEmpty(); // 兼容空字符串
}

// 【新增】判断是否为新建文件（通过索引）
bool TabController::isNewFile(int index) const
{
    if (!isValidTabIndex(index)) {
        return false;
    }
    return isNewFile(m_openFiles[index]);
}

// 【新增】获取文件显示名称
QString TabController::getDisplayName(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        // 兼容性处理：空字符串仍然显示为新建文件
        return generateNewTabName();
    }

    if (isNewFile(filePath)) {
        // 从URL中提取文件名
        return extractNewFileName(filePath);
    }

    // 普通文件，返回文件名
    return QFileInfo(filePath).fileName();
}

// 【新增】另存为功能 - 将新建文件URL转换为真实文件路径
bool TabController::saveFileAs(int index, const QString& newFilePath)
{
    if (!isValidTabIndex(index) || newFilePath.isEmpty()) {
        return false;
    }

    const QString& oldPath = m_openFiles[index];

    // 检查新文件路径是否已被其他标签使用
    int existingIndex = findTabByFilePath(newFilePath);
    if (existingIndex != -1 && existingIndex != index) {
        // 新文件路径已被其他标签使用，不允许保存
        return false;
    }

    // 更新文件路径
    m_openFiles[index] = newFilePath;

    // 发出信号通知文件路径已更新
    emit filePathUpdated(index, oldPath, newFilePath);
    emit openFilesChanged();
    emit currentFilePathChanged();

    return true;
}

void TabController::requestFocusForCurrentTab()
{
    if (isValidTabIndex(m_currentTabIndex)) {
        emit focusRequested(m_currentTabIndex);
    }
}

QString TabController::generateNewTabName() const
{
    return QString("新标签页 %1").arg(m_openFiles.size());
}

// 【新增】生成新文件URL
QString TabController::generateNewFileUrl() const
{
    QString fileName = QString("新建文件%1").arg(m_newFileCounter);
    // 【注意】这里修改了const函数中修改成员变量的问题
    const_cast<TabController*>(this)->m_newFileCounter++;
    return NEW_FILE_SCHEME + fileName;
}

// 【新增】从新建文件URL中提取文件名
QString TabController::extractNewFileName(const QString& url) const
{
    if (url.startsWith(NEW_FILE_SCHEME)) {
        return url.mid(NEW_FILE_SCHEME.length());
    }
    return url;
}