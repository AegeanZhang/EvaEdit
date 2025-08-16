#include "CursorManager.h"
#include <QGuiApplication>
#include <QStyleHints>
#include <QDebug>
#include <algorithm>

CursorManager::CursorManager(QObject* parent)
    : QObject(parent)
    , m_blinkTimer(new QTimer(this))
{
    // 初始化主光标
    Cursor primaryCursor;
    primaryCursor.position = 0;
    primaryCursor.anchorPosition = 0;
    m_cursors.append(primaryCursor);

    // 设置闪烁定时器
    m_blinkTimer->setSingleShot(false);
    m_blinkTimer->setInterval(m_blinkInterval);
    connect(m_blinkTimer, &QTimer::timeout, this, &CursorManager::onBlinkTimer);

    // 启动闪烁
    startBlinking();
}

// ==============================================================================
// 单光标操作
// ==============================================================================

void CursorManager::setCursorPosition(int position, bool select)
{
    position = qMax(0, position);

    if (m_cursors.isEmpty()) {
        Cursor cursor;
        cursor.position = position;
        cursor.anchorPosition = select ? 0 : position;
        m_cursors.append(cursor);
        m_primaryCursorIndex = 0;
    }
    else {
        Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];

        if (!select) {
            // 不选择时，移动锚点到新位置
            primaryCursor.anchorPosition = position;
        }

        primaryCursor.position = position;
    }

    // 重新启动闪烁
    setBlink(true);
    m_blinkTimer->start();

    emit cursorPositionChanged(position);

    // 如果选择状态改变，发送选择信号
    if (hasSelection()) {
        emit selectionChanged(selection());
    }
    else {
        emit selectionChanged(Selection(position, position));
    }

    emit cursorsChanged(m_cursors);
    ensureCursorVisible();
}

int CursorManager::cursorPosition() const
{
    if (m_cursors.isEmpty())
        return 0;

    return m_cursors[m_primaryCursorIndex].position;
}

void CursorManager::setAnchorPosition(int position)
{
    position = qMax(0, position);

    if (!m_cursors.isEmpty()) {
        m_cursors[m_primaryCursorIndex].anchorPosition = position;

        if (hasSelection()) {
            emit selectionChanged(selection());
        }

        emit cursorsChanged(m_cursors);
    }
}

int CursorManager::anchorPosition() const
{
    if (m_cursors.isEmpty())
        return 0;

    return m_cursors[m_primaryCursorIndex].anchorPosition;
}

// ==============================================================================
// 选择操作
// ==============================================================================

void CursorManager::setSelection(int start, int end)
{
    start = qMax(0, start);
    end = qMax(0, end);

    if (start > end) {
        std::swap(start, end);
    }

    if (m_cursors.isEmpty()) {
        Cursor cursor;
        cursor.position = end;
        cursor.anchorPosition = start;
        m_cursors.append(cursor);
        m_primaryCursorIndex = 0;
    }
    else {
        Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];
        primaryCursor.position = end;
        primaryCursor.anchorPosition = start;
    }

    emit selectionChanged(Selection(start, end));
    emit cursorPositionChanged(end);
    emit cursorsChanged(m_cursors);
    ensureCursorVisible();
}

void CursorManager::selectAll(int textLength)
{
    textLength = qMax(0, textLength);
    setSelection(0, textLength);
}

void CursorManager::clearSelection()
{
    if (!hasSelection())
        return;

    if (!m_cursors.isEmpty()) {
        Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];
        primaryCursor.anchorPosition = primaryCursor.position;

        emit selectionChanged(Selection(primaryCursor.position, primaryCursor.position));
        emit cursorsChanged(m_cursors);
    }
}

bool CursorManager::hasSelection() const
{
    if (m_cursors.isEmpty())
        return false;

    return m_cursors[m_primaryCursorIndex].hasSelection();
}

Selection CursorManager::selection() const
{
    if (m_cursors.isEmpty())
        return Selection(0, 0);

    const Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];
    return Selection(primaryCursor.selectionStart(), primaryCursor.selectionEnd());
}

// ==============================================================================
// 多光标操作
// ==============================================================================

void CursorManager::addCursor(int position)
{
    if (!m_multiCursorMode)
        return;

    position = qMax(0, position);

    // 检查是否已存在相同位置的光标
    for (const Cursor& cursor : m_cursors) {
        if (cursor.position == position) {
            return; // 已存在，不添加
        }
    }

    Cursor newCursor;
    newCursor.position = position;
    newCursor.anchorPosition = position;

    m_cursors.append(newCursor);
    sortCursors();

    emit cursorsChanged(m_cursors);
}

void CursorManager::removeCursor(int index)
{
    if (index < 0 || index >= m_cursors.size() || m_cursors.size() <= 1)
        return;

    // 不能删除主光标，如果要删除主光标，先换主光标
    if (index == m_primaryCursorIndex) {
        m_primaryCursorIndex = (index == 0) ? 1 : 0;
    }
    else if (index < m_primaryCursorIndex) {
        m_primaryCursorIndex--;
    }

    m_cursors.removeAt(index);
    emit cursorsChanged(m_cursors);
}

void CursorManager::clearAllCursors()
{
    if (m_cursors.size() <= 1)
        return;

    // 保留主光标
    Cursor primaryCursor = m_cursors[m_primaryCursorIndex];
    m_cursors.clear();
    m_cursors.append(primaryCursor);
    m_primaryCursorIndex = 0;

    emit cursorsChanged(m_cursors);
}

QList<Cursor> CursorManager::cursors() const
{
    return m_cursors;
}

int CursorManager::cursorCount() const
{
    return m_cursors.size();
}

void CursorManager::setMultiCursorMode(bool enabled)
{
    if (m_multiCursorMode == enabled)
        return;

    m_multiCursorMode = enabled;

    if (!enabled) {
        // 禁用多光标模式时，只保留主光标
        clearAllCursors();
    }
}

bool CursorManager::isMultiCursorMode() const
{
    return m_multiCursorMode;
}

// ==============================================================================
// 光标移动
// ==============================================================================

void CursorManager::moveCursor(int delta, bool select)
{
    if (m_cursors.isEmpty())
        return;

    if (m_multiCursorMode) {
        // 多光标模式：移动所有光标
        for (Cursor& cursor : m_cursors) {
            int newPosition = qMax(0, cursor.position + delta);

            if (!select) {
                cursor.anchorPosition = newPosition;
            }
            cursor.position = newPosition;
        }

        mergeCursors();
        updatePrimaryCursor();
    }
    else {
        // 单光标模式：只移动主光标
        Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];
        int newPosition = qMax(0, primaryCursor.position + delta);

        if (!select) {
            primaryCursor.anchorPosition = newPosition;
        }
        primaryCursor.position = newPosition;
    }

    // 重新启动闪烁
    setBlink(true);
    m_blinkTimer->start();

    emit cursorPositionChanged(cursorPosition());
    emit cursorsChanged(m_cursors);

    if (hasSelection()) {
        emit selectionChanged(selection());
    }

    ensureCursorVisible();
}

void CursorManager::moveCursorToLine(int line, bool select)
{
    // 这个方法需要与 DocumentModel 配合使用
    // 这里提供基础实现，具体的行列转换需要在更高层处理
    Q_UNUSED(line)
        Q_UNUSED(select)

        qWarning() << "moveCursorToLine 需要 DocumentModel 支持进行位置转换";
}

void CursorManager::moveCursorToColumn(int column, bool select)
{
    // 同样需要 DocumentModel 支持
    Q_UNUSED(column)
        Q_UNUSED(select)

        qWarning() << "moveCursorToColumn 需要 DocumentModel 支持进行位置转换";
}

void CursorManager::moveCursorToLineColumn(int line, int column, bool select)
{
    // 需要 DocumentModel 支持
    Q_UNUSED(line)
        Q_UNUSED(column)
        Q_UNUSED(select)

        qWarning() << "moveCursorToLineColumn 需要 DocumentModel 支持进行位置转换";
}

// ==============================================================================
// 可见性管理
// ==============================================================================

void CursorManager::ensureCursorVisible()
{
    if (!m_cursorRect.isEmpty()) {
        emit ensureVisibleRequested(m_cursorRect);
    }
}

QRect CursorManager::cursorRect() const
{
    return m_cursorRect;
}

void CursorManager::setCursorRect(const QRect& rect)
{
    if (m_cursorRect == rect)
        return;

    m_cursorRect = rect;
    ensureCursorVisible();
}

// ==============================================================================
// 闪烁控制
// ==============================================================================

void CursorManager::startBlinking()
{
    if (!m_blinkTimer->isActive()) {
        setBlink(true);
        m_blinkTimer->start();
    }
}

void CursorManager::stopBlinking()
{
    if (m_blinkTimer->isActive()) {
        m_blinkTimer->stop();
        setBlink(false);
    }
}

void CursorManager::setBlink(bool visible)
{
    if (m_blinkVisible == visible)
        return;

    m_blinkVisible = visible;
    emit cursorVisibilityChanged(m_blinkVisible);
}

bool CursorManager::isBlinkVisible() const
{
    return m_blinkVisible;
}

void CursorManager::setBlinkInterval(int milliseconds)
{
    milliseconds = qMax(100, milliseconds); // 最小100ms

    if (m_blinkInterval == milliseconds)
        return;

    m_blinkInterval = milliseconds;
    m_blinkTimer->setInterval(m_blinkInterval);
}

int CursorManager::blinkInterval() const
{
    return m_blinkInterval;
}

// ==============================================================================
// 私有槽函数
// ==============================================================================

void CursorManager::onBlinkTimer()
{
    setBlink(!m_blinkVisible);
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

void CursorManager::updatePrimaryCursor()
{
    if (m_cursors.isEmpty())
        return;

    // 确保主光标索引有效
    m_primaryCursorIndex = qBound(0, m_primaryCursorIndex, m_cursors.size() - 1);

    emit cursorPositionChanged(cursorPosition());
}

void CursorManager::mergeCursors()
{
    if (m_cursors.size() <= 1)
        return;

    // 合并重叠或相邻的光标
    QList<Cursor> mergedCursors;

    // 先排序
    sortCursors();

    for (const Cursor& cursor : m_cursors) {
        bool merged = false;

        // 检查是否可以与最后一个光标合并
        if (!mergedCursors.isEmpty()) {
            Cursor& lastCursor = mergedCursors.last();

            // 如果位置相同或选择区域重叠，合并
            if (cursor.position == lastCursor.position ||
                (cursor.hasSelection() && lastCursor.hasSelection() &&
                    cursor.selectionStart() <= lastCursor.selectionEnd() &&
                    cursor.selectionEnd() >= lastCursor.selectionStart())) {

                // 合并选择区域
                int newStart = qMin(cursor.selectionStart(), lastCursor.selectionStart());
                int newEnd = qMax(cursor.selectionEnd(), lastCursor.selectionEnd());

                lastCursor.anchorPosition = newStart;
                lastCursor.position = newEnd;
                merged = true;
            }
        }

        if (!merged) {
            mergedCursors.append(cursor);
        }
    }

    // 更新光标列表
    if (mergedCursors.size() != m_cursors.size()) {
        m_cursors = mergedCursors;

        // 更新主光标索引
        m_primaryCursorIndex = qBound(0, m_primaryCursorIndex, m_cursors.size() - 1);

        emit cursorsChanged(m_cursors);
    }
}

void CursorManager::sortCursors()
{
    if (m_cursors.size() <= 1)
        return;

    // 记住主光标
    Cursor primaryCursor = m_cursors[m_primaryCursorIndex];

    // 按位置排序
    std::sort(m_cursors.begin(), m_cursors.end(),
        [](const Cursor& a, const Cursor& b) {
            return a.position < b.position;
        });

    // 找到主光标的新位置
    for (int i = 0; i < m_cursors.size(); ++i) {
        if (m_cursors[i].position == primaryCursor.position &&
            m_cursors[i].anchorPosition == primaryCursor.anchorPosition) {
            m_primaryCursorIndex = i;
            break;
        }
    }
}

// ==============================================================================
// 额外的便利方法
// ==============================================================================

// 获取所有选择区域
QList<Selection> CursorManager::getAllSelections() const
{
    QList<Selection> selections;

    for (const Cursor& cursor : m_cursors) {
        if (cursor.hasSelection()) {
            selections.append(Selection(cursor.selectionStart(), cursor.selectionEnd()));
        }
    }

    return selections;
}

// 获取所有光标位置
QList<int> CursorManager::getAllPositions() const
{
    QList<int> positions;

    for (const Cursor& cursor : m_cursors) {
        positions.append(cursor.position);
    }

    return positions;
}

// 检查位置是否在任何选择区域内
bool CursorManager::isPositionSelected(int position) const
{
    for (const Cursor& cursor : m_cursors) {
        if (cursor.hasSelection() &&
            position >= cursor.selectionStart() &&
            position < cursor.selectionEnd()) {
            return true;
        }
    }

    return false;
}

// 设置系统光标闪烁间隔
void CursorManager::setSystemBlinkInterval()
{
    // 获取系统光标闪烁间隔 - 仅使用QGuiApplication
    int systemInterval = 530; // 默认值

    // 确保我们有QGuiApplication实例，并且它有styleHints方法
    QGuiApplication* app = qobject_cast<QGuiApplication*>(QGuiApplication::instance());
    if (app && app->styleHints()) {
        systemInterval = app->styleHints()->cursorFlashTime();
    }

    if (systemInterval > 0) {
        setBlinkInterval(systemInterval / 2); // 返回的是完整周期，我们需要半周期
    }
    else {
        setBlinkInterval(530); // 使用默认值
    }
}

// 暂停闪烁（用于输入时）
void CursorManager::pauseBlinking()
{
    if (m_blinkTimer->isActive()) {
        m_blinkTimer->stop();
        setBlink(true); // 确保光标可见
    }
}

// 恢复闪烁
void CursorManager::resumeBlinking()
{
    setBlink(true);
    m_blinkTimer->start();
}

// 重置闪烁状态（用于键盘输入后）
void CursorManager::resetBlink()
{
    setBlink(true);
    if (m_blinkTimer->isActive()) {
        m_blinkTimer->stop();
        m_blinkTimer->start();
    }
}

// 获取主光标的选择范围
Selection CursorManager::getPrimarySelection() const
{
    if (m_cursors.isEmpty())
        return Selection(0, 0);

    const Cursor& primaryCursor = m_cursors[m_primaryCursorIndex];
    return Selection(primaryCursor.selectionStart(), primaryCursor.selectionEnd());
}

// 设置光标样式相关的属性
void CursorManager::setCursorWidth(int width)
{
    m_cursorWidth = qMax(1, width);
}

void CursorManager::setCursorColor(const QColor& color)
{
    m_cursorColor = color;
}

// 调试辅助方法
QString CursorManager::debugString() const
{
    QStringList cursorStrings;

    for (int i = 0; i < m_cursors.size(); ++i) {
        const Cursor& cursor = m_cursors[i];
        QString cursorStr = QString("Cursor[%1]: pos=%2, anchor=%3")
            .arg(i)
            .arg(cursor.position)
            .arg(cursor.anchorPosition);

        if (i == m_primaryCursorIndex) {
            cursorStr += " (PRIMARY)";
        }

        if (cursor.hasSelection()) {
            cursorStr += QString(" selection=[%1,%2]")
                .arg(cursor.selectionStart())
                .arg(cursor.selectionEnd());
        }

        cursorStrings.append(cursorStr);
    }

    return QString("CursorManager: multiMode=%1, blinking=%2\n%3")
        .arg(m_multiCursorMode)
        .arg(m_blinkTimer->isActive())
        .arg(cursorStrings.join("\n"));
}