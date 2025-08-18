#include "InputHandler.h"
#include "CursorManager.h"
#include "SelectionManager.h"
#include "InputManager.h"
#include "../core/DocumentModel.h"
#include "../service/LayoutEngine.h"
#include "../render/TextRenderer.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QDateTime>
#include <QKeyEvent>
#include <QInputMethodEvent>

InputHandler::InputHandler(QObject* parent)
    : QObject(parent)
{
}

InputHandler::~InputHandler()
{
}

void InputHandler::setDocument(DocumentModel* document)
{
    m_document = document;
}

void InputHandler::setCursorManager(CursorManager* cursorManager)
{
    m_cursorManager = cursorManager;
}

void InputHandler::setSelectionManager(SelectionManager* selectionManager)
{
    m_selectionManager = selectionManager;
}

void InputHandler::setInputManager(InputManager* inputManager)
{
    m_inputManager = inputManager;
    setupInputManager();
}

void InputHandler::setRenderer(TextRenderer* renderer)
{
    m_renderer = renderer;
}

bool InputHandler::handleKeyEvent(QKeyEvent* event)
{
    if (!event || !m_inputManager)
        return false;

    return m_inputManager->handleKeyEvent(event);
}

bool InputHandler::handleInputMethodEvent(QInputMethodEvent* event)
{
    if (!event || !m_document || !m_cursorManager)
        return false;

    QString commitString = event->commitString();
    if (!commitString.isEmpty()) {
        int pos = m_cursorManager->cursorPosition();
        m_document->insertText(pos, commitString);
        m_cursorManager->setCursorPosition(pos + commitString.length());
        if (m_renderer) {
            m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
        }
        return true;
    }

    return false;
}

// 光标移动命令实现
void InputHandler::handleMoveCursorLeft()
{
    if (!m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMax(0, pos - 1), false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void InputHandler::handleMoveCursorRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMin(m_document->textLength(), pos + 1), false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void InputHandler::handleMoveCursorUp()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Up);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, false);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(targetPos);
        }
    }
}

void InputHandler::handleMoveCursorDown()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Down);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, false);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(targetPos);
        }
    }
}

void InputHandler::handleMoveCursorWordLeft()
{
    if (!m_cursorManager || !m_document) return;

    int pos = findWordBoundary(m_cursorManager->cursorPosition(), false);
    m_cursorManager->setCursorPosition(pos, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(pos);
    }
}

void InputHandler::handleMoveCursorWordRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = findWordBoundary(m_cursorManager->cursorPosition(), true);
    m_cursorManager->setCursorPosition(pos, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(pos);
    }
}

void InputHandler::handleMoveCursorLineStart()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(currentPos);
    int lineStartPos = m_document->lineColumnToPosition(line, 0);
    m_cursorManager->setCursorPosition(lineStartPos, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(lineStartPos);
    }
}

void InputHandler::handleMoveCursorLineEnd()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(currentPos);
    QString lineText = m_document->getLine(line);
    int lineEndPos = m_document->lineColumnToPosition(line, lineText.length());
    m_cursorManager->setCursorPosition(lineEndPos, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(lineEndPos);
    }
}

void InputHandler::handleMoveCursorDocumentStart()
{
    if (!m_cursorManager) return;

    m_cursorManager->setCursorPosition(0, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(0);
    }
}

void InputHandler::handleMoveCursorDocumentEnd()
{
    if (!m_cursorManager || !m_document) return;

    int endPos = m_document->textLength();
    m_cursorManager->setCursorPosition(endPos, false);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(endPos);
    }
}

// 选择命令实现
void InputHandler::handleSelectLeft()
{
    if (!m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMax(0, pos - 1), true);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void InputHandler::handleSelectRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    m_cursorManager->setCursorPosition(qMin(m_document->textLength(), pos + 1), true);
    if (m_renderer) {
        m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void InputHandler::handleSelectUp()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Up);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, true); // extend = true
        if (m_renderer) {
            m_renderer->ensurePositionVisible(targetPos);
        }
    }
}

void InputHandler::handleSelectDown()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateVerticalCharacterMovement(currentPos, MoveDirection::Down);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, true); // extend = true
        if (m_renderer) {
            m_renderer->ensurePositionVisible(targetPos);
        }
    }
}

void InputHandler::handleSelectAll()
{
    if (!m_selectionManager || !m_document) return;

    m_selectionManager->selectAll(m_document->textLength());
}

void InputHandler::handleSelectWord()
{
    if (!m_selectionManager || !m_cursorManager) return;

    int pos = m_cursorManager->cursorPosition();
    m_selectionManager->selectWord(pos);
}

void InputHandler::handleSelectLine()
{
    if (!m_selectionManager || !m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    int line = m_document->positionToLine(pos);
    m_selectionManager->selectLine(line);
}

// 编辑命令实现
void InputHandler::handleInsertText(const QString& text)
{
    if (!m_cursorManager || !m_document || text.isEmpty()) return;

    int pos = m_cursorManager->cursorPosition();
    m_document->insertText(pos, text);
    m_cursorManager->setCursorPosition(pos + text.length());
    if (m_renderer) {
        m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
    }
}

void InputHandler::handleNewLine()
{
    handleInsertText("\n");
}

void InputHandler::handleTab()
{
    handleInsertText("\t");
}

void InputHandler::handleDeleteLeft()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    if (pos > 0) {
        m_document->removeText(pos - 1, 1);
        m_cursorManager->setCursorPosition(pos - 1);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(m_cursorManager->cursorPosition());
        }
    }
}

void InputHandler::handleDeleteRight()
{
    if (!m_cursorManager || !m_document) return;

    int pos = m_cursorManager->cursorPosition();
    if (pos < m_document->textLength()) {
        m_document->removeText(pos, 1);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(pos);
        }
    }
}

void InputHandler::handleDeleteWordLeft()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int wordStart = findWordBoundary(currentPos, false);

    if (wordStart < currentPos) {
        m_document->removeText(wordStart, currentPos - wordStart);
        m_cursorManager->setCursorPosition(wordStart);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(wordStart);
        }
    }
}

void InputHandler::handleDeleteWordRight()
{
    if (!m_cursorManager || !m_document) return;

    int currentPos = m_cursorManager->cursorPosition();
    int wordEnd = findWordBoundary(currentPos, true);

    if (wordEnd > currentPos) {
        m_document->removeText(currentPos, wordEnd - currentPos);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(currentPos);
        }
    }
}

// 剪贴板操作实现
void InputHandler::handleCut()
{
    handleCopy();
    if (m_selectionManager && m_selectionManager->hasSelection()) {
        // 删除选中文本
        auto selections = m_selectionManager->selections();
        for (const auto& selection : selections) {
            m_document->removeText(selection.start, selection.end - selection.start);
        }
        m_selectionManager->clearSelections();
    }
}

void InputHandler::handleCopy()
{
    if (!m_selectionManager || !m_selectionManager->hasSelection() || !m_document) return;

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

void InputHandler::handlePaste()
{
    QString clipboardText = QGuiApplication::clipboard()->text();
    if (!clipboardText.isEmpty()) {
        handleInsertText(clipboardText);
    }
}

// 撤销/重做
void InputHandler::handleUndo()
{
    if (m_document) {
        m_document->undo();
    }
}

void InputHandler::handleRedo()
{
    if (m_document) {
        m_document->redo();
    }
}

// 鼠标事件处理
void InputHandler::handleMousePress(const QPointF& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    if (!m_renderer || !m_document || !m_cursorManager) return;

    int pos = m_renderer->pointToPosition(position);
    bool extend = modifiers & Qt::ShiftModifier;

    m_cursorManager->setCursorPosition(pos, extend);

    // 处理选择
    if (m_selectionManager && !extend) {
        // 简单点击清除选择
        m_selectionManager->clearSelections();
    }
}

void InputHandler::handleMouseMove(const QPointF& position, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if (!m_renderer || !m_cursorManager || !(buttons & Qt::LeftButton)) return;

    int pos = m_renderer->pointToPosition(position);
    m_cursorManager->setCursorPosition(pos, true); // 扩展选择
}

void InputHandler::handleMouseRelease(const QPointF& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    // 处理鼠标释放事件，大多数逻辑在按下和移动中已处理
}

void InputHandler::handleMouseDoubleClick(const QPointF& position, Qt::MouseButton button)
{
    if (!m_renderer || !m_selectionManager) return;

    int pos = m_renderer->pointToPosition(position);
    m_selectionManager->selectWord(pos);
}

// 设置InputManager
void InputHandler::setupInputManager()
{
    if (!m_inputManager) return;

    // 注册命令处理函数
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

// 移动辅助方法实现 - 从TextRenderer转移过来的代码
void InputHandler::handleMovement(MoveDirection direction, bool extend, MoveUnit unit)
{
    if (!m_cursorManager || !m_document)
        return;

    int currentPos = m_cursorManager->cursorPosition();
    int targetPos = calculateMovementTarget(currentPos, direction, unit);

    if (targetPos != currentPos) {
        m_cursorManager->setCursorPosition(targetPos, extend);
        if (m_renderer) {
            m_renderer->ensurePositionVisible(targetPos);
        }
    }
}

int InputHandler::calculateMovementTarget(int currentPos, MoveDirection direction, MoveUnit unit) const
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

int InputHandler::calculateCharacterMovement(int currentPos, MoveDirection direction) const
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

int InputHandler::calculateWordMovement(int currentPos, MoveDirection direction) const
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

int InputHandler::calculateLineMovement(int currentPos, MoveDirection direction) const
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

int InputHandler::calculatePageMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document || !m_renderer)
        return currentPos;

    int currentLine = m_document->positionToLine(currentPos);
    int currentColumn = m_document->positionToColumn(currentPos);

    // 计算可见行数
    auto layoutEngine = m_renderer->layoutEngine();
    if (!layoutEngine) return currentPos;

    qreal lineHeight = layoutEngine->lineHeight();
    int visibleLines = static_cast<int>(m_renderer->height() / lineHeight);

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

int InputHandler::calculateDocumentMovement(int currentPos, MoveDirection direction) const
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

int InputHandler::calculateVerticalCharacterMovement(int currentPos, MoveDirection direction) const
{
    if (!m_document)
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

int InputHandler::findWordBoundary(int position, bool forward) const
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

int InputHandler::findParagraphBoundary(int position, bool forward) const
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

bool InputHandler::isWordCharacter(QChar ch) const
{
    // 定义单词字符：字母、数字、下划线
    return ch.isLetterOrNumber() || ch == '_';
}