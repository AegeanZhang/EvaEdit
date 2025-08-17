#include "LayoutEngine.h"
#include "SyntaxHighlighter.h"
#include <QTextOption>
#include <QTextLine>
#include <QDebug>
#include <cmath>
#include <algorithm>

LayoutEngine::LayoutEngine(QObject* parent)
    : QObject(parent)
    , m_font("Consolas")//, 12)
    , m_fontMetrics(m_font)
{
    m_font.setPixelSize(12);
    // 重新初始化字体度量（因为字体已经修改）
    m_fontMetrics = QFontMetrics(m_font);

    // 初始化视口信息
    m_viewport.rect = QRectF();
    m_viewport.firstVisibleLine = 0;
    m_viewport.lastVisibleLine = 0;
    m_viewport.scrollX = 0;
    m_viewport.scrollY = 0;
}

LayoutEngine::~LayoutEngine()
{
    clearLineLayouts();
}

// ==============================================================================
// 字体和度量
// ==============================================================================

void LayoutEngine::setFont(const QFont& font)
{
    if (m_font == font)
        return;

    m_font = font;
    m_fontMetrics = QFontMetrics(m_font);

    // 字体变化时，所有布局都需要重新计算
    invalidateAllLayouts();

    emit layoutChanged();
}

QFont LayoutEngine::font() const
{
    return m_font;
}

QFontMetrics LayoutEngine::fontMetrics() const
{
    return m_fontMetrics;
}

qreal LayoutEngine::lineHeight() const
{
    return m_fontMetrics.lineSpacing();
}

qreal LayoutEngine::characterWidth() const
{
    // Qt 6 中 averageCharacterWidth() 被移除，使用 'M' 字符的宽度作为替代
    return m_fontMetrics.horizontalAdvance('M');
}

qreal LayoutEngine::tabWidth() const
{
    return m_tabWidth * characterWidth();
}

void LayoutEngine::setTabWidth(int characters)
{
    if (m_tabWidth == characters)
        return;

    m_tabWidth = qMax(1, characters);

    // Tab宽度变化时，需要重新布局
    invalidateAllLayouts();
    emit layoutChanged();
}

// ==============================================================================
// 文本包装
// ==============================================================================

void LayoutEngine::setTextWidth(qreal width)
{
    if (qFuzzyCompare(m_textWidth, width))
        return;

    m_textWidth = width;

    // 文本宽度变化时，如果启用了自动换行，需要重新布局
    if (m_wordWrap) {
        invalidateAllLayouts();
        emit layoutChanged();
    }
}

qreal LayoutEngine::textWidth() const
{
    return m_textWidth;
}

void LayoutEngine::setWordWrap(bool enabled)
{
    if (m_wordWrap == enabled)
        return;

    m_wordWrap = enabled;

    // 自动换行状态变化时，需要重新布局
    invalidateAllLayouts();
    emit layoutChanged();
}

bool LayoutEngine::wordWrap() const
{
    return m_wordWrap;
}

// ==============================================================================
// 布局管理
// ==============================================================================

void LayoutEngine::setText(const QString& text)
{
    if (m_text == text)
        return;

    m_text = text;

    // 清除所有现有布局
    clearLineLayouts();

    // 重新创建行布局结构
    QStringList lines = splitIntoLines(m_text);

    for (int i = 0; i < lines.size(); ++i) {
        LineLayout* lineLayout = new LineLayout;
        lineLayout->lineNumber = i;
        lineLayout->layout = nullptr;
        lineLayout->width = 0;
        lineLayout->height = lineHeight();
        lineLayout->visible = false;
        lineLayout->dirty = true;

        m_lineLayouts.append(lineLayout);
    }

    emit layoutChanged();
}

void LayoutEngine::updateText(int position, int removedLength, const QString& addedText)
{
    // 更新文本内容
    m_text.remove(position, removedLength);
    m_text.insert(position, addedText);

    // 重新分析行结构
    QStringList newLines = splitIntoLines(m_text);

    // 找到变更影响的行范围
    int startLine = positionToLine(position);
    int endLine = startLine;

    // 计算影响的行数
    int removedLines = 0;
    for (int i = position; i < position + removedLength && i < m_text.length() + removedLength; ++i) {
        if (m_text.at(qMin(i, m_text.length() - 1)) == '\n') {
            removedLines++;
        }
    }

    int addedLines = addedText.count('\n');
    int netLineChange = addedLines - removedLines;

    // 调整行布局结构
    if (netLineChange > 0) {
        // 添加了行
        for (int i = 0; i < netLineChange; ++i) {
            LineLayout* lineLayout = new LineLayout;
            lineLayout->lineNumber = startLine + i + 1;
            lineLayout->layout = nullptr;
            lineLayout->width = 0;
            lineLayout->height = lineHeight();
            lineLayout->visible = false;
            lineLayout->dirty = true;

            m_lineLayouts.insert(startLine + i + 1, lineLayout);
        }

        // 更新后续行的行号
        for (int i = startLine + netLineChange + 1; i < m_lineLayouts.size(); ++i) {
            m_lineLayouts[i]->lineNumber = i;
        }

    }
    else if (netLineChange < 0) {
        // 删除了行
        for (int i = 0; i < -netLineChange && startLine + 1 < m_lineLayouts.size(); ++i) {
            delete m_lineLayouts[startLine + 1];
            m_lineLayouts.removeAt(startLine + 1);
        }

        // 更新后续行的行号
        for (int i = startLine + 1; i < m_lineLayouts.size(); ++i) {
            m_lineLayouts[i]->lineNumber = i;
        }
    }

    // 标记受影响的行为脏
    endLine = qMin(startLine + qMax(1, qAbs(netLineChange)), m_lineLayouts.size() - 1);
    for (int i = startLine; i <= endLine; ++i) {
        if (i < m_lineLayouts.size()) {
            invalidateLineLayout(i);
        }
    }

    emit layoutChanged();
}

LineLayout* LayoutEngine::getLineLayout(int lineNumber)
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return nullptr;

    LineLayout* lineLayout = m_lineLayouts[lineNumber];

    // 如果布局是脏的，重新创建
    if (lineLayout->dirty || !lineLayout->layout) {
        createLineLayout(lineNumber);
    }

    return lineLayout;
}

void LayoutEngine::invalidateLineLayout(int lineNumber)
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return;

    LineLayout* lineLayout = m_lineLayouts[lineNumber];
    lineLayout->dirty = true;

    if (lineLayout->layout) {
        delete lineLayout->layout;
        lineLayout->layout = nullptr;
    }

    emit lineLayoutUpdated(lineNumber);
}

void LayoutEngine::invalidateAllLayouts()
{
    for (LineLayout* lineLayout : m_lineLayouts) {
        lineLayout->dirty = true;
        if (lineLayout->layout) {
            delete lineLayout->layout;
            lineLayout->layout = nullptr;
        }
    }
}

// ==============================================================================
// 视口管理
// ==============================================================================

void LayoutEngine::setViewport(const ViewportInfo& viewport)
{
    m_viewport = viewport;

    // 更新可见行信息
    updateVisibleLines();

    emit viewportChanged();
}

ViewportInfo LayoutEngine::viewport() const
{
    return m_viewport;
}

void LayoutEngine::updateVisibleLines()
{
    if (m_lineLayouts.isEmpty()) {
        m_viewport.firstVisibleLine = 0;
        m_viewport.lastVisibleLine = 0;
        return;
    }

    qreal currentY = -m_viewport.scrollY;
    qreal viewportBottom = m_viewport.rect.height() - m_viewport.scrollY;

    m_viewport.firstVisibleLine = 0;
    m_viewport.lastVisibleLine = m_lineLayouts.size() - 1;

    // 找到第一个可见行
    for (int i = 0; i < m_lineLayouts.size(); ++i) {
        LineLayout* lineLayout = m_lineLayouts[i];
        if (currentY + lineLayout->height > 0) {
            m_viewport.firstVisibleLine = i;
            break;
        }
        currentY += lineLayout->height;
    }

    // 找到最后一个可见行
    currentY = -m_viewport.scrollY;
    for (int i = 0; i < m_lineLayouts.size(); ++i) {
        if (currentY > viewportBottom) {
            m_viewport.lastVisibleLine = qMax(0, i - 1);
            break;
        }

        LineLayout* lineLayout = m_lineLayouts[i];
        currentY += lineLayout->height;

        if (i == m_lineLayouts.size() - 1) {
            m_viewport.lastVisibleLine = i;
        }
    }
}

// ==============================================================================
// 坐标转换
// ==============================================================================

QPointF LayoutEngine::positionToPoint(int position) const
{
    if (m_lineLayouts.isEmpty())
        return QPointF(0, 0);

    // 找到位置所在的行
    int lineNumber = positionToLine(position);
    int columnInLine = positionToColumn(position, lineNumber);

    // 计算 Y 坐标
    qreal y = 0;
    for (int i = 0; i < lineNumber && i < m_lineLayouts.size(); ++i) {
        y += m_lineLayouts[i]->height;
    }

    // 计算 X 坐标
    qreal x = 0;
    if (lineNumber < m_lineLayouts.size()) {
        QString lineText = getLineText(lineNumber);
        QString textBeforeCursor = lineText.left(columnInLine);

        // 处理制表符
        QString expandedText = expandTabs(textBeforeCursor);
        x = m_fontMetrics.horizontalAdvance(expandedText);
    }

    return QPointF(x, y);
}

int LayoutEngine::pointToPosition(const QPointF& point) const
{
    if (m_lineLayouts.isEmpty())
        return 0;

    // 找到点击的行
    qreal currentY = 0;
    int lineNumber = 0;

    for (int i = 0; i < m_lineLayouts.size(); ++i) {
        qreal lineBottom = currentY + m_lineLayouts[i]->height;
        if (point.y() < lineBottom) {
            lineNumber = i;
            break;
        }
        currentY = lineBottom;
        lineNumber = i;
    }

    // 在行内找到列位置
    QString lineText = getLineText(lineNumber);
    qreal targetX = point.x();
    qreal currentX = 0;
    int column = 0;

    for (int i = 0; i < lineText.length(); ++i) {
        QChar ch = lineText.at(i);
        qreal charWidth;

        if (ch == '\t') {
            // 计算制表符宽度
            qreal nextTabStop = std::ceil((currentX + 1) / tabWidth()) * tabWidth();
            charWidth = nextTabStop - currentX;
        }
        else {
            charWidth = m_fontMetrics.horizontalAdvance(ch);
        }

        if (currentX + charWidth / 2 > targetX) {
            break;
        }

        currentX += charWidth;
        column = i + 1;
    }

    return lineColumnToPosition(lineNumber, column);
}

QRectF LayoutEngine::lineRect(int lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return QRectF();

    qreal y = 0;
    for (int i = 0; i < lineNumber; ++i) {
        y += m_lineLayouts[i]->height;
    }

    LineLayout* lineLayout = m_lineLayouts[lineNumber];
    return QRectF(0, y, lineLayout->width, lineLayout->height);
}

QRectF LayoutEngine::selectionRect(int startPos, int endPos) const
{
    if (startPos >= endPos)
        return QRectF();

    QPointF startPoint = positionToPoint(startPos);
    QPointF endPoint = positionToPoint(endPos);

    // 简化：假设选择在同一行
    if (qFuzzyCompare(startPoint.y(), endPoint.y())) {
        qreal height = lineHeight();
        return QRectF(startPoint.x(), startPoint.y(),
            endPoint.x() - startPoint.x(), height);
    }
    else {
        // 多行选择：返回包围矩形
        qreal left = qMin(startPoint.x(), endPoint.x());
        qreal top = qMin(startPoint.y(), endPoint.y());
        qreal right = qMax(startPoint.x(), endPoint.x());
        qreal bottom = qMax(startPoint.y(), endPoint.y()) + lineHeight();

        return QRectF(left, top, right - left, bottom - top);
    }
}

// ==============================================================================
// 可见性优化
// ==============================================================================

QList<int> LayoutEngine::getVisibleLines() const
{
    QList<int> visibleLines;

    for (int i = m_viewport.firstVisibleLine; i <= m_viewport.lastVisibleLine; ++i) {
        if (i >= 0 && i < m_lineLayouts.size()) {
            visibleLines.append(i);
        }
    }

    return visibleLines;
}

void LayoutEngine::ensureLayoutForVisibleLines()
{
    QList<int> visibleLines = getVisibleLines();

    for (int lineNumber : visibleLines) {
        getLineLayout(lineNumber); // 这会触发布局创建
    }
}

// ==============================================================================
// 软换行支持
// ==============================================================================

int LayoutEngine::visualLineCount() const
{
    if (!m_wordWrap) {
        return m_lineLayouts.size();
    }

    int visualLines = 0;
    for (const LineLayout* lineLayout : m_lineLayouts) {
        if (lineLayout->layout) {
            visualLines += lineLayout->layout->lineCount();
        }
        else {
            visualLines += 1; // 估算
        }
    }

    return visualLines;
}

int LayoutEngine::logicalLineToVisualLine(int logicalLine) const
{
    if (!m_wordWrap || logicalLine < 0 || logicalLine >= m_lineLayouts.size()) {
        return logicalLine;
    }

    int visualLine = 0;
    for (int i = 0; i < logicalLine; ++i) {
        const LineLayout* lineLayout = m_lineLayouts[i];
        if (lineLayout->layout) {
            visualLine += lineLayout->layout->lineCount();
        }
        else {
            visualLine += 1;
        }
    }

    return visualLine;
}

int LayoutEngine::visualLineToLogicalLine(int visualLine) const
{
    if (!m_wordWrap) {
        return visualLine;
    }

    int currentVisualLine = 0;
    for (int i = 0; i < m_lineLayouts.size(); ++i) {
        const LineLayout* lineLayout = m_lineLayouts[i];
        int linesInLayout = lineLayout->layout ? lineLayout->layout->lineCount() : 1;

        if (currentVisualLine + linesInLayout > visualLine) {
            return i;
        }

        currentVisualLine += linesInLayout;
    }

    return qMax(0, m_lineLayouts.size() - 1);
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

void LayoutEngine::createLineLayout(int lineNumber)
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return;

    LineLayout* lineLayout = m_lineLayouts[lineNumber];

    // 清除旧布局
    if (lineLayout->layout) {
        delete lineLayout->layout;
    }

    // 创建新布局
    lineLayout->layout = new QTextLayout();
    lineLayout->layout->setFont(m_font);

    // 设置文本内容
    QString lineText = getLineText(lineNumber);
    lineLayout->layout->setText(lineText);

    // 设置文本选项
    QTextOption textOption;
    textOption.setTabStopDistance(tabWidth());

    if (m_wordWrap && m_textWidth > 0) {
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    }
    else {
        textOption.setWrapMode(QTextOption::NoWrap);
    }

    lineLayout->layout->setTextOption(textOption);

    // 开始布局过程
    lineLayout->layout->beginLayout();

    qreal lineY = 0;
    qreal maxWidth = 0;

    while (true) {
        QTextLine line = lineLayout->layout->createLine();
        if (!line.isValid())
            break;

        if (m_wordWrap && m_textWidth > 0) {
            line.setLineWidth(m_textWidth);
        }

        line.setPosition(QPointF(0, lineY));
        lineY += line.height();
        maxWidth = qMax(maxWidth, line.naturalTextWidth());
    }

    lineLayout->layout->endLayout();

    // 更新布局信息
    lineLayout->width = maxWidth;
    lineLayout->height = qMax(lineY, lineHeight()); // 至少一行高
    lineLayout->dirty = false;

    emit lineLayoutUpdated(lineNumber);
}

void LayoutEngine::updateLineLayout(LineLayout* layout)
{
    if (!layout || !layout->layout)
        return;

    // 重新计算布局
    createLineLayout(layout->lineNumber);
}

void LayoutEngine::clearLineLayouts()
{
    for (LineLayout* lineLayout : m_lineLayouts) {
        if (lineLayout->layout) {
            delete lineLayout->layout;
        }
        delete lineLayout;
    }
    m_lineLayouts.clear();
}

QStringList LayoutEngine::splitIntoLines(const QString& text) const
{
    return text.split('\n');
}

// ==============================================================================
// 辅助方法
// ==============================================================================

QString LayoutEngine::getLineText(int lineNumber) const
{
    QStringList lines = splitIntoLines(m_text);
    if (lineNumber >= 0 && lineNumber < lines.size()) {
        return lines[lineNumber];
    }
    return QString();
}

int LayoutEngine::positionToLine(int position) const
{
    int currentPos = 0;
    QStringList lines = splitIntoLines(m_text);

    for (int i = 0; i < lines.size(); ++i) {
        int lineLength = lines[i].length();
        if (currentPos + lineLength >= position) {
            return i;
        }
        currentPos += lineLength + 1; // +1 for newline
    }

    return qMax(0, lines.size() - 1);
}

int LayoutEngine::positionToColumn(int position, int lineNumber) const
{
    int lineStart = lineColumnToPosition(lineNumber, 0);
    return qMax(0, position - lineStart);
}

int LayoutEngine::lineColumnToPosition(int lineNumber, int column) const
{
    QStringList lines = splitIntoLines(m_text);

    if (lineNumber < 0 || lineNumber >= lines.size())
        return 0;

    int position = 0;

    // 累加前面行的长度
    for (int i = 0; i < lineNumber; ++i) {
        position += lines[i].length() + 1; // +1 for newline
    }

    // 加上当前行的列偏移
    int lineLength = lines[lineNumber].length();
    column = qBound(0, column, lineLength);
    position += column;

    return position;
}

QString LayoutEngine::expandTabs(const QString& text) const
{
    QString result;
    result.reserve(text.length() * 2); // 预留空间

    int column = 0;
    for (QChar ch : text) {
        if (ch == '\t') {
            // 计算到下一个制表位的空格数
            int spacesToAdd = m_tabWidth - (column % m_tabWidth);
            result.append(QString(spacesToAdd, ' '));
            column += spacesToAdd;
        }
        else {
            result.append(ch);
            column++;
        }
    }

    return result;
}

// ==============================================================================
// 高级功能
// ==============================================================================

// 获取行的实际渲染高度（考虑软换行）
qreal LayoutEngine::getLineRenderHeight(int lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return lineHeight();

    const LineLayout* lineLayout = m_lineLayouts[lineNumber];
    if (lineLayout->layout) {
        return lineLayout->layout->boundingRect().height();
    }

    return lineHeight();
}

// 获取行的实际渲染宽度
qreal LayoutEngine::getLineRenderWidth(int lineNumber) const
{
    if (lineNumber < 0 || lineNumber >= m_lineLayouts.size())
        return 0;

    const LineLayout* lineLayout = m_lineLayouts[lineNumber];
    if (lineLayout->layout) {
        return lineLayout->layout->boundingRect().width();
    }

    QString lineText = getLineText(lineNumber);
    QString expandedText = expandTabs(lineText);
    return m_fontMetrics.horizontalAdvance(expandedText);
}

// 缓存管理
void LayoutEngine::setMaxCachedLayouts(int maxLayouts)
{
    Q_UNUSED(maxLayouts)
        // 可以实现LRU缓存来限制内存使用
}

void LayoutEngine::clearLayoutCache()
{
    for (LineLayout* lineLayout : m_lineLayouts) {
        if (lineLayout->layout && !lineLayout->visible) {
            delete lineLayout->layout;
            lineLayout->layout = nullptr;
            lineLayout->dirty = true;
        }
    }
}

// 性能统计
int LayoutEngine::getCachedLayoutCount() const
{
    int count = 0;
    for (const LineLayout* lineLayout : m_lineLayouts) {
        if (lineLayout->layout) {
            count++;
        }
    }
    return count;
}

qreal LayoutEngine::getTotalDocumentHeight() const
{
    qreal totalHeight = 0;
    for (const LineLayout* lineLayout : m_lineLayouts) {
        totalHeight += lineLayout->height;
    }
    return totalHeight;
}

qreal LayoutEngine::getTotalDocumentWidth() const
{
    qreal maxWidth = 0;
    for (const LineLayout* lineLayout : m_lineLayouts) {
        maxWidth = qMax(maxWidth, lineLayout->width);
    }
    return maxWidth;
}

// 调试支持
QString LayoutEngine::getDebugInfo() const
{
    QStringList info;

    info << QString("LayoutEngine Debug Info:");
    info << QString("  Font: %1, %2pt").arg(m_font.family()).arg(m_font.pointSize());
    info << QString("  Line height: %1").arg(lineHeight());
    info << QString("  Character width: %1").arg(characterWidth());
    info << QString("  Tab width: %1 chars (%2 pixels)").arg(m_tabWidth).arg(tabWidth());
    info << QString("  Word wrap: %1").arg(m_wordWrap ? "enabled" : "disabled");
    info << QString("  Text width: %1").arg(m_textWidth);
    info << QString("  Total lines: %1").arg(m_lineLayouts.size());
    info << QString("  Cached layouts: %1").arg(getCachedLayoutCount());
    info << QString("  Document size: %1 x %2").arg(getTotalDocumentWidth()).arg(getTotalDocumentHeight());
    info << QString("  Viewport: (%1, %2) %3x%4")
        .arg(m_viewport.rect.x()).arg(m_viewport.rect.y())
        .arg(m_viewport.rect.width()).arg(m_viewport.rect.height());
    info << QString("  Visible lines: %1 - %2")
        .arg(m_viewport.firstVisibleLine).arg(m_viewport.lastVisibleLine);

    return info.join("\n");
}

// 布局验证
bool LayoutEngine::validateLayouts() const
{
    for (int i = 0; i < m_lineLayouts.size(); ++i) {
        const LineLayout* lineLayout = m_lineLayouts[i];

        if (lineLayout->lineNumber != i) {
            qWarning() << "Layout validation failed: line number mismatch";
            return false;
        }

        if (lineLayout->height <= 0) {
            qWarning() << "Layout validation failed: invalid height";
            return false;
        }
    }

    return true;
}