#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <QObject>
#include <QString>
#include <QKeyEvent>
#include <QInputMethodEvent>
#include <functional>

class DocumentModel;
class CursorManager;
class SelectionManager;
class InputManager;
class TextRenderer;

// 命令处理器 - 处理所有编辑命令
class InputHandler : public QObject {
    Q_OBJECT

public:
    explicit InputHandler(QObject* parent = nullptr);
    ~InputHandler();

    // 设置依赖组件
    void setDocument(DocumentModel* document);
    void setCursorManager(CursorManager* cursorManager);
    void setSelectionManager(SelectionManager* selectionManager);
    void setInputManager(InputManager* inputManager);
    void setRenderer(TextRenderer* renderer);

    // 事件处理
    bool handleKeyEvent(QKeyEvent* event);
    bool handleInputMethodEvent(QInputMethodEvent* event);

public slots:
    // 光标移动命令
    void handleMoveCursorLeft();
    void handleMoveCursorRight();
    void handleMoveCursorUp();
    void handleMoveCursorDown();
    void handleMoveCursorWordLeft();
    void handleMoveCursorWordRight();
    void handleMoveCursorLineStart();
    void handleMoveCursorLineEnd();
    void handleMoveCursorDocumentStart();
    void handleMoveCursorDocumentEnd();

    // 选择命令
    void handleSelectLeft();
    void handleSelectRight();
    void handleSelectUp();
    void handleSelectDown();
    void handleSelectAll();
    void handleSelectWord();
    void handleSelectLine();

    // 编辑命令
    void handleInsertText(const QString& text);
    void handleNewLine();
    void handleTab();
    void handleDeleteLeft();
    void handleDeleteRight();
    void handleDeleteWordLeft();
    void handleDeleteWordRight();

    // 剪贴板操作
    void handleCut();
    void handleCopy();
    void handlePaste();

    // 撤销/重做
    void handleUndo();
    void handleRedo();

    // 鼠标事件处理
    void handleMousePress(const QPointF& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    void handleMouseMove(const QPointF& position, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);
    void handleMouseRelease(const QPointF& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    void handleMouseDoubleClick(const QPointF& position, Qt::MouseButton button);

private:
    DocumentModel* m_document = nullptr;
    CursorManager* m_cursorManager = nullptr;
    SelectionManager* m_selectionManager = nullptr;
    InputManager* m_inputManager = nullptr;
    TextRenderer* m_renderer = nullptr;

    // 移动方向枚举
    enum class MoveDirection {
        Left,
        Right,
        Up,
        Down
    };

    // 移动单位枚举
    enum class MoveUnit {
        Character,    // 字符级
        Word,         // 单词级
        Line,         // 行级（到行首/行尾）
        Page,         // 页面级
        Document      // 文档级
    };

    // 私有辅助方法
    void handleMovement(MoveDirection direction, bool extend, MoveUnit unit = MoveUnit::Character);
    int calculateMovementTarget(int currentPos, MoveDirection direction, MoveUnit unit) const;
    int calculateCharacterMovement(int currentPos, MoveDirection direction) const;
    int calculateWordMovement(int currentPos, MoveDirection direction) const;
    int calculateLineMovement(int currentPos, MoveDirection direction) const;
    int calculatePageMovement(int currentPos, MoveDirection direction) const;
    int calculateDocumentMovement(int currentPos, MoveDirection direction) const;
    int calculateVerticalCharacterMovement(int currentPos, MoveDirection direction) const;
    int findWordBoundary(int position, bool forward) const;
    int findParagraphBoundary(int position, bool forward) const;
    bool isWordCharacter(QChar ch) const;

    void setupInputManager();
};

#endif // COMMAND_HANDLER_H