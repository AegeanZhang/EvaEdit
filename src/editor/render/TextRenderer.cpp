#include "TextRenderer.h"
#include "../core/DocumentModel.h"
#include "../service/LayoutEngine.h"
#include "../interaction/CursorManager.h"
#include "../interaction/SelectionManager.h"
#include "../service/SyntaxHighlighter.h"

#include <QPainter>
#include <QTextLayout>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include <QElapsedTimer>

TextRenderer::TextRenderer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_font("Consolas", 12)
    , m_backgroundColor(QColor(248, 249, 250)) // 浅色背景
    , m_textColor(QColor(36, 41, 47))          // 深色文字
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(QQuickItem::ItemAcceptsInputMethod, true);
    setFlag(QQuickItem::ItemIsFocusScope, true);

    // 初始化组件
    initializeComponents();

    // 连接信号
    connectSignals();

    // 设置初始行号宽度
    updateLineNumberWidth();
}

TextRenderer::~TextRenderer()
{
    // 智能指针会自动清理资源
}

void TextRenderer::initializeComponents()
{
    // 创建布局引擎
    m_layoutEngine = new LayoutEngine(this);
    m_layoutEngine->setFont(m_font);

    // 创建光标管理器
    m_cursorManager = new CursorManager(this);

    // 创建选择管理器
    m_selectionManager = new SelectionManager(this);

    // 创建语法高亮器
    m_syntaxHighlighter = new SyntaxHighlighter(this);
}

void TextRenderer::connectSignals()
{
    if (m_layoutEngine) {
        connect(m_layoutEngine, &LayoutEngine::layoutChanged,
            this, &TextRenderer::onLayoutChanged);
        connect(m_layoutEngine, &LayoutEngine::viewportChanged,
            this, [this]() { this->update(); });
    }

    if (m_cursorManager) {
        connect(m_cursorManager, &CursorManager::cursorPositionChanged,
            this, &TextRenderer::onCursorChanged);
        connect(m_cursorManager, &CursorManager::cursorVisibilityChanged,
            this, [this]() { this->update(); });
        connect(m_cursorManager, &CursorManager::ensureVisibleRequested,
            this, [this](const QRect& rect) {
                // 确保光标可见的逻辑
                Q_UNUSED(rect)
                    update();
            });
    }

    if (m_selectionManager) {
        connect(m_selectionManager, &SelectionManager::selectionsChanged,
            this, &TextRenderer::onSelectionChanged);
    }

    // 连接几何变化信号
    connect(this, &QQuickItem::widthChanged, this, &TextRenderer::onGeometryChanged);
    connect(this, &QQuickItem::heightChanged, this, &TextRenderer::onGeometryChanged);
}

// ==============================================================================
// 属性访问器实现
// ==============================================================================

DocumentModel* TextRenderer::document() const
{
    return m_document;
}

void TextRenderer::setDocument(DocumentModel* document)
{
    if (m_document == document)
        return;

    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }

    m_document = document;

    if (m_document) {
        connect(m_document, &DocumentModel::textChanged,
            this, &TextRenderer::onDocumentChanged);
        connect(m_document, &DocumentModel::modifiedChanged,
            this, [this]() { this->update(); });

        // 更新布局引擎的文本
        if (m_layoutEngine) {
            m_layoutEngine->setText(m_document->getFullText());
        }

        // 检测文件类型并设置语法高亮
        if (m_syntaxHighlighter) {
            m_syntaxHighlighter->setLanguageByFileExtension(
                QFileInfo(m_document->filePath()).suffix());
        }

        // 将文档关联到选择管理器
        if (m_selectionManager) {
            m_selectionManager->setDocument(m_document);
        }
    }

    update();
    emit documentChanged();
}

bool TextRenderer::showLineNumbers() const
{
    return m_showLineNumbers;
}

void TextRenderer::setShowLineNumbers(bool show)
{
    if (m_showLineNumbers == show)
        return;

    m_showLineNumbers = show;
    updateLineNumberWidth();
    update();
    emit lineNumbersChanged();
}

bool TextRenderer::wordWrap() const
{
    return m_wordWrap;
}

void TextRenderer::setWordWrap(bool wrap)
{
    if (m_wordWrap == wrap)
        return;

    m_wordWrap = wrap;

    if (m_layoutEngine) {
        m_layoutEngine->setWordWrap(wrap);
        if (wrap) {
            m_layoutEngine->setTextWidth(textArea().width());
        }
        else {
            m_layoutEngine->setTextWidth(-1);
        }
    }

    update();
    emit wordWrapChanged();
}

QFont TextRenderer::font() const
{
    return m_font;
}

void TextRenderer::setFont(const QFont& font)
{
    if (m_font == font)
        return;

    m_font = font;

    if (m_layoutEngine) {
        m_layoutEngine->setFont(font);
    }

    updateLineNumberWidth();
    update();
    emit fontChanged();
}

QColor TextRenderer::backgroundColor() const
{
    return m_backgroundColor;
}

void TextRenderer::setBackgroundColor(const QColor& color)
{
    if (m_backgroundColor == color)
        return;

    m_backgroundColor = color;
    update();
    emit backgroundColorChanged();
}

QColor TextRenderer::textColor() const
{
    return m_textColor;
}

void TextRenderer::setTextColor(const QColor& color)
{
    if (m_textColor == color)
        return;

    m_textColor = color;
    update();
    emit textColorChanged();
}

int TextRenderer::scrollX() const
{
    return m_scrollX;
}

void TextRenderer::setScrollX(int x)
{
    if (m_scrollX == x)
        return;

    m_scrollX = x;
    update();
    emit scrollXChanged();
}

int TextRenderer::scrollY() const
{
    return m_scrollY;
}

void TextRenderer::setScrollY(int y)
{
    if (m_scrollY == y)
        return;

    m_scrollY = y;
    update();
    emit scrollYChanged();
}

// ==============================================================================
// 核心渲染方法
// ==============================================================================

void TextRenderer::paint(QPainter* painter)
{
    if (!painter || !m_document || !m_layoutEngine)
        return;

    painter->save();

    // 设置渲染提示
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    QRectF rect = boundingRect();

    // 1. 绘制背景
    paintBackground(painter, rect);

    // 2. 绘制当前行高亮
    paintCurrentLine(painter, rect);

    // 3. 绘制选择区域
    paintSelections(painter, rect);

    // 4. 绘制文本
    paintText(painter, rect);

    // 5. 绘制行号
    if (m_showLineNumbers) {
        paintLineNumbers(painter, rect);
    }

    // 6. 绘制光标
    paintCursors(painter, rect);

    painter->restore();

    emit painted();
}

void TextRenderer::paintBackground(QPainter* painter, const QRectF& rect)
{
    painter->fillRect(rect, m_backgroundColor);
}

void TextRenderer::paintLineNumbers(QPainter* painter, const QRectF& rect)
{
    if (!m_showLineNumbers || !m_document)
        return;

    QRectF lineNumberRect = lineNumberArea();
    if (!rect.intersects(lineNumberRect))
        return;

    painter->save();

    // 设置行号区域背景
    QColor lineNumberBg = m_backgroundColor.darker(105);
    painter->fillRect(lineNumberRect, lineNumberBg);

    // 绘制分隔线
    painter->setPen(QPen(m_textColor.lighter(180), 1));
    painter->drawLine(lineNumberRect.topRight(), lineNumberRect.bottomRight());

    // 设置行号文字样式
    painter->setFont(m_font);
    painter->setPen(m_textColor.lighter(150));

    QFontMetrics fm(m_font);
    qreal lineHeight = m_layoutEngine ? m_layoutEngine->lineHeight() : fm.lineSpacing();

    // 计算可见行范围
    QList<int> visibleLines = getVisibleLines();

    for (int lineNumber : visibleLines) {
        qreal y = lineNumber * lineHeight - m_scrollY;

        if (y < rect.top() - lineHeight || y > rect.bottom() + lineHeight)
            continue;

        QString lineText = QString::number(lineNumber + 1);
        QRectF lineRect(lineNumberRect.left(), y,
            lineNumberRect.width() - 5, lineHeight);

        // 当前行高亮
        if (m_cursorManager &&
            m_document->positionToLine(m_cursorManager->cursorPosition()) == lineNumber) {
            painter->setPen(m_textColor);
        }
        else {
            painter->setPen(m_textColor.lighter(150));
        }

        painter->drawText(lineRect, Qt::AlignRight | Qt::AlignVCenter, lineText);
    }

    painter->restore();
}

void TextRenderer::paintText(QPainter* painter, const QRectF& rect)
{
    if (!m_document || !m_layoutEngine)
        return;

    painter->save();

    // 设置剪切区域为文本区域
    QRectF textRect = textArea();
    painter->setClipRect(textRect.intersected(rect));

    // 设置字体和颜色
    painter->setFont(m_font);
    painter->setPen(m_textColor);

    QFontMetrics fm(m_font);
    qreal lineHeight = m_layoutEngine->lineHeight();

    // 获取可见行
    QList<int> visibleLines = getVisibleLines();

    for (int lineNumber : visibleLines) {
        QString lineText = m_document->getLine(lineNumber);
        if (lineText.isEmpty())
            continue;

        qreal y = lineNumber * lineHeight - m_scrollY;
        qreal x = textRect.left() - m_scrollX;

        // 如果有语法高亮
        if (m_syntaxHighlighter) {
            QList<Token> tokens = m_syntaxHighlighter->tokenizeLine(lineText, lineNumber);
            paintHighlightedLine(painter, lineText, tokens, QPointF(x, y));
        }
        else {
            // 普通文本绘制
            painter->drawText(QPointF(x, y + fm.ascent()), lineText);
        }
    }

    painter->restore();
}

void TextRenderer::paintHighlightedLine(QPainter* painter, const QString& lineText,
    const QList<Token>& tokens, const QPointF& position)
{
    QFontMetrics fm(m_font);
    qreal x = position.x();
    qreal y = position.y();

    int lastPos = 0;

    for (const Token& token : tokens) {
        // 绘制token之前的普通文本
        if (token.position > lastPos) {
            QString normalText = lineText.mid(lastPos, token.position - lastPos);
            painter->setPen(m_textColor);
            painter->drawText(QPointF(x, y + fm.ascent()), normalText);
            x += fm.horizontalAdvance(normalText);
        }

        // 绘制高亮的token
        QString tokenText = lineText.mid(token.position, token.length);
        QTextCharFormat format = m_syntaxHighlighter->getFormat(token.type);

        // 应用格式
        if (format.hasProperty(QTextFormat::ForegroundBrush)) {
            painter->setPen(format.foreground().color());
        }
        else {
            painter->setPen(m_textColor);
        }

        if (format.fontWeight() == QFont::Bold) {
            QFont boldFont = m_font;
            boldFont.setBold(true);
            painter->setFont(boldFont);
        }

        painter->drawText(QPointF(x, y + fm.ascent()), tokenText);
        x += fm.horizontalAdvance(tokenText);

        // 恢复原字体
        if (format.fontWeight() == QFont::Bold) {
            painter->setFont(m_font);
        }

        lastPos = token.position + token.length;
    }

    // 绘制剩余的普通文本
    if (lastPos < lineText.length()) {
        QString remainingText = lineText.mid(lastPos);
        painter->setPen(m_textColor);
        painter->drawText(QPointF(x, y + fm.ascent()), remainingText);
    }
}

void TextRenderer::paintSelections(QPainter* painter, const QRectF& rect)
{
    if (!m_selectionManager || !m_selectionManager->hasSelection())
        return;

    painter->save();

    // 设置选择颜色
    QColor selectionColor = m_selectionManager->selectionColor();

    QList<SelectionRange> selections = m_selectionManager->selections();

    for (const SelectionRange& selection : selections) {
        QList<QRect> selectionRects = getSelectionRects(selection);

        for (const QRect& selRect : selectionRects) {
            if (rect.intersects(selRect)) {
                painter->fillRect(selRect, selectionColor);
            }
        }
    }

    painter->restore();
}

void TextRenderer::paintCursors(QPainter* painter, const QRectF& rect)
{
    if (!m_cursorManager)
        return;

    painter->save();

    // 只在获得焦点且闪烁可见时绘制光标
    if (!hasActiveFocus() || !m_cursorManager->isBlinkVisible()) {
        painter->restore();
        return;
    }

    painter->setPen(QPen(m_textColor, 2));

    QList<Cursor> cursors = m_cursorManager->cursors();

    for (const Cursor& cursor : cursors) {
        QPoint cursorPoint = positionToPoint(cursor.position);
        QFontMetrics fm(m_font);

        QRect cursorRect(cursorPoint.x() - 1, cursorPoint.y(),
            2, fm.height());

        if (rect.intersects(cursorRect)) {
            painter->drawLine(cursorRect.topLeft(), cursorRect.bottomLeft());
        }
    }

    painter->restore();
}

void TextRenderer::paintCurrentLine(QPainter* painter, const QRectF& rect)
{
    if (!m_cursorManager || !hasActiveFocus())
        return;

    painter->save();

    int currentLine = m_document ?
        m_document->positionToLine(m_cursorManager->cursorPosition()) : 0;

    QRect lineRect = this->lineRect(currentLine);

    if (rect.intersects(lineRect)) {
        QColor currentLineColor = m_backgroundColor.darker(103);
        painter->fillRect(lineRect, currentLineColor);
    }

    painter->restore();
}

// ==============================================================================
// 坐标转换方法
// ==============================================================================

QPoint TextRenderer::positionToPoint(int position) const
{
    if (!m_document || !m_layoutEngine)
        return QPoint();

    int line = m_document->positionToLine(position);
    int column = m_document->positionToColumn(position);

    QFontMetrics fm(m_font);
    qreal lineHeight = m_layoutEngine->lineHeight();

    qreal x = textArea().left() + fm.horizontalAdvance(
        m_document->getLine(line).left(column)) - m_scrollX;
    qreal y = line * lineHeight - m_scrollY;

    return QPoint(static_cast<int>(x), static_cast<int>(y));
}

int TextRenderer::pointToPosition(const QPointF& point) const
{
    if (!m_document || !m_layoutEngine)
        return 0;

    QRectF textRect = textArea();

    // 调整点坐标考虑滚动
    qreal adjustedX = point.x() - textRect.left() + m_scrollX;
    qreal adjustedY = point.y() + m_scrollY;

    qreal lineHeight = m_layoutEngine->lineHeight();
    int line = static_cast<int>(adjustedY / lineHeight);

    // 边界检查
    line = qBound(0, line, m_document->lineCount() - 1);

    QString lineText = m_document->getLine(line);
    QFontMetrics fm(m_font);

    int column = 0;
    qreal currentX = 0;

    for (int i = 0; i < lineText.length(); ++i) {
        qreal charWidth = fm.horizontalAdvance(lineText.at(i));
        if (currentX + charWidth / 2 > adjustedX) {
            break;
        }
        currentX += charWidth;
        column = i + 1;
    }

    return m_document->lineColumnToPosition(line, column);
}

QRect TextRenderer::lineRect(int lineNumber) const
{
    if (!m_layoutEngine)
        return QRect();

    qreal lineHeight = m_layoutEngine->lineHeight();
    qreal y = lineNumber * lineHeight - m_scrollY;

    return QRect(0, static_cast<int>(y),
        static_cast<int>(width()),
        static_cast<int>(lineHeight));
}

// ==============================================================================
// 可见性方法
// ==============================================================================

QList<int> TextRenderer::getVisibleLines() const
{
    QList<int> visibleLines;

    if (!m_document || !m_layoutEngine)
        return visibleLines;

    qreal lineHeight = m_layoutEngine->lineHeight();
    int firstLine = static_cast<int>(m_scrollY / lineHeight);
    int lastLine = static_cast<int>((m_scrollY + height()) / lineHeight) + 1;

    firstLine = qMax(0, firstLine);
    lastLine = qMin(m_document->lineCount() - 1, lastLine);

    for (int i = firstLine; i <= lastLine; ++i) {
        visibleLines.append(i);
    }

    return visibleLines;
}

void TextRenderer::ensurePositionVisible(int position)
{
    QPoint point = positionToPoint(position);
    QRectF textRect = textArea();

    // 垂直滚动
    if (point.y() < textRect.top()) {
        setScrollY(m_scrollY - (textRect.top() - point.y()));
    }
    else if (point.y() > textRect.bottom()) {
        setScrollY(m_scrollY + (point.y() - textRect.bottom()));
    }

    // 水平滚动
    if (point.x() < textRect.left()) {
        setScrollX(m_scrollX - (textRect.left() - point.x()));
    }
    else if (point.x() > textRect.right()) {
        setScrollX(m_scrollX + (point.x() - textRect.right()));
    }
}

void TextRenderer::ensureLineVisible(int lineNumber)
{
    if (!m_layoutEngine)
        return;

    qreal lineHeight = m_layoutEngine->lineHeight();
    qreal lineY = lineNumber * lineHeight;

    if (lineY < m_scrollY) {
        setScrollY(static_cast<int>(lineY));
    }
    else if (lineY + lineHeight > m_scrollY + height()) {
        setScrollY(static_cast<int>(lineY + lineHeight - height()));
    }
}

// ==============================================================================
// 事件处理
// ==============================================================================

void TextRenderer::mousePressEvent(QMouseEvent* event)
{
    setFocus(true);

    if (!m_document || !m_cursorManager)
        return;

    int position = pointToPosition(event->position());
    bool extend = event->modifiers() & Qt::ShiftModifier;

    m_cursorManager->setCursorPosition(position, extend);

    if (m_selectionManager && !extend) {
        int clickCount = getClickCount(event);

        if (clickCount == 2) {
            // 双击选择单词
            m_selectionManager->selectWord(position);
        }
        else if (clickCount >= 3) {
            // 三击选择行
            int line = m_document->positionToLine(position);
            m_selectionManager->selectLine(line);
        }
        else {
            // 单击清除选择
            m_selectionManager->clearSelections();
        }
    }

    update();
    QQuickPaintedItem::mousePressEvent(event);
}

void TextRenderer::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_document || !m_cursorManager)
        return;

    if (event->buttons() & Qt::LeftButton) {
        int position = pointToPosition(event->position());
        m_cursorManager->setCursorPosition(position, true); // 扩展选择
        update();
    }

    QQuickPaintedItem::mouseMoveEvent(event);
}

void TextRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    QQuickPaintedItem::mouseReleaseEvent(event);
}

void TextRenderer::mouseDoubleClickEvent(QMouseEvent* event)
{
    // 双击处理在 mousePressEvent 中
    QQuickPaintedItem::mouseDoubleClickEvent(event);
}

void TextRenderer::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y();
    const int scrollStep = 3; // 每次滚动3行

    if (delta > 0) {
        setScrollY(qMax(0, m_scrollY - scrollStep *
            static_cast<int>(m_layoutEngine ? m_layoutEngine->lineHeight() : 20)));
    }
    else {
        int maxScroll = qMax(0, static_cast<int>(
            (m_document ? m_document->lineCount() : 0) *
            (m_layoutEngine ? m_layoutEngine->lineHeight() : 20) - height()));
        setScrollY(qMin(maxScroll, m_scrollY + scrollStep *
            static_cast<int>(m_layoutEngine ? m_layoutEngine->lineHeight() : 20)));
    }

    update();
    event->accept();
}

void TextRenderer::keyPressEvent(QKeyEvent* event)
{
    if (!m_document) {
        QQuickPaintedItem::keyPressEvent(event);
        return;
    }

    // 简单的键盘处理（完整的输入管理应该在 InputManager 中）
    switch (event->key()) {
    case Qt::Key_Left:
        if (m_cursorManager) {
            int pos = m_cursorManager->cursorPosition();
            bool extend = event->modifiers() & Qt::ShiftModifier;
            m_cursorManager->setCursorPosition(qMax(0, pos - 1), extend);
            ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        break;
    case Qt::Key_Right:
        if (m_cursorManager) {
            int pos = m_cursorManager->cursorPosition();
            bool extend = event->modifiers() & Qt::ShiftModifier;
            m_cursorManager->setCursorPosition(
                qMin(m_document->textLength(), pos + 1), extend);
            ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
        // 垂直移动的详细实现
        handleVerticalMovement(event);
        break;
    default:
        if (!event->text().isEmpty() && event->text().at(0).isPrint()) {
            // 插入文本
            if (m_cursorManager) {
                int pos = m_cursorManager->cursorPosition();
                m_document->insertText(pos, event->text());
                m_cursorManager->setCursorPosition(pos + event->text().length());
                ensurePositionVisible(m_cursorManager->cursorPosition());
            }
        }
        else {
            QQuickPaintedItem::keyPressEvent(event);
        }
        break;
    }

    update();
}

void TextRenderer::handleVerticalMovement(QKeyEvent* event)
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int currentLine = m_document->positionToLine(currentPos);
    int currentColumn = m_document->positionToColumn(currentPos);

    int targetLine = currentLine;
    if (event->key() == Qt::Key_Up) {
        targetLine = qMax(0, currentLine - 1);
    }
    else {
        targetLine = qMin(m_document->lineCount() - 1, currentLine + 1);
    }

    // 保持列位置，但不超过目标行的长度
    QString targetLineText = m_document->getLine(targetLine);
    int targetColumn = qMin(currentColumn, targetLineText.length());

    int newPos = m_document->lineColumnToPosition(targetLine, targetColumn);
    bool extend = event->modifiers() & Qt::ShiftModifier;

    m_cursorManager->setCursorPosition(newPos, extend);
    ensurePositionVisible(newPos);
}

void TextRenderer::keyReleaseEvent(QKeyEvent* event)
{
    QQuickPaintedItem::keyReleaseEvent(event);
}

void TextRenderer::inputMethodEvent(QInputMethodEvent* event)
{
    // 输入法事件处理
    if (!m_document || !m_cursorManager)
        return;

    QString commitString = event->commitString();
    if (!commitString.isEmpty()) {
        int pos = m_cursorManager->cursorPosition();
        m_document->insertText(pos, commitString);
        m_cursorManager->setCursorPosition(pos + commitString.length());
        ensurePositionVisible(m_cursorManager->cursorPosition());
        update();
    }

    QQuickPaintedItem::inputMethodEvent(event);
}

QVariant TextRenderer::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch (query) {
    case Qt::ImCursorRectangle:
        if (m_cursorManager) {
            QPoint cursorPoint = positionToPoint(m_cursorManager->cursorPosition());
            QFontMetrics fm(m_font);
            return QRect(cursorPoint, QSize(1, fm.height()));
        }
        break;
    case Qt::ImFont:
        return m_font;
    case Qt::ImCursorPosition:
        return m_cursorManager ? m_cursorManager->cursorPosition() : 0;
    default:
        break;
    }

    return QQuickPaintedItem::inputMethodQuery(query);
}

// ==============================================================================
// 槽函数实现
// ==============================================================================

void TextRenderer::onDocumentChanged()
{
    if (m_layoutEngine && m_document) {
        m_layoutEngine->setText(m_document->getFullText());
    }
    update();
}

void TextRenderer::onLayoutChanged()
{
    update();
}

void TextRenderer::onCursorChanged()
{
    update();
}

void TextRenderer::onSelectionChanged()
{
    update();
}

void TextRenderer::onGeometryChanged()
{
    if (m_layoutEngine && m_wordWrap) {
        m_layoutEngine->setTextWidth(textArea().width());
    }

    updateLineNumberWidth();
    update();
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

void TextRenderer::updateLineNumberWidth()
{
    if (!m_showLineNumbers || !m_document) {
        m_lineNumberWidth = 0;
        return;
    }

    QFontMetrics fm(m_font);
    int lineCount = m_document->lineCount();
    int digits = QString::number(lineCount).length();
    m_lineNumberWidth = fm.horizontalAdvance('9') * digits + 20; // 额外的边距
}

QRectF TextRenderer::textArea() const
{
    qreal left = m_showLineNumbers ? m_lineNumberWidth : 0;
    return QRectF(left, 0, width() - left, height());
}

QRectF TextRenderer::lineNumberArea() const
{
    if (!m_showLineNumbers)
        return QRectF();

    return QRectF(0, 0, m_lineNumberWidth, height());
}

QList<QRect> TextRenderer::getSelectionRects(const SelectionRange& selection) const
{
    QList<QRect> rects;

    if (!m_document || selection.isEmpty())
        return rects;

    int startLine = m_document->positionToLine(selection.start);
    int endLine = m_document->positionToLine(selection.end);

    QFontMetrics fm(m_font);
    qreal lineHeight = m_layoutEngine ? m_layoutEngine->lineHeight() : fm.lineSpacing();
    QRectF textRect = textArea();

    for (int line = startLine; line <= endLine; ++line) {
        QString lineText = m_document->getLine(line);

        int lineStart = m_document->lineColumnToPosition(line, 0);
        int lineEnd = lineStart + lineText.length();

        int selStart = qMax(selection.start, lineStart) - lineStart;
        int selEnd = qMin(selection.end, lineEnd) - lineStart;

        if (selStart < selEnd) {
            qreal x1 = textRect.left() + fm.horizontalAdvance(lineText.left(selStart)) - m_scrollX;
            qreal x2 = textRect.left() + fm.horizontalAdvance(lineText.left(selEnd)) - m_scrollX;
            qreal y = line * lineHeight - m_scrollY;

            rects.append(QRect(static_cast<int>(x1), static_cast<int>(y),
                static_cast<int>(x2 - x1), static_cast<int>(lineHeight)));
        }
    }

    return rects;
}

int TextRenderer::getClickCount(QMouseEvent* event)
{
    static const int DOUBLE_CLICK_TIME = 500; // 毫秒

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (currentTime - m_lastClickTime <= DOUBLE_CLICK_TIME) {
        m_lastClickCount++;
    }
    else {
        m_lastClickCount = 1;
    }

    m_lastClickTime = currentTime;
    return m_lastClickCount;
}