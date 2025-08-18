#include "TextEditorController.h"
#include "../render/TextRenderer.h"
#include "../interaction/InputHandler.h"
#include "../interaction/InputManager.h"
#include "../core/DocumentModel.h"

#include <QKeyEvent>
#include <QInputMethodEvent>

TextEditorController::TextEditorController(QObject* parent)
    : QObject(parent)
    , m_inputHandler(new InputHandler(this))
    , m_inputManager(new InputManager(this))
{
    setupInputManager();
}

TextEditorController::~TextEditorController()
{
}

TextRenderer* TextEditorController::renderer() const
{
    return m_renderer;
}

void TextEditorController::setRenderer(TextRenderer* renderer)
{
    if (m_renderer == renderer)
        return;

    if (m_renderer) {
        m_renderer->removeEventFilter(this);
        disconnect(m_renderer, nullptr, this, nullptr);
    }

    m_renderer = renderer;

    if (m_renderer) {
        m_renderer->installEventFilter(this);

        // 连接渲染器的信号到命令处理器
        connect(m_renderer, &TextRenderer::mousePressed,
            m_inputHandler, &InputHandler::handleMousePress);
        connect(m_renderer, &TextRenderer::mouseMoved,
            m_inputHandler, &InputHandler::handleMouseMove);
        connect(m_renderer, &TextRenderer::mouseReleased,
            m_inputHandler, &InputHandler::handleMouseRelease);
        connect(m_renderer, &TextRenderer::mouseDoubleClicked,
            m_inputHandler, &InputHandler::handleMouseDoubleClick);

        // 设置命令处理器的渲染器
        m_inputHandler->setRenderer(m_renderer);

        // 设置其他依赖组件
        if (m_renderer->cursorManager()) {
            m_inputHandler->setCursorManager(m_renderer->cursorManager());
        }
        if (m_renderer->selectionManager()) {
            m_inputHandler->setSelectionManager(m_renderer->selectionManager());
        }
    }

    emit rendererChanged();
}

DocumentModel* TextEditorController::document() const
{
    return m_document;
}

void TextEditorController::setDocument(DocumentModel* document)
{
    if (m_document == document)
        return;

    m_document = document;

    if (m_renderer) {
        m_renderer->setDocument(document);
    }

    m_inputHandler->setDocument(document);

    emit documentChanged();
}

bool TextEditorController::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_renderer) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            return m_inputHandler->handleKeyEvent(keyEvent);
        }
        else if (event->type() == QEvent::InputMethod) {
            QInputMethodEvent* inputEvent = static_cast<QInputMethodEvent*>(event);
            return m_inputHandler->handleInputMethodEvent(inputEvent);
        }
    }
    return QObject::eventFilter(watched, event);
}

void TextEditorController::setupInputManager()
{
    m_inputHandler->setInputManager(m_inputManager);
}

// 暴露给QML的命令方法实现
void TextEditorController::moveCursorLeft()
{
    m_inputHandler->handleMoveCursorLeft();
}

void TextEditorController::moveCursorRight()
{
    m_inputHandler->handleMoveCursorRight();
}

void TextEditorController::moveCursorUp()
{
    m_inputHandler->handleMoveCursorUp();
}

void TextEditorController::moveCursorDown()
{
    m_inputHandler->handleMoveCursorDown();
}

void TextEditorController::moveCursorWordLeft()
{
    m_inputHandler->handleMoveCursorWordLeft();
}

void TextEditorController::moveCursorWordRight()
{
    m_inputHandler->handleMoveCursorWordRight();
}

void TextEditorController::moveCursorLineStart()
{
    m_inputHandler->handleMoveCursorLineStart();
}

void TextEditorController::moveCursorLineEnd()
{
    m_inputHandler->handleMoveCursorLineEnd();
}

void TextEditorController::moveCursorDocumentStart()
{
    m_inputHandler->handleMoveCursorDocumentStart();
}

void TextEditorController::moveCursorDocumentEnd()
{
    m_inputHandler->handleMoveCursorDocumentEnd();
}

void TextEditorController::selectLeft()
{
    m_inputHandler->handleSelectLeft();
}

void TextEditorController::selectRight()
{
    m_inputHandler->handleSelectRight();
}

void TextEditorController::selectUp()
{
    m_inputHandler->handleSelectUp();
}

void TextEditorController::selectDown()
{
    m_inputHandler->handleSelectDown();
}

void TextEditorController::selectAll()
{
    m_inputHandler->handleSelectAll();
}

void TextEditorController::selectWord()
{
    m_inputHandler->handleSelectWord();
}

void TextEditorController::selectLine()
{
    m_inputHandler->handleSelectLine();
}

void TextEditorController::insertText(const QString& text)
{
    m_inputHandler->handleInsertText(text);
}

void TextEditorController::newLine()
{
    m_inputHandler->handleNewLine();
}

void TextEditorController::tab()
{
    m_inputHandler->handleTab();
}

void TextEditorController::deleteLeft()
{
    m_inputHandler->handleDeleteLeft();
}

void TextEditorController::deleteRight()
{
    m_inputHandler->handleDeleteRight();
}

void TextEditorController::deleteWordLeft()
{
    m_inputHandler->handleDeleteWordLeft();
}

void TextEditorController::deleteWordRight()
{
    m_inputHandler->handleDeleteWordRight();
}

void TextEditorController::cut()
{
    m_inputHandler->handleCut();
}

void TextEditorController::copy()
{
    m_inputHandler->handleCopy();
}

void TextEditorController::paste()
{
    m_inputHandler->handlePaste();
}

void TextEditorController::undo()
{
    m_inputHandler->handleUndo();
}

void TextEditorController::redo()
{
    m_inputHandler->handleRedo();
}