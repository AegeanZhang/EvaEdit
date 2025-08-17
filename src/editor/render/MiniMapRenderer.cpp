#include "MiniMapRenderer.h"
#include "TextRenderer.h"
#include "../core/DocumentModel.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFontMetrics>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

MiniMapRenderer::MiniMapRenderer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);

    // 设置默认大小
    setImplicitWidth(120);
    setImplicitHeight(400);

    // 优化渲染性能
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setPerformanceHint(QQuickPaintedItem::FastFBOResizing, true);
}

// ==============================================================================
// 属性访问器
// ==============================================================================

DocumentModel* MiniMapRenderer::document() const
{
    return m_document;
}

void MiniMapRenderer::setDocument(DocumentModel* document)
{
    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }

    m_document = document;

    if (m_document) {
        connect(m_document, &DocumentModel::textChanged,
            this, &MiniMapRenderer::onDocumentChanged);
        connect(m_document, &DocumentModel::modifiedChanged,
            this, [this]() { update(); });
    }

    update();
    emit documentChanged();
}

TextRenderer* MiniMapRenderer::textRenderer() const
{
    return m_textRenderer;
}

void MiniMapRenderer::setTextRenderer(TextRenderer* renderer)
{
    if (m_textRenderer == renderer)
        return;

    if (m_textRenderer) {
        disconnect(m_textRenderer, nullptr, this, nullptr);
    }

    m_textRenderer = renderer;

    if (m_textRenderer) {
        connect(m_textRenderer, &TextRenderer::scrollXChanged,
            this, &MiniMapRenderer::onViewportChanged);
        connect(m_textRenderer, &TextRenderer::scrollYChanged,
            this, &MiniMapRenderer::onViewportChanged);
        // 使用painted信号而不是不存在的信号
        connect(m_textRenderer, &TextRenderer::painted,
            this, [this]() { update(); });
    }

    update();
    emit textRendererChanged();
}

qreal MiniMapRenderer::scale() const
{
    return m_scale;
}

void MiniMapRenderer::setScale(qreal scale)
{
    scale = qBound(0.01, scale, 1.0); // 限制缩放范围

    if (qFuzzyCompare(m_scale, scale))
        return;

    m_scale = scale;
    update();
    emit scaleChanged();
}

QColor MiniMapRenderer::backgroundColor() const
{
    return m_backgroundColor;
}

void MiniMapRenderer::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor == color)
        return;

    m_backgroundColor = color;
    update();
    emit backgroundColorChanged();
}

QColor MiniMapRenderer::textColor() const
{
    return m_textColor;
}

void MiniMapRenderer::setTextColor(const QColor& color)
{
    if (m_textColor == color)
        return;

    m_textColor = color;
    update();
    emit textColorChanged();
}

QColor MiniMapRenderer::viewportColor() const
{
    return m_viewportColor;
}

void MiniMapRenderer::setViewportColor(const QColor& color)
{
    if (m_viewportColor == color)
        return;

    m_viewportColor = color;
    update();
    emit viewportColorChanged();
}

// ==============================================================================
// 核心渲染方法
// ==============================================================================

void MiniMapRenderer::paint(QPainter* painter)
{
    if (!painter || !m_document)
        return;

    painter->save();

    // 设置渲染提示
    painter->setRenderHint(QPainter::Antialiasing, false); // MiniMap 不需要抗锯齿
    painter->setRenderHint(QPainter::TextAntialiasing, false);

    QRectF rect = boundingRect();

    // 1. 绘制背景
    painter->fillRect(rect, m_backgroundColor);

    // 2. 绘制缩略文本
    paintMiniText(painter);

    // 3. 绘制视口指示器
    paintViewport(painter);

    painter->restore();
}

void MiniMapRenderer::paintMiniText(QPainter* painter)
{
    if (!m_document)
        return;

    painter->save();

    // 设置缩放字体
    //QFont miniFont("Consolas", 1); // 使用最小字体
    QFont miniFont("Consolas"); // 使用最小字体
    miniFont.setPixelSize(1); // 使用最小字体
    painter->setFont(miniFont);
    painter->setPen(m_textColor);

    QFontMetrics fm(miniFont);
    qreal lineHeight = fm.lineSpacing() * m_scale;
    // Qt 6 中 averageCharacterWidth() 被移除，使用 'M' 字符的宽度作为替代
    qreal charWidth = fm.horizontalAdvance('M') * m_scale;

    // 计算可见行范围
    int totalLines = m_document->lineCount();
    qreal totalHeight = totalLines * lineHeight;
    qreal availableHeight = height();

    // 如果内容高度小于可用高度，居中显示
    qreal startY = (totalHeight < availableHeight) ? (availableHeight - totalHeight) / 2 : 0;

    // 计算需要渲染的行范围
    int startLine = qMax(0, static_cast<int>(-startY / lineHeight));
    int endLine = qMin(totalLines, static_cast<int>((availableHeight - startY) / lineHeight) + 1);

    // 渲染可见行
    for (int lineNumber = startLine; lineNumber < endLine; ++lineNumber) {
        QString lineText = m_document->getLine(lineNumber);
        if (lineText.isEmpty())
            continue;

        qreal y = startY + lineNumber * lineHeight;

        // 简化渲染：每个字符用一个小矩形表示
        paintLineAsBlocks(painter, lineText, QPointF(2, y), charWidth, lineHeight);
    }

    painter->restore();
}

void MiniMapRenderer::paintLineAsBlocks(QPainter* painter, const QString& lineText,
    const QPointF& position, qreal charWidth, qreal lineHeight)
{
    if (lineText.isEmpty())
        return;

    painter->save();

    // 使用小矩形代替字符，提高渲染性能
    QRectF charRect(0, 0, qMax(1.0, charWidth), qMax(1.0, lineHeight * 0.8));

    qreal x = position.x();
    qreal y = position.y();

    int maxChars = static_cast<int>((width() - x) / charWidth);
    int charsToRender = qMin(lineText.length(), maxChars);

    for (int i = 0; i < charsToRender; ++i) {
        QChar ch = lineText.at(i);

        // 跳过空白字符
        if (ch.isSpace() && ch != '\t') {
            x += charWidth;
            continue;
        }

        // 设置字符颜色（可以根据语法高亮调整）
        QColor charColor = getCharacterColor(ch);
        painter->fillRect(QRectF(x, y, charRect.width(), charRect.height()), charColor);

        x += charWidth;
    }

    painter->restore();
}

QColor MiniMapRenderer::getCharacterColor(QChar ch) const
{
    // 简单的语法着色
    if (ch.isLetter()) {
        return m_textColor;
    }
    else if (ch.isDigit()) {
        return QColor(100, 150, 100); // 数字用绿色
    }
    else if (ch == '"' || ch == '\'') {
        return QColor(150, 100, 100); // 字符串用红色
    }
    else if (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == '[' || ch == ']') {
        return QColor(100, 100, 150); // 括号用蓝色
    }
    else {
        return m_textColor.lighter(150);
    }
}

void MiniMapRenderer::paintViewport(QPainter* painter)
{
    if (!m_textRenderer)
        return;

    painter->save();

    QRectF viewportRect = getViewportRect();

    // 绘制视口边框
    painter->setPen(QPen(m_viewportColor.darker(150), 1));
    painter->setBrush(m_viewportColor);
    painter->drawRect(viewportRect);

    painter->restore();
}

QRectF MiniMapRenderer::getViewportRect() const
{
    if (!m_textRenderer || !m_document)
        return QRectF();

    // 获取主编辑器的视口信息
    int scrollY = m_textRenderer->scrollY();
    qreal editorHeight = m_textRenderer->height();

    // 计算总文档高度
    int totalLines = m_document->lineCount();
    qreal miniLineHeight = height() / qMax(1, totalLines);

    // 计算可见行范围
    QFontMetrics fm(m_textRenderer->font());
    qreal realLineHeight = fm.lineSpacing();

    int visibleLines = static_cast<int>(editorHeight / realLineHeight);
    int firstVisibleLine = static_cast<int>(scrollY / realLineHeight);

    // 在 minimap 中的位置
    qreal miniY = firstVisibleLine * miniLineHeight;
    qreal miniHeight = visibleLines * miniLineHeight;

    // 限制在 minimap 范围内
    miniY = qBound(0.0, miniY, height() - miniHeight);
    miniHeight = qMin(miniHeight, height() - miniY);

    return QRectF(0, miniY, width(), miniHeight);
}

// ==============================================================================
// QML可调用方法
// ==============================================================================

void MiniMapRenderer::scrollToPosition(qreal y)
{
    if (!m_textRenderer || !m_document)
        return;

    // 将 minimap 的 y 坐标转换为文档位置
    qreal documentY = yToDocumentPosition(y);

    emit scrollRequested(documentY);
}

void MiniMapRenderer::setMiniMapWidth(qreal width)
{
    setImplicitWidth(width);
    setWidth(width);
    update();
}

qreal MiniMapRenderer::getRecommendedWidth() const
{
    if (!m_document)
        return 120;

    // 根据文档内容计算建议宽度
    int maxLineLength = 0;
    int lineCount = m_document->lineCount();

    // 扫描前100行来估算最大行长度
    int samplesToCheck = qMin(100, lineCount);
    for (int i = 0; i < samplesToCheck; ++i) {
        QString line = m_document->getLine(i);
        maxLineLength = qMax(maxLineLength, line.length());
    }

    // 基于最大行长度计算宽度
    qreal charWidth = 1.0 * m_scale;
    qreal recommendedWidth = qMin(200.0, qMax(80.0, maxLineLength * charWidth + 20));

    return recommendedWidth;
}

void MiniMapRenderer::highlightLine(int lineNumber, const QColor& color)
{
    Q_UNUSED(lineNumber)
        Q_UNUSED(color)
        // 可以存储高亮信息，在绘制时使用
        // m_highlightedLines[lineNumber] = color;
        update();
}

void MiniMapRenderer::clearHighlights()
{
    // m_highlightedLines.clear();
    update();
}

void MiniMapRenderer::setSyntaxHighlighting(bool enabled)
{
    Q_UNUSED(enabled)
        // m_syntaxHighlightingEnabled = enabled;
        update();
}

void MiniMapRenderer::showSearchResults(const QList<int>& positions)
{
    Q_UNUSED(positions)
        // m_searchResults = positions;
        update();
}

void MiniMapRenderer::clearSearchResults()
{
    // m_searchResults.clear();
    update();
}

void MiniMapRenderer::applyTheme(const QString& themeName)
{
    if (themeName == "dark") {
        setBackgroundColor(QColor(30, 30, 30));
        setTextColor(QColor(180, 180, 180));
        setViewportColor(QColor(0, 120, 215, 120));
    }
    else if (themeName == "light") {
        setBackgroundColor(QColor(240, 240, 240));
        setTextColor(QColor(100, 100, 100));
        setViewportColor(QColor(0, 120, 215, 100));
    }
    // 可以添加更多主题
}

void MiniMapRenderer::autoScale()
{
    if (!m_document || !m_textRenderer)
        return;

    int totalLines = m_document->lineCount();
    qreal availableHeight = height();

    if (totalLines > 0) {
        // 计算使所有行都可见的缩放比例
        qreal idealLineHeight = availableHeight / totalLines;
        qreal currentLineHeight = 20.0; // 估算值

        if (m_textRenderer) {
            QFontMetrics fm(m_textRenderer->font());
            currentLineHeight = fm.lineSpacing();
        }

        qreal newScale = idealLineHeight / currentLineHeight;
        newScale = qBound(0.01, newScale, 1.0);

        setScale(newScale);
    }
}

QString MiniMapRenderer::getDebugInfo() const
{
    QStringList info;

    info << QString("MiniMapRenderer Debug Info:");
    info << QString("  Scale: %1").arg(m_scale);
    info << QString("  Size: %1x%2").arg(width()).arg(height());
    info << QString("  Document: %1").arg(m_document ? "Valid" : "Null");
    info << QString("  TextRenderer: %1").arg(m_textRenderer ? "Valid" : "Null");

    if (m_document) {
        info << QString("  Document lines: %1").arg(m_document->lineCount());
        info << QString("  Document length: %1").arg(m_document->textLength());
    }

    if (m_textRenderer) {
        info << QString("  Scroll position: (%1, %2)")
            .arg(m_textRenderer->scrollX())
            .arg(m_textRenderer->scrollY());
    }

    QRectF viewportRect = getViewportRect();
    info << QString("  Viewport rect: (%1, %2, %3, %4)")
        .arg(viewportRect.x())
        .arg(viewportRect.y())
        .arg(viewportRect.width())
        .arg(viewportRect.height());

    return info.join("\n");
}

// ==============================================================================
// 批量更新
// ==============================================================================

void MiniMapRenderer::beginBatchUpdate()
{
    // m_batchUpdateMode = true;
}

void MiniMapRenderer::endBatchUpdate()
{
    // m_batchUpdateMode = false;
    update();
}

// ==============================================================================
// 获取可见范围
// ==============================================================================

QPair<int, int> MiniMapRenderer::getVisibleDocumentRange() const
{
    if (!m_textRenderer || !m_document)
        return QPair<int, int>(0, 0);

    QRectF viewportRect = getViewportRect();

    int totalLines = m_document->lineCount();
    qreal miniLineHeight = height() / qMax(1, totalLines);

    int startLine = static_cast<int>(viewportRect.top() / miniLineHeight);
    int endLine = static_cast<int>(viewportRect.bottom() / miniLineHeight);

    startLine = qBound(0, startLine, totalLines - 1);
    endLine = qBound(startLine, endLine, totalLines - 1);

    return QPair<int, int>(startLine, endLine);
}

// ==============================================================================
// 事件处理
// ==============================================================================

void MiniMapRenderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        scrollToPosition(event->position().y());
        event->accept();
    }
    else {
        QQuickPaintedItem::mousePressEvent(event);
    }
}

void MiniMapRenderer::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        scrollToPosition(event->position().y());
        event->accept();
    }
    else {
        QQuickPaintedItem::mouseMoveEvent(event);
    }
}

void MiniMapRenderer::wheelEvent(QWheelEvent* event)
{
    // 将滚轮事件转发给主编辑器
    if (m_textRenderer) {
        // 可以直接转发事件，或者发送滚动信号
        const int delta = event->angleDelta().y();
        const int scrollStep = 3;

        qreal currentY = m_textRenderer->scrollY();
        qreal newY;

        if (delta > 0) {
            newY = qMax(0.0, currentY - scrollStep * 20); // 假设行高为20
        }
        else {
            newY = currentY + scrollStep * 20;
        }

        emit scrollRequested(newY);
        event->accept();
    }
    else {
        QQuickPaintedItem::wheelEvent(event);
    }
}

// ==============================================================================
// 槽函数
// ==============================================================================

void MiniMapRenderer::onDocumentChanged()
{
    // 文档内容变化时更新 minimap
    // 使用 lambda 表达式避免成员函数指针的问题
    QTimer::singleShot(50, this, [this]() {
        update();
        });
}

void MiniMapRenderer::onViewportChanged()
{
    // 视口变化时只更新视口指示器
    update();
}

// ==============================================================================
// 坐标转换方法
// ==============================================================================

qreal MiniMapRenderer::yToDocumentPosition(qreal y) const
{
    if (!m_document)
        return 0;

    int totalLines = m_document->lineCount();
    if (totalLines == 0)
        return 0;

    // 计算对应的行号
    qreal lineRatio = y / height();
    int targetLine = static_cast<int>(lineRatio * totalLines);
    targetLine = qBound(0, targetLine, totalLines - 1);

    // 假设每行高度为20像素（这个值应该从 TextRenderer 获取）
    qreal estimatedLineHeight = 20.0;
    if (m_textRenderer) {
        QFontMetrics fm(m_textRenderer->font());
        estimatedLineHeight = fm.lineSpacing();
    }

    return targetLine * estimatedLineHeight;
}

qreal MiniMapRenderer::documentPositionToY(qreal position) const
{
    if (!m_document || !m_textRenderer)
        return 0;

    // 估算行高
    QFontMetrics fm(m_textRenderer->font());
    qreal lineHeight = fm.lineSpacing();

    // 计算行号
    int line = static_cast<int>(position / lineHeight);
    int totalLines = m_document->lineCount();

    if (totalLines == 0)
        return 0;

    // 在 minimap 中的相对位置
    qreal ratio = static_cast<qreal>(line) / totalLines;
    return ratio * height();
}