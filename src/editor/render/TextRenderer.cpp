#include "TextRenderer.h"
#include "../core/DocumentModel.h"
#include "../service/LayoutEngine.h"
#include "../interaction/InputManager.h"
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

static const int TEXT_LEFT_PADDING = 5; // 文本左侧内边距

TextRenderer::TextRenderer(QQuickItem* parent)
    : QQuickPaintedItem(parent)
    , m_backgroundColor(QColor(248, 249, 250)) // 浅色背景
    , m_textColor(QColor(36, 41, 47))          // 深色文字
{
    // 明确创建字体，只设置点大小
    m_font = QFont("Consolas");
    m_font.setPixelSize(12);

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

    // 创建输入管理器
    m_inputManager = new InputManager(this);
    //setupInputManager();
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

QColor TextRenderer::lineNumberSeparatorColor() const
{
    return m_lineNumberSeparatorColor;
}

void TextRenderer::setLineNumberSeparatorColor(const QColor& color)
{
    if (m_lineNumberSeparatorColor == color)
        return;
    m_lineNumberSeparatorColor = color;
    update();
    emit lineNumberSeparatorColorChanged();
}

int TextRenderer::lineNumberExtraWidth() const
{
    return m_lineNumberExtraWidth;
}

void TextRenderer::setLineNumberExtraWidth(int width)
{
    if (m_lineNumberExtraWidth == width)
        return;
    m_lineNumberExtraWidth = width;
    updateLineNumberWidth();
    update();
    emit lineNumberExtraWidthChanged();
}

/*qreal TextRenderer::lineNumberWidth() const
{
    return m_lineNumberWidth;
}
void TextRenderer::setLineNumberWidth(qreal width)
{
    if (qFuzzyCompare(m_lineNumberWidth, width))
        return;
    m_lineNumberWidth = width;
    update();
    emit lineNumberWidthChanged();
}*/


// ==============================================================================
// 核心渲染方法
// ==============================================================================

void TextRenderer::paint(QPainter* painter)
{
    if (!painter || !m_document || !m_layoutEngine)
        return;

    painter->save();

    // 设置渲染提示, 只在需要时启用抗锯齿，减少不必要的性能开销
    //painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    QRectF rect = boundingRect();

    // 1. 绘制背景
    paintBackground(painter, rect);

    // 2. 绘制当前行高亮
    paintCurrentLine(painter, rect);

    // 3. 绘制选择区域
    paintSelections(painter, rect);

    // 4. 绘制文本
    // TODO 建议：在paintText内部实现脏区域裁剪，只绘制可见区域文本
    paintText(painter, rect);

    // 5. 绘制行号
    if (m_showLineNumbers) {
        // TODO 建议：在paintLineNumbers内部缓存行号文本，避免重复计算
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
    //QColor lineNumberBg = m_backgroundColor.darker(105);
    QColor lineNumberBg = m_backgroundColor;
    painter->fillRect(lineNumberRect, lineNumberBg);

    // 绘制分隔线
    //painter->setPen(QPen(m_textColor.lighter(180), 1));
    painter->setPen(QPen(m_lineNumberSeparatorColor, 1));
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
        //qreal x = textRect.left() - m_scrollX;
        qreal x = textRect.left() + TEXT_LEFT_PADDING - m_scrollX;

		// TODO 这里paintHighlightedLine消耗CPU太高，需要优化，暂时先注释掉
        // 如果有语法高亮
        /*
        if (m_syntaxHighlighter) {
            QList<Token> tokens = m_syntaxHighlighter->tokenizeLine(lineText, lineNumber);
            paintHighlightedLine(painter, lineText, tokens, QPointF(x, y));
        }
        else {
            // 普通文本绘制
            painter->drawText(QPointF(x, y + fm.ascent()), lineText);
        }
        */
        painter->drawText(QPointF(x, y + fm.ascent()), lineText);
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

    qreal x = textArea().left() + TEXT_LEFT_PADDING + fm.horizontalAdvance(
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
    qreal adjustedX = point.x() - textRect.left() - TEXT_LEFT_PADDING + m_scrollX;
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

/*
void TextRenderer::keyPressEvent(QKeyEvent* event)
{
    if (!m_document) {
        QQuickPaintedItem::keyPressEvent(event);
        return;
    }

    // 使用 InputManager 处理键盘事件
    if (m_inputManager && m_inputManager->handleKeyEvent(event)) {
        update();
        return;
    }

    // 后备处理
    QQuickPaintedItem::keyPressEvent(event);
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
*/

void TextRenderer::handleHorizontalMovement(QKeyEvent* event)
{
    if (!event) return;

    MoveDirection direction = (event->key() == Qt::Key_Left) ? MoveDirection::Left : MoveDirection::Right;
    bool extend = event->modifiers() & Qt::ShiftModifier;

    // 根据修饰键确定移动单位
    MoveUnit unit = MoveUnit::Character;
    if (event->modifiers() & Qt::ControlModifier) {
        unit = MoveUnit::Word;  // Ctrl + ←/→ = 单词移动
    }

    handleMovement(direction, extend, unit);
}

void TextRenderer::handleVerticalMovement(QKeyEvent* event)
{
    if (!event) return;

    MoveDirection direction = (event->key() == Qt::Key_Up) ? MoveDirection::Up : MoveDirection::Down;
    bool extend = event->modifiers() & Qt::ShiftModifier;

    MoveUnit unit = MoveUnit::Character;
    if (event->modifiers() & Qt::ControlModifier) {
        unit = MoveUnit::Page;  // Ctrl + ↑/↓ = 页面移动
    }

    handleMovement(direction, extend, unit);
}

void TextRenderer::handleMovement(MoveDirection direction, bool extend, MoveUnit unit)
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateMovementTarget(currentPos, direction, unit);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, extend);
        ensurePositionVisible(targetPos);
    }
}

int TextRenderer::calculateMovementTarget(int currentPos, MoveDirection direction, MoveUnit unit) const
{
    switch (unit) {
    case MoveUnit::Character:
        return calculateCharacterMovement(currentPos, direction);
    case MoveUnit::Word:
        return calculateWordMovement(currentPos, direction);
    case MoveUnit::Line:
        return calculateLineMovement(currentPos, direction);
    case MoveUnit::Page:
        return calculatePageMovement(currentPos, direction);
    case MoveUnit::Document:
        return calculateDocumentMovement(currentPos, direction);
    }
    return currentPos;
}

int TextRenderer::calculateCharacterMovement(int currentPos, MoveDirection direction) const
{
    switch (direction) {
    case MoveDirection::Left:
        return qMax(0, currentPos - 1);
    case MoveDirection::Right:
        return qMin(m_document->textLength(), currentPos + 1);
    case MoveDirection::Up:
    case MoveDirection::Down:
        return calculateVerticalCharacterMovement(currentPos, direction);
    }
    return currentPos;
}

int TextRenderer::calculateWordMovement(int currentPos, MoveDirection direction) const
{
    switch (direction) {
    case MoveDirection::Left:
    case MoveDirection::Right:
        return findWordBoundary(currentPos, direction == MoveDirection::Right);
    case MoveDirection::Up:
    case MoveDirection::Down:
        return findParagraphBoundary(currentPos, direction == MoveDirection::Down);
    }
    return currentPos;
}

int TextRenderer::calculateVerticalCharacterMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document || !m_layoutEngine)
        return currentPos;

    int currentLine = m_document->positionToLine(currentPos);
    int currentColumn = m_document->positionToColumn(currentPos);
    int targetLine = currentLine;

    // 计算目标行
    if (direction == MoveDirection::Up) {
        targetLine = qMax(0, currentLine - 1);
    }
    else if (direction == MoveDirection::Down) {
        targetLine = qMin(m_document->lineCount() - 1, currentLine + 1);
    }

    // 如果行号没有变化，返回原位置
    if (targetLine == currentLine) {
        return currentPos;
    }

    // 保持列位置，但不超过目标行的长度
    QString targetLineText = m_document->getLine(targetLine);
    int targetColumn = qMin(currentColumn, targetLineText.length());

    return m_document->lineColumnToPosition(targetLine, targetColumn);
}

int TextRenderer::findWordBoundary(int position, bool forward) const
{
    if (!m_document)
        return position;

    QString text = m_document->getFullText();
    int textLength = text.length();

    // 边界检查
    position = qBound(0, position, textLength);

    if (forward) {
        // 向前查找单词边界

        // 如果当前位置是单词字符，跳过当前单词
        while (position < textLength && isWordCharacter(text.at(position))) {
            position++;
        }

        // 跳过非单词字符（空格、标点等）
        while (position < textLength && !isWordCharacter(text.at(position))) {
            position++;
        }

        return qMin(position, textLength);
    }
    else {
        // 向后查找单词边界

        // 如果不在开头，先回退一个位置
        if (position > 0) {
            position--;
        }

        // 跳过非单词字符
        while (position > 0 && !isWordCharacter(text.at(position))) {
            position--;
        }

        // 跳过当前单词到单词开始
        while (position > 0 && isWordCharacter(text.at(position - 1))) {
            position--;
        }

        return qMax(0, position);
    }
}

int TextRenderer::findParagraphBoundary(int position, bool forward) const
{
    if (!m_document)
        return position;

    QString text = m_document->getFullText();
    int textLength = text.length();

    // 边界检查
    position = qBound(0, position, textLength);

    if (forward) {
        // 向前查找段落边界
        bool foundEmptyLine = false;

        while (position < textLength) {
            if (text.at(position) == '\n') {
                // 检查下一行是否为空行或只有空白字符
                int nextLineStart = position + 1;
                bool isEmptyLine = true;

                // 检查下一行内容
                while (nextLineStart < textLength && text.at(nextLineStart) != '\n') {
                    if (!text.at(nextLineStart).isSpace()) {
                        isEmptyLine = false;
                        break;
                    }
                    nextLineStart++;
                }

                if (isEmptyLine) {
                    foundEmptyLine = true;
                    position = nextLineStart;
                    if (position < textLength && text.at(position) == '\n') {
                        position++; // 跳过空行的换行符
                    }
                    break;
                }
            }
            position++;
        }

        // 如果没找到空行，返回文档末尾
        if (!foundEmptyLine) {
            position = textLength;
        }

        return qMin(position, textLength);
    }
    else {
        // 向后查找段落边界
        bool foundEmptyLine = false;

        while (position > 0) {
            position--;

            if (text.at(position) == '\n') {
                // 检查当前行是否为空行
                int lineStart = position + 1;
                bool isEmptyLine = true;

                // 检查当前行内容
                int linePos = lineStart;
                while (linePos < textLength && text.at(linePos) != '\n') {
                    if (!text.at(linePos).isSpace()) {
                        isEmptyLine = false;
                        break;
                    }
                    linePos++;
                }

                if (isEmptyLine) {
                    foundEmptyLine = true;
                    // 找到空行，现在找到下一个非空行的开始
                    position = lineStart;
                    while (position < textLength) {
                        if (text.at(position) == '\n') {
                            position++;
                            continue;
                        }
                        if (!text.at(position).isSpace()) {
                            break;
                        }
                        position++;
                    }
                    break;
                }
            }
        }

        // 如果没找到空行，返回文档开始
        if (!foundEmptyLine) {
            position = 0;
        }

        return qMax(0, position);
    }
}

// ==============================================================================
// 其他缺失的移动计算函数实现
// ==============================================================================

int TextRenderer::calculateLineMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document)
        return currentPos;

    int currentLine = m_document->positionToLine(currentPos);

    if (direction == MoveDirection::Left) {
        // 移动到行首
        return m_document->lineColumnToPosition(currentLine, 0);
    }
    else if (direction == MoveDirection::Right) {
        // 移动到行尾
        QString lineText = m_document->getLine(currentLine);
        return m_document->lineColumnToPosition(currentLine, lineText.length());
    }

    return currentPos;
}

int TextRenderer::calculatePageMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document || !m_layoutEngine)
        return currentPos;

    int currentLine = m_document->positionToLine(currentPos);
    int currentColumn = m_document->positionToColumn(currentPos);

    // 计算可见行数
    qreal lineHeight = m_layoutEngine->lineHeight();
    int visibleLines = static_cast<int>(height() / lineHeight);

    int targetLine = currentLine;

    if (direction == MoveDirection::Up) {
        // 向上一页
        targetLine = qMax(0, currentLine - visibleLines);
    }
    else if (direction == MoveDirection::Down) {
        // 向下一页
        targetLine = qMin(m_document->lineCount() - 1, currentLine + visibleLines);
    }

    // 保持列位置
    QString targetLineText = m_document->getLine(targetLine);
    int targetColumn = qMin(currentColumn, targetLineText.length());

    return m_document->lineColumnToPosition(targetLine, targetColumn);
}

int TextRenderer::calculateDocumentMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document)
        return currentPos;

    if (direction == MoveDirection::Up || direction == MoveDirection::Left) {
        // 移动到文档开始
        return 0;
    }
    else if (direction == MoveDirection::Down || direction == MoveDirection::Right) {
        // 移动到文档结束
        return m_document->textLength();
    }

    return currentPos;
}

// ==============================================================================
// 辅助函数
// ==============================================================================

bool TextRenderer::isWordCharacter(QChar ch) const
{
    // 定义单词字符：字母、数字、下划线
    return ch.isLetterOrNumber() || ch == '_';
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

/*void TextRenderer::updateLineNumberWidth()
{
    if (!m_showLineNumbers || !m_document) {
        m_lineNumberWidth = 0;
        return;
    }

    QFontMetrics fm(m_font);
    int lineCount = m_document->lineCount();
    int digits = QString::number(lineCount).length();
    m_lineNumberWidth = fm.horizontalAdvance('9') * digits + 20; // 额外的边距
}*/

void TextRenderer::updateLineNumberWidth()
{
    if (!m_showLineNumbers || !m_document) {
        if (!qFuzzyCompare(m_lineNumberWidth, 0.0)) {
            m_lineNumberWidth = 0;
            emit lineNumberExtraWidthChanged(); // 添加信号发送
        }
        return;
    }

    QFontMetrics fm(m_font);
    int lineCount = m_document->lineCount();
    int digits = QString::number(lineCount).length();
    //qreal newWidth = fm.horizontalAdvance('9') * digits + 20; // 额外的边距
    qreal newWidth = fm.horizontalAdvance('9') * digits + m_lineNumberExtraWidth; // 额外的边距

    if (!qFuzzyCompare(m_lineNumberWidth, newWidth)) {
        m_lineNumberWidth = newWidth;
        emit lineNumberExtraWidthChanged(); // 添加信号发送
    }
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


/*void TextRenderer::setupInputManager()
{
    if (!m_inputManager) return;

    // 注册命令处理器
    m_inputManager->registerCommandHandler(EditCommand::NewLine, [this](const QString&) {
        if (m_cursorManager && m_document) {
            int pos = m_cursorManager->cursorPosition();
            m_document->insertText(pos, "\n");
            m_cursorManager->setCursorPosition(pos + 1);
            ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        });

    m_inputManager->registerCommandHandler(EditCommand::InsertText, [this](const QString& text) {
        if (m_cursorManager && m_document) {
            int pos = m_cursorManager->cursorPosition();
            m_document->insertText(pos, text);
            m_cursorManager->setCursorPosition(pos + text.length());
            ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        });

    m_inputManager->registerCommandHandler(EditCommand::DeleteLeft, [this](const QString&) {
        if (m_cursorManager && m_document && m_cursorManager->cursorPosition() > 0) {
            int pos = m_cursorManager->cursorPosition();
            m_document->removeText(pos - 1, 1);
            m_cursorManager->setCursorPosition(pos - 1);
            ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        });

    // ... 其他命令处理器 ...
}*/

/*
void TextRenderer::setupInputManager()
{
    if (!m_inputManager) return;

    // 使用 QMetaObject::invokeMethod 来调用方法
    m_inputManager->registerCommandHandler(EditCommand::MoveCursorLeft,
        [this](const QString&) { this->handleMoveCursorLeft(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorRight,
        [this](const QString&) { this->handleMoveCursorRight(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorUp,
        [this](const QString&) { this->handleMoveCursorUp(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorDown,
        [this](const QString&) { this->handleMoveCursorDown(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorWordLeft,
        [this](const QString&) { this->handleMoveCursorWordLeft(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorWordRight,
        [this](const QString&) { this->handleMoveCursorWordRight(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorLineStart,
        [this](const QString&) { this->handleMoveCursorLineStart(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorLineEnd,
        [this](const QString&) { this->handleMoveCursorLineEnd(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorDocumentStart,
        [this](const QString&) { this->handleMoveCursorDocumentStart(); });

    m_inputManager->registerCommandHandler(EditCommand::MoveCursorDocumentEnd,
        [this](const QString&) { this->handleMoveCursorDocumentEnd(); });

    // 选择命令
    m_inputManager->registerCommandHandler(EditCommand::SelectLeft,
        [this](const QString&) { this->handleSelectLeft(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectRight,
        [this](const QString&) { this->handleSelectRight(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectUp,
        [this](const QString&) { this->handleSelectUp(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectDown,
        [this](const QString&) { this->handleSelectDown(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectAll,
        [this](const QString&) { this->handleSelectAll(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectWord,
        [this](const QString&) { this->handleSelectWord(); });

    m_inputManager->registerCommandHandler(EditCommand::SelectLine,
        [this](const QString&) { this->handleSelectLine(); });

    // 编辑命令
    m_inputManager->registerCommandHandler(EditCommand::InsertText,
        [this](const QString& text) { this->handleInsertText(text); });

    m_inputManager->registerCommandHandler(EditCommand::NewLine,
        [this](const QString&) { this->handleNewLine(); });

    m_inputManager->registerCommandHandler(EditCommand::Tab,
        [this](const QString&) { this->handleTab(); });

    m_inputManager->registerCommandHandler(EditCommand::DeleteLeft,
        [this](const QString&) { this->handleDeleteLeft(); });

    m_inputManager->registerCommandHandler(EditCommand::DeleteRight,
        [this](const QString&) { this->handleDeleteRight(); });

    m_inputManager->registerCommandHandler(EditCommand::DeleteWordLeft,
        [this](const QString&) { this->handleDeleteWordLeft(); });

    m_inputManager->registerCommandHandler(EditCommand::DeleteWordRight,
        [this](const QString&) { this->handleDeleteWordRight(); });

    // 剪贴板命令
    m_inputManager->registerCommandHandler(EditCommand::Cut,
        [this](const QString&) { this->handleCut(); });

    m_inputManager->registerCommandHandler(EditCommand::Copy,
        [this](const QString&) { this->handleCopy(); });

    m_inputManager->registerCommandHandler(EditCommand::Paste,
        [this](const QString&) { this->handlePaste(); });

    // 撤销重做命令
    m_inputManager->registerCommandHandler(EditCommand::Undo,
        [this](const QString&) { this->handleUndo(); });

    m_inputManager->registerCommandHandler(EditCommand::Redo,
        [this](const QString&) { this->handleRedo(); });
}
*/

// ==============================================================================
// 命令处理方法实现
// ==============================================================================
/*
void TextRenderer::handleMoveCursorLeft()
{
    if (!m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMax(0, pos - 1), false);
    ensurePositionVisible(m_cursorManager->cursorPosition());
}

void TextRenderer::handleMoveCursorRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMin(m_document->textLength(), pos + 1), false);
    ensurePositionVisible(m_cursorManager->cursorPosition());
}

void TextRenderer::handleMoveCursorUp()
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Up);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, false);
        ensurePositionVisible(targetPos);
    }
}

void TextRenderer::handleMoveCursorDown()
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Down);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, false);
        ensurePositionVisible(targetPos);
    }
}

void TextRenderer::handleMoveCursorWordLeft()
{
    if (!m_cursorManager || !m_document) return;

    int pos = findWordBoundary(m_cursorManager->cursorPosition(), false);
    m_cursorManager->setCursorPosition(pos, false);
    ensurePositionVisible(pos);
}

void TextRenderer::handleMoveCursorWordRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = findWordBoundary(m_cursorManager->cursorPosition(), true);
    m_cursorManager->setCursorPosition(pos, false);
    ensurePositionVisible(pos);
}

void TextRenderer::handleMoveCursorLineStart()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(currentPos);
    int lineStartPos = m_document->lineColumnToPosition(line, 0);
    m_cursorManager->setCursorPosition(lineStartPos, false);
    ensurePositionVisible(lineStartPos);
}

void TextRenderer::handleMoveCursorLineEnd()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(currentPos);
    QString lineText = m_document->getLine(line);
    int lineEndPos = m_document->lineColumnToPosition(line, lineText.length());
    m_cursorManager->setCursorPosition(lineEndPos, false);
    ensurePositionVisible(lineEndPos);
}

void TextRenderer::handleMoveCursorDocumentStart()
{
    if (!m_cursorManager) return;

    m_cursorManager->setCursorPosition(0, false);
    ensurePositionVisible(0);
}

void TextRenderer::handleMoveCursorDocumentEnd()
{
    if (!m_cursorManager || !m_document) return;

    int endPos = m_document->textLength();
    m_cursorManager->setCursorPosition(endPos, false);
    ensurePositionVisible(endPos);
}

void TextRenderer::handleSelectLeft()
{
    if (!m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMax(0, pos - 1), true);
    ensurePositionVisible(m_cursorManager->cursorPosition());
}

void TextRenderer::handleSelectRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMin(m_document->textLength(), pos + 1), true);
    ensurePositionVisible(m_cursorManager->cursorPosition());
}

void TextRenderer::handleSelectUp()
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Up);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, true);  // extend = true
        ensurePositionVisible(targetPos);
    }
}

void TextRenderer::handleSelectDown()
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Down);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, true);  // extend = true
        ensurePositionVisible(targetPos);
    }
}

void TextRenderer::handleSelectAll()
{
    if (!m_selectionManager || !m_document) return;

    m_selectionManager->selectAll(m_document->textLength());
}

void TextRenderer::handleSelectWord()
{
    if (!m_selectionManager || !m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_selectionManager->selectWord(pos);
}

void TextRenderer::handleSelectLine()
{
    if (!m_selectionManager || !m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(pos);
    m_selectionManager->selectLine(line);
}

void TextRenderer::handleInsertText(const QString& text)
{
    if (!m_cursorManager || !m_document || text.isEmpty()) return;

    int pos = m_cursorManager->cursorPosition();
    m_document->insertText(pos, text);
    m_cursorManager->setCursorPosition(pos + text.length());
    ensurePositionVisible(m_cursorManager->cursorPosition());
}

void TextRenderer::handleNewLine()
{
    handleInsertText("\n");
}

void TextRenderer::handleTab()
{
    handleInsertText("\t");
}

void TextRenderer::handleDeleteLeft()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    if (pos > 0) {
        m_document->removeText(pos - 1, 1);
        m_cursorManager->setCursorPosition(pos - 1);
        ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void TextRenderer::handleDeleteRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    if (pos < m_document->textLength()) {
        m_document->removeText(pos, 1);
        ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void TextRenderer::handleDeleteWordLeft()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int wordStart = findWordBoundary(currentPos, false);

    if (wordStart < currentPos) {
        m_document->removeText(wordStart, currentPos - wordStart);
        m_cursorManager->setCursorPosition(wordStart);
        ensurePositionVisible(wordStart);
    }
}

void TextRenderer::handleDeleteWordRight()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int wordEnd = findWordBoundary(currentPos, true);

    if (wordEnd > currentPos) {
        m_document->removeText(currentPos, wordEnd - currentPos);
        ensurePositionVisible(currentPos);
    }
}

void TextRenderer::handleCut()
{
    handleCopy();
    if (m_selectionManager && m_selectionManager->hasSelection()) {
        // 删除选中文本的逻辑
        auto selections = m_selectionManager->selections();
        for (const auto& selection : selections) {
            m_document->removeText(selection.start, selection.end - selection.start);
        }
        m_selectionManager->clearSelections();
    }
}

void TextRenderer::handleCopy()
{
    if (!m_selectionManager || !m_selectionManager->hasSelection()) return;

    // 获取选中文本并复制到剪贴板
    auto selections = m_selectionManager->selections();
    QStringList texts;
    for (const auto& selection : selections) {
        QString selectedText = m_document->getText(selection.start, selection.end - selection.start);
        texts.append(selectedText);
    }

    QString clipboardText = texts.join("\n");
    QGuiApplication::clipboard()->setText(clipboardText);
}

void TextRenderer::handlePaste()
{
    QString clipboardText = QGuiApplication::clipboard()->text();
    if (!clipboardText.isEmpty()) {
        handleInsertText(clipboardText);
    }
}

void TextRenderer::handleUndo()
{
    if (m_document) {
        m_document->undo();
    }
}

void TextRenderer::handleRedo()
{
    if (m_document) {
        m_document->redo();
    }
}

*/