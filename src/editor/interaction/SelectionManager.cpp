#include "SelectionManager.h"
#include "../core/DocumentModel.h"
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

SelectionManager::SelectionManager(QObject* parent)
    : QObject(parent)
{
}

// ==============================================================================
// 文档关联
// ==============================================================================

void SelectionManager::setDocument(DocumentModel* document)
{
    if (m_document == document)
        return;

    if (m_document) {
        // 断开与旧文档的连接
        disconnect(m_document, nullptr, this, nullptr);
    }

    m_document = document;

    if (m_document) {
        // 连接到新文档的信号
        connect(m_document, &DocumentModel::textChanged,
            this, &SelectionManager::onDocumentTextChanged);
    }

    // 清除现有选择
    clearSelections();
}

DocumentModel* SelectionManager::document() const
{
    return m_document;
}

// ==============================================================================
// 选择操作
// ==============================================================================

void SelectionManager::setSelection(const SelectionRange& range)
{
    if (range.isEmpty()) {
        clearSelections();
        return;
    }

    SelectionRange normalizedRange = range.normalized();

    // 替换所有现有选择
    m_selections.clear();
    m_selections.append(normalizedRange);

    emit selectionsChanged(m_selections);
}

void SelectionManager::addSelection(const SelectionRange& range)
{
    if (range.isEmpty())
        return;

    SelectionRange normalizedRange = range.normalized();

    // 检查是否与现有选择重叠
    bool merged = false;
    for (int i = 0; i < m_selections.size(); ++i) {
        SelectionRange& existing = m_selections[i];

        // 如果重叠或相邻，合并选择
        if (normalizedRange.start <= existing.end && normalizedRange.end >= existing.start) {
            existing.start = qMin(existing.start, normalizedRange.start);
            existing.end = qMax(existing.end, normalizedRange.end);
            merged = true;
            break;
        }
    }

    if (!merged) {
        m_selections.append(normalizedRange);
    }

    // 排序并合并重叠的选择
    sortSelections();
    mergeOverlappingSelections();

    emit selectionsChanged(m_selections);
}

void SelectionManager::removeSelection(int index)
{
    if (index < 0 || index >= m_selections.size())
        return;

    m_selections.removeAt(index);
    emit selectionsChanged(m_selections);
}

void SelectionManager::clearSelections()
{
    if (m_selections.isEmpty())
        return;

    m_selections.clear();
    emit selectionsChanged(m_selections);
}

QList<SelectionRange> SelectionManager::selections() const
{
    return m_selections;
}

SelectionRange SelectionManager::primarySelection() const
{
    if (m_selections.isEmpty())
        return SelectionRange(0, 0);

    // 返回第一个选择作为主选择
    return m_selections.first();
}

bool SelectionManager::hasSelection() const
{
    return !m_selections.isEmpty();
}

// ==============================================================================
// 选择模式
// ==============================================================================

void SelectionManager::setSelectionMode(SelectionMode mode)
{
    if (m_selectionMode == mode)
        return;

    m_selectionMode = mode;

    // 如果有现有选择，根据新模式重新调整
    if (!m_selections.isEmpty()) {
        for (SelectionRange& selection : m_selections) {
            selection.mode = mode;

            switch (mode) {
            case SelectionMode::Word:
                expandSelectionToWord(selection);
                break;
            case SelectionMode::Line:
                expandSelectionToLine(selection);
                break;
            case SelectionMode::Character:
            case SelectionMode::Block:
                // 字符模式和块模式不需要扩展
                break;
            }
        }

        emit selectionsChanged(m_selections);
    }

    emit selectionModeChanged(m_selectionMode);
}

SelectionMode SelectionManager::selectionMode() const
{
    return m_selectionMode;
}

// ==============================================================================
// 文本相关选择
// ==============================================================================

void SelectionManager::selectWord(int position)
{
    if (!m_document) {
        qWarning() << "SelectionManager: 没有设置文档模型";
        return;
    }

    QString text = m_document->getFullText();
    if (position < 0 || position >= text.length())
        return;

    // 找到单词边界
    int start = position;
    int end = position;

    // 向左找单词开始
    while (start > 0 && (text.at(start - 1).isLetterOrNumber() || text.at(start - 1) == '_')) {
        start--;
    }

    // 向右找单词结束
    while (end < text.length() && (text.at(end).isLetterOrNumber() || text.at(end) == '_')) {
        end++;
    }

    if (start < end) {
        SelectionRange range(start, end, SelectionMode::Word);
        setSelection(range);
    }
}

void SelectionManager::selectLine(int lineNumber)
{
    if (!m_document) {
        qWarning() << "SelectionManager: 没有设置文档模型";
        return;
    }

    if (lineNumber < 0 || lineNumber >= m_document->lineCount())
        return;

    int lineStart = m_document->lineColumnToPosition(lineNumber, 0);
    int lineEnd;

    if (lineNumber == m_document->lineCount() - 1) {
        // 最后一行
        lineEnd = m_document->textLength();
    }
    else {
        // 包含换行符
        lineEnd = m_document->lineColumnToPosition(lineNumber + 1, 0);
    }

    SelectionRange range(lineStart, lineEnd, SelectionMode::Line);
    setSelection(range);
}

void SelectionManager::selectParagraph(int position)
{
    if (!m_document) {
        qWarning() << "SelectionManager: 没有设置文档模型";
        return;
    }

    QString text = m_document->getFullText();
    if (position < 0 || position >= text.length())
        return;

    // 找到段落边界（以空行分隔）
    int start = position;
    int end = position;

    // 向上找段落开始
    while (start > 0) {
        if (text.at(start - 1) == '\n') {
            // 检查是否为空行
            bool isEmptyLine = true;
            int lineStart = start;
            while (lineStart < text.length() && text.at(lineStart) != '\n') {
                if (!text.at(lineStart).isSpace()) {
                    isEmptyLine = false;
                    break;
                }
                lineStart++;
            }
            if (isEmptyLine) {
                break;
            }
        }
        start--;
    }

    // 向下找段落结束
    while (end < text.length()) {
        if (text.at(end) == '\n') {
            // 检查下一行是否为空行
            int nextLineStart = end + 1;
            bool isEmptyLine = true;
            while (nextLineStart < text.length() && text.at(nextLineStart) != '\n') {
                if (!text.at(nextLineStart).isSpace()) {
                    isEmptyLine = false;
                    break;
                }
                nextLineStart++;
            }
            if (isEmptyLine) {
                end++; // 包含当前换行符
                break;
            }
        }
        end++;
    }

    SelectionRange range(start, end, SelectionMode::Character);
    setSelection(range);
}

void SelectionManager::selectAll(int textLength)
{
    if (textLength <= 0)
        return;

    SelectionRange range(0, textLength, SelectionMode::Character);
    setSelection(range);
}

// ==============================================================================
// 块选择（矩形选择）
// ==============================================================================

void SelectionManager::startBlockSelection(int startLine, int startColumn)
{
    m_blockSelectionActive = true;
    m_blockStartLine = startLine;
    m_blockStartColumn = startColumn;
    m_blockEndLine = startLine;
    m_blockEndColumn = startColumn;

    emit blockSelectionChanged(true);
}

void SelectionManager::updateBlockSelection(int endLine, int endColumn)
{
    if (!m_blockSelectionActive)
        return;

    m_blockEndLine = endLine;
    m_blockEndColumn = endColumn;

    // 创建块选择
    QList<SelectionRange> blockSelections = createBlockSelection();

    // 替换当前选择
    m_selections = blockSelections;
    emit selectionsChanged(m_selections);
}

void SelectionManager::endBlockSelection()
{
    if (!m_blockSelectionActive)
        return;

    m_blockSelectionActive = false;
    emit blockSelectionChanged(false);

    // 最终的块选择已经在 updateBlockSelection 中设置
}

bool SelectionManager::isBlockSelectionActive() const
{
    return m_blockSelectionActive;
}

// ==============================================================================
// 选择扩展
// ==============================================================================

void SelectionManager::extendSelectionToWord(int position)
{
    if (m_selections.isEmpty()) {
        selectWord(position);
        return;
    }

    // 扩展第一个选择到单词边界
    SelectionRange& selection = m_selections.first();

    if (position < selection.start) {
        // 向左扩展
        selectWord(position);
        if (!m_selections.isEmpty()) {
            SelectionRange wordSelection = m_selections.first();
            selection.start = wordSelection.start;
        }
    }
    else if (position > selection.end) {
        // 向右扩展
        selectWord(position);
        if (!m_selections.isEmpty()) {
            SelectionRange wordSelection = m_selections.first();
            selection.end = wordSelection.end;
        }
    }

    emit selectionsChanged(m_selections);
}

void SelectionManager::extendSelectionToLine(int lineNumber)
{
    if (!m_document) {
        qWarning() << "SelectionManager: 没有设置文档模型";
        return;
    }

    if (m_selections.isEmpty()) {
        selectLine(lineNumber);
        return;
    }

    // 扩展第一个选择到行边界
    SelectionRange& selection = m_selections.first();

    int lineStart = m_document->lineColumnToPosition(lineNumber, 0);
    int lineEnd;

    if (lineNumber == m_document->lineCount() - 1) {
        lineEnd = m_document->textLength();
    }
    else {
        lineEnd = m_document->lineColumnToPosition(lineNumber + 1, 0);
    }

    if (lineStart < selection.start) {
        selection.start = lineStart;
    }
    if (lineEnd > selection.end) {
        selection.end = lineEnd;
    }

    emit selectionsChanged(m_selections);
}

void SelectionManager::extendSelectionTo(int position)
{
    if (m_selections.isEmpty()) {
        // 创建新选择
        SelectionRange range(position, position);
        setSelection(range);
        return;
    }

    // 扩展第一个选择
    SelectionRange& selection = m_selections.first();

    if (position < selection.start) {
        selection.start = position;
    }
    else if (position > selection.end) {
        selection.end = position;
    }
    else {
        // 位置在选择内部，调整最近的边界
        int distToStart = qAbs(position - selection.start);
        int distToEnd = qAbs(position - selection.end);

        if (distToStart < distToEnd) {
            selection.start = position;
        }
        else {
            selection.end = position;
        }
    }

    emit selectionsChanged(m_selections);
}

// ==============================================================================
// 选择合并
// ==============================================================================

void SelectionManager::mergeOverlappingSelections()
{
    if (m_selections.size() <= 1)
        return;

    sortSelections();

    QList<SelectionRange> merged;

    for (const SelectionRange& current : m_selections) {
        if (merged.isEmpty()) {
            merged.append(current);
        }
        else {
            SelectionRange& last = merged.last();

            // 检查是否重叠或相邻
            if (current.start <= last.end) {
                // 合并
                last.end = qMax(last.end, current.end);
            }
            else {
                // 不重叠，添加新选择
                merged.append(current);
            }
        }
    }

    if (merged.size() != m_selections.size()) {
        m_selections = merged;
        emit selectionsChanged(m_selections);
    }
}

void SelectionManager::sortSelections()
{
    if (m_selections.size() <= 1)
        return;

    std::sort(m_selections.begin(), m_selections.end(),
        [](const SelectionRange& a, const SelectionRange& b) {
            return a.start < b.start;
        });
}

// ==============================================================================
// 视觉效果
// ==============================================================================

QList<QRect> SelectionManager::getSelectionRects() const
{
    QList<QRect> rects;

    if (!m_document) {
        return rects;
    }

    for (const SelectionRange& selection : m_selections) {
        if (selection.isEmpty())
            continue;

        // 这里需要与布局引擎配合来计算实际的屏幕矩形
        // 暂时返回逻辑位置
        QRect rect;
        rect.setLeft(selection.start);
        rect.setRight(selection.end);
        rects.append(rect);
    }

    return rects;
}

void SelectionManager::setSelectionColor(const QColor& color)
{
    if (m_selectionColor == color)
        return;

    m_selectionColor = color;
    // 可以发出信号通知渲染器更新颜色
}

QColor SelectionManager::selectionColor() const
{
    return m_selectionColor;
}

// ==============================================================================
// 剪贴板支持
// ==============================================================================

QString SelectionManager::getSelectedText() const
{
    if (!m_document || m_selections.isEmpty())
        return QString();

    QStringList texts;
    for (const SelectionRange& selection : m_selections) {
        if (!selection.isEmpty()) {
            QString text = m_document->getText(selection.start, selection.length());
            texts.append(text);
        }
    }

    return texts.join("\n");
}

QStringList SelectionManager::getSelectedTexts() const
{
    QStringList texts;

    if (!m_document)
        return texts;

    for (const SelectionRange& selection : m_selections) {
        if (!selection.isEmpty()) {
            QString text = m_document->getText(selection.start, selection.length());
            texts.append(text);
        }
    }

    return texts;
}

// ==============================================================================
// 高级选择功能
// ==============================================================================

// 选择所有匹配的文本
void SelectionManager::selectAllMatches(const QString& pattern, bool caseSensitive, bool wholeWords)
{
    if (!m_document || pattern.isEmpty())
        return;

    QList<int> matches = m_document->findText(pattern, caseSensitive, wholeWords);

    m_selections.clear();
    for (int position : matches) {
        SelectionRange range(position, position + pattern.length());
        m_selections.append(range);
    }

    emit selectionsChanged(m_selections);
}

// 反转选择
void SelectionManager::invertSelection(int textLength)
{
    if (!m_document)
        return;

    if (m_selections.isEmpty()) {
        selectAll(textLength);
        return;
    }

    QList<SelectionRange> inverted;
    sortSelections();

    int currentPos = 0;
    for (const SelectionRange& selection : m_selections) {
        if (currentPos < selection.start) {
            inverted.append(SelectionRange(currentPos, selection.start));
        }
        currentPos = selection.end;
    }

    if (currentPos < textLength) {
        inverted.append(SelectionRange(currentPos, textLength));
    }

    m_selections = inverted;
    emit selectionsChanged(m_selections);
}

// 选择范围内的单词
void SelectionManager::selectWordsInRange(int start, int end)
{
    if (!m_document || start >= end)
        return;

    QString text = m_document->getText(start, end - start);
    QRegularExpression wordRegex("\\b\\w+\\b");

    m_selections.clear();

    QRegularExpressionMatchIterator iterator = wordRegex.globalMatch(text);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int wordStart = start + match.capturedStart();
        int wordEnd = start + match.capturedEnd();

        SelectionRange range(wordStart, wordEnd, SelectionMode::Word);
        m_selections.append(range);
    }

    emit selectionsChanged(m_selections);
}

// 智能选择（根据光标位置智能选择内容）
void SelectionManager::smartSelect(int position)
{
    if (!m_document)
        return;

    QString text = m_document->getFullText();
    if (position < 0 || position >= text.length())
        return;

    QChar ch = text.at(position);

    // 根据字符类型智能选择
    if (ch.isLetterOrNumber() || ch == '_') {
        // 选择单词
        selectWord(position);
    }
    else if (ch == '"' || ch == '\'' || ch == '`') {
        // 选择引号内的内容
        selectQuotedText(position, ch);
    }
    else if (ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
        ch == '{' || ch == '}' || ch == '<' || ch == '>') {
        // 选择括号内的内容
        selectBracketContent(position);
    }
    else {
        // 默认选择当前行
        int line = m_document->positionToLine(position);
        selectLine(line);
    }
}

// ==============================================================================
// 文档变更处理
// ==============================================================================

void SelectionManager::onDocumentTextChanged(const TextChange& change)
{
    if (m_selections.isEmpty())
        return;

    // 调整选择位置以适应文本变更
    for (SelectionRange& selection : m_selections) {
        adjustSelectionForTextChange(selection, change);
    }

    // 移除无效的选择
    m_selections.erase(
        std::remove_if(m_selections.begin(), m_selections.end(),
            [](const SelectionRange& range) { return range.isEmpty(); }),
        m_selections.end());

    emit selectionsChanged(m_selections);
}

void SelectionManager::adjustSelectionForTextChange(SelectionRange& selection, const TextChange& change)
{
    int changePos = change.position;
    int removedLength = change.removedLength;
    int insertedLength = change.insertedText.length();
    int netChange = insertedLength - removedLength;

    if (changePos <= selection.start) {
        // 变更在选择之前
        selection.start += netChange;
        selection.end += netChange;
    }
    else if (changePos < selection.end) {
        // 变更在选择内部
        if (changePos + removedLength <= selection.end) {
            // 变更完全在选择内部
            selection.end += netChange;
        }
        else {
            // 变更部分超出选择范围
            selection.end = changePos + insertedLength;
        }
    }
    // 如果变更在选择之后，不需要调整

    // 确保选择范围有效
    selection.start = qMax(0, selection.start);
    selection.end = qMax(selection.start, selection.end);
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

void SelectionManager::expandSelectionToWord(SelectionRange& range)
{
    if (!m_document)
        return;

    QString text = m_document->getFullText();

    // 扩展开始位置到单词边界
    int start = range.start;
    while (start > 0 && (text.at(start - 1).isLetterOrNumber() || text.at(start - 1) == '_')) {
        start--;
    }

    // 扩展结束位置到单词边界
    int end = range.end;
    while (end < text.length() && (text.at(end).isLetterOrNumber() || text.at(end) == '_')) {
        end++;
    }

    range.start = start;
    range.end = end;
}

void SelectionManager::expandSelectionToLine(SelectionRange& range)
{
    if (!m_document)
        return;

    // 获取选择范围涉及的行
    int startLine = m_document->positionToLine(range.start);
    int endLine = m_document->positionToLine(range.end);

    // 扩展到完整行
    range.start = m_document->lineColumnToPosition(startLine, 0);

    if (endLine == m_document->lineCount() - 1) {
        range.end = m_document->textLength();
    }
    else {
        range.end = m_document->lineColumnToPosition(endLine + 1, 0);
    }
}

QList<SelectionRange> SelectionManager::createBlockSelection() const
{
    QList<SelectionRange> selections;

    if (!m_document || !m_blockSelectionActive)
        return selections;

    int startLine = qMin(m_blockStartLine, m_blockEndLine);
    int endLine = qMax(m_blockStartLine, m_blockEndLine);
    int startColumn = qMin(m_blockStartColumn, m_blockEndColumn);
    int endColumn = qMax(m_blockStartColumn, m_blockEndColumn);

    for (int line = startLine; line <= endLine; ++line) {
        if (line >= m_document->lineCount())
            break;

        QString lineText = m_document->getLine(line);
        int lineLength = lineText.length();

        // 调整列范围以适应行长度
        int actualStartColumn = qMin(startColumn, lineLength);
        int actualEndColumn = qMin(endColumn, lineLength);

        if (actualStartColumn < actualEndColumn) {
            int startPos = m_document->lineColumnToPosition(line, actualStartColumn);
            int endPos = m_document->lineColumnToPosition(line, actualEndColumn);

            SelectionRange range(startPos, endPos, SelectionMode::Block);
            selections.append(range);
        }
    }

    return selections;
}

void SelectionManager::selectQuotedText(int position, QChar quote)
{
    if (!m_document)
        return;

    QString text = m_document->getFullText();

    // 找到引号对
    int start = position;
    int end = position + 1;

    // 向左找开始引号
    while (start > 0 && text.at(start - 1) != quote) {
        start--;
    }
    if (start > 0) start--; // 包含开始引号

    // 向右找结束引号
    while (end < text.length() && text.at(end) != quote) {
        end++;
    }
    if (end < text.length()) end++; // 包含结束引号

    if (start < end) {
        SelectionRange range(start, end);
        setSelection(range);
    }
}

void SelectionManager::selectBracketContent(int position)
{
    if (!m_document)
        return;

    QString text = m_document->getFullText();
    QChar ch = text.at(position);

    QChar openBracket, closeBracket;

    // 确定括号类型
    if (ch == '(' || ch == ')') {
        openBracket = '(';
        closeBracket = ')';
    }
    else if (ch == '[' || ch == ']') {
        openBracket = '[';
        closeBracket = ']';
    }
    else if (ch == '{' || ch == '}') {
        openBracket = '{';
        closeBracket = '}';
    }
    else if (ch == '<' || ch == '>') {
        openBracket = '<';
        closeBracket = '>';
    }
    else {
        return;
    }

    // 找到匹配的括号对
    int start = position;
    int end = position;
    int depth = 0;

    // 如果当前是闭括号，向左找
    if (ch == closeBracket) {
        depth = 1;
        start--;
        while (start >= 0 && depth > 0) {
            if (text.at(start) == closeBracket) {
                depth++;
            }
            else if (text.at(start) == openBracket) {
                depth--;
            }
            if (depth > 0) start--;
        }
        end = position + 1;
    }
    else {
        // 当前是开括号，向右找
        depth = 1;
        end++;
        while (end < text.length() && depth > 0) {
            if (text.at(end) == openBracket) {
                depth++;
            }
            else if (text.at(end) == closeBracket) {
                depth--;
            }
            if (depth > 0) end++;
        }
        if (depth == 0) end++;
    }

    if (start < end) {
        SelectionRange range(start, end);
        setSelection(range);
    }
}

// 调试方法
QString SelectionManager::debugString() const
{
    QStringList parts;

    parts << QString("SelectionManager: mode=%1, blockActive=%2")
        .arg(static_cast<int>(m_selectionMode))
        .arg(m_blockSelectionActive);

    for (int i = 0; i < m_selections.size(); ++i) {
        const SelectionRange& selection = m_selections[i];
        parts << QString("  Selection[%1]: [%2,%3] mode=%4 length=%5")
            .arg(i)
            .arg(selection.start)
            .arg(selection.end)
            .arg(static_cast<int>(selection.mode))
            .arg(selection.length());
    }

    if (m_blockSelectionActive) {
        parts << QString("  BlockSelection: start=(%1,%2) end=(%3,%4)")
            .arg(m_blockStartLine).arg(m_blockStartColumn)
            .arg(m_blockEndLine).arg(m_blockEndColumn);
    }

    return parts.join("\n");
}