#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QKeySequence>
#include <QHash>
#include <QPoint>
#include <QDateTime>
#include <functional>

// 编辑命令枚举
enum class EditCommand {
    None,
    // 光标移动
    MoveCursorLeft,
    MoveCursorRight,
    MoveCursorUp,
    MoveCursorDown,
    MoveCursorWordLeft,
    MoveCursorWordRight,
    MoveCursorLineStart,
    MoveCursorLineEnd,
    MoveCursorDocumentStart,
    MoveCursorDocumentEnd,
    MoveCursorPageUp,
    MoveCursorPageDown,

    // 选择
    SelectLeft,
    SelectRight,
    SelectUp,
    SelectDown,
    SelectWordLeft,
    SelectWordRight,
    SelectLineStart,
    SelectLineEnd,
    SelectAll,
    SelectLine,
    SelectWord,

    // 编辑
    InsertText,
    DeleteLeft,
    DeleteRight,
    DeleteWordLeft,
    DeleteWordRight,
    DeleteLine,

    // 剪贴板
    Cut,
    Copy,
    Paste,

    // 撤销/重做
    Undo,
    Redo,

    // 其他
    NewLine,
    Tab,
    Indent,
    Unindent
};

using CommandHandler = std::function<void(const QString& parameter)>;

// 输入管理器
class InputManager : public QObject {
    Q_OBJECT

public:
    explicit InputManager(QObject* parent = nullptr);

    // 事件处理
    bool handleKeyEvent(QKeyEvent* event);
    bool handleMouseEvent(QMouseEvent* event);
    bool handleWheelEvent(QWheelEvent* event);
    bool handleInputMethodEvent(QInputMethodEvent* event);

    // 快捷键管理
    void registerShortcut(const QKeySequence& shortcut, EditCommand command);
    void registerShortcut(const QKeySequence& shortcut, const QString& commandName);
    void unregisterShortcut(const QKeySequence& shortcut);

    // 命令处理器注册
    void registerCommandHandler(EditCommand command, CommandHandler handler);
    void registerCommandHandler(const QString& commandName, CommandHandler handler);

    // 输入法支持
    void setInputMethodEnabled(bool enabled);
    bool isInputMethodEnabled() const;

    // 鼠标操作配置
    void setClickThreshold(int milliseconds);
    void setDoubleClickInterval(int milliseconds);
    void setDragThreshold(int pixels);

    // 扩展功能
    bool handleKeySequence(const QList<QKeyEvent*>& events);
    void setVimMode(bool enabled);
    void setEmacsMode(bool enabled);
    void startMacroRecording(const QString& macroName);
    void stopMacroRecording();
    void playMacro(const QString& macroName);
    void setContext(const QString& context);
    void enableGestureSupport(bool enabled);
    bool handleGesture(const QString& gesture);
    void setAutoCompletionEnabled(bool enabled);
    bool shouldTriggerAutoCompletion(QKeyEvent* event) const;
    QString getShortcutInfo() const;
    void logKeyEvent(QKeyEvent* event) const;

signals:
    void commandTriggered(EditCommand command, const QString& parameter = QString());
    void customCommandTriggered(const QString& commandName, const QString& parameter = QString());

    void cursorMoveRequested(int position, bool select = false);
    void textInsertRequested(int position, const QString& text);
    void textDeleteRequested(int position, int length);
    void selectionChangeRequested(int start, int end);

    void mousePressed(QMouseEvent* event);
    void mouseReleased(QMouseEvent* event);
    void mouseMoved(QMouseEvent* event);
    void mouseDoubleClicked(QMouseEvent* event);
    void wheelScrolled(QWheelEvent* event);

private slots:
    bool handleMousePress(QMouseEvent* event);
    bool handleMouseRelease(QMouseEvent* event);
    bool handleMouseMove(QMouseEvent* event);
    bool handleMouseDoubleClick(QMouseEvent* event);

private:
    QHash<QKeySequence, EditCommand> m_shortcuts;
    QHash<QKeySequence, QString> m_customShortcuts;
    QHash<EditCommand, CommandHandler> m_commandHandlers;
    QHash<QString, CommandHandler> m_customCommandHandlers;

    bool m_inputMethodEnabled = true;
    int m_clickThreshold = 500;
    int m_doubleClickInterval = 400;
    int m_dragThreshold = 4;

    QPoint m_lastMousePos;
    Qt::MouseButton m_pressedButton = Qt::NoButton;
    QDateTime m_lastClickTime;
    bool m_dragging = false;

    // 扩展功能成员变量
    bool m_macroRecording = false;
    QString m_currentMacro;
    QList<QKeyEvent*> m_recordedEvents;
    QHash<QString, QList<QKeyEvent*>> m_macros;
    QString m_currentContext = "editor";
    bool m_gestureEnabled = false;
    bool m_autoCompletionEnabled = true;

    void setupDefaultShortcuts();
    void setupVimShortcuts();
    void setupEmacsShortcuts();
    void setupFindShortcuts();
    void setupReplaceShortcuts();
    EditCommand keyEventToCommand(QKeyEvent* event) const;
    QString extractTextFromKeyEvent(QKeyEvent* event) const;
};

#endif // INPUT_MANAGER_H