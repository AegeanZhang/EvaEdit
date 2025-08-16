#include "InputManager.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>
#include <QDateTime>

InputManager::InputManager(QObject* parent)
    : QObject(parent)
    , m_lastClickTime(QDateTime::currentDateTime())
{
    setupDefaultShortcuts();
}

// ==============================================================================
// 事件处理
// ==============================================================================

bool InputManager::handleKeyEvent(QKeyEvent* event)
{
    if (!event)
        return false;

    // 记录宏
    if (m_macroRecording) {
        // 使用正确的 QKeyEvent 构造函数
        QKeyEvent* recordedEvent = new QKeyEvent(
            event->type(),
            event->key(),
            event->modifiers(),
            event->nativeScanCode(),
            event->nativeVirtualKey(),
            event->nativeModifiers(),
            event->text(),
            event->isAutoRepeat(),
            event->count()
        );
        m_recordedEvents.append(recordedEvent);
    }

    // 检查快捷键
    QKeySequence sequence(event->key() | event->modifiers());

    // 处理自定义快捷键
    if (m_customShortcuts.contains(sequence)) {
        QString commandName = m_customShortcuts[sequence];
        if (m_customCommandHandlers.contains(commandName)) {
            m_customCommandHandlers[commandName](QString());
            emit customCommandTriggered(commandName);
            return true;
        }
    }

    // 处理标准快捷键
    if (m_shortcuts.contains(sequence)) {
        EditCommand command = m_shortcuts[sequence];
        QString parameter = extractTextFromKeyEvent(event);

        if (m_commandHandlers.contains(command)) {
            m_commandHandlers[command](parameter);
        }

        emit commandTriggered(command, parameter);
        return true;
    }

    // 处理常规按键
    EditCommand command = keyEventToCommand(event);
    if (command != EditCommand::None) {
        QString parameter = extractTextFromKeyEvent(event);

        if (m_commandHandlers.contains(command)) {
            m_commandHandlers[command](parameter);
        }

        emit commandTriggered(command, parameter);
        return true;
    }

    // 处理文本输入
    QString text = event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        if (m_commandHandlers.contains(EditCommand::InsertText)) {
            m_commandHandlers[EditCommand::InsertText](text);
        }
        emit commandTriggered(EditCommand::InsertText, text);
        return true;
    }

    return false;
}

bool InputManager::handleMouseEvent(QMouseEvent* event)
{
    if (!event)
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return handleMousePress(event);
    case QEvent::MouseButtonRelease:
        return handleMouseRelease(event);
    case QEvent::MouseMove:
        return handleMouseMove(event);
    case QEvent::MouseButtonDblClick:
        return handleMouseDoubleClick(event);
    default:
        break;
    }

    return false;
}

bool InputManager::handleMousePress(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    m_pressedButton = event->button();
    m_dragging = false;

    QDateTime currentTime = QDateTime::currentDateTime();

    // 检查是否为快速连续点击
    if (m_lastClickTime.msecsTo(currentTime) < m_clickThreshold) {
        // 可能是多重点击，等待处理
    }

    m_lastClickTime = currentTime;

    emit mousePressed(event);
    return true;
}

bool InputManager::handleMouseRelease(QMouseEvent* event)
{
    m_pressedButton = Qt::NoButton;
    m_dragging = false;

    emit mouseReleased(event);
    return true;
}

bool InputManager::handleMouseMove(QMouseEvent* event)
{
    if (m_pressedButton != Qt::NoButton) {
        // 检查是否开始拖拽
        int dragDistance = (event->pos() - m_lastMousePos).manhattanLength();
        if (!m_dragging && dragDistance > m_dragThreshold) {
            m_dragging = true;
        }
    }

    emit mouseMoved(event);
    return true;
}

bool InputManager::handleMouseDoubleClick(QMouseEvent* event)
{
    emit mouseDoubleClicked(event);
    return true;
}

bool InputManager::handleWheelEvent(QWheelEvent* event)
{
    if (!event)
        return false;

    emit wheelScrolled(event);
    return true;
}

bool InputManager::handleInputMethodEvent(QInputMethodEvent* event)
{
    if (!event || !m_inputMethodEnabled)
        return false;

    QString commitString = event->commitString();
    if (!commitString.isEmpty()) {
        if (m_commandHandlers.contains(EditCommand::InsertText)) {
            m_commandHandlers[EditCommand::InsertText](commitString);
        }
        emit commandTriggered(EditCommand::InsertText, commitString);
        return true;
    }

    return false;
}

// ==============================================================================
// 快捷键管理
// ==============================================================================

void InputManager::registerShortcut(const QKeySequence& shortcut, EditCommand command)
{
    m_shortcuts[shortcut] = command;
}

void InputManager::registerShortcut(const QKeySequence& shortcut, const QString& commandName)
{
    m_customShortcuts[shortcut] = commandName;
}

void InputManager::unregisterShortcut(const QKeySequence& shortcut)
{
    m_shortcuts.remove(shortcut);
    m_customShortcuts.remove(shortcut);
}

// ==============================================================================
// 命令处理器注册
// ==============================================================================

void InputManager::registerCommandHandler(EditCommand command, CommandHandler handler)
{
    m_commandHandlers[command] = handler;
}

void InputManager::registerCommandHandler(const QString& commandName, CommandHandler handler)
{
    m_customCommandHandlers[commandName] = handler;
}

// ==============================================================================
// 配置方法
// ==============================================================================

void InputManager::setInputMethodEnabled(bool enabled)
{
    m_inputMethodEnabled = enabled;
}

bool InputManager::isInputMethodEnabled() const
{
    return m_inputMethodEnabled;
}

void InputManager::setClickThreshold(int milliseconds)
{
    m_clickThreshold = qMax(0, milliseconds);
}

void InputManager::setDoubleClickInterval(int milliseconds)
{
    m_doubleClickInterval = qMax(0, milliseconds);
}

void InputManager::setDragThreshold(int pixels)
{
    m_dragThreshold = qMax(0, pixels);
}

// ==============================================================================
// 私有方法
// ==============================================================================

void InputManager::setupDefaultShortcuts()
{
    // 光标移动
    registerShortcut(QKeySequence(Qt::Key_Left), EditCommand::MoveCursorLeft);
    registerShortcut(QKeySequence(Qt::Key_Right), EditCommand::MoveCursorRight);
    registerShortcut(QKeySequence(Qt::Key_Up), EditCommand::MoveCursorUp);
    registerShortcut(QKeySequence(Qt::Key_Down), EditCommand::MoveCursorDown);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left), EditCommand::MoveCursorWordLeft);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right), EditCommand::MoveCursorWordRight);

    registerShortcut(QKeySequence(Qt::Key_Home), EditCommand::MoveCursorLineStart);
    registerShortcut(QKeySequence(Qt::Key_End), EditCommand::MoveCursorLineEnd);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Home), EditCommand::MoveCursorDocumentStart);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_End), EditCommand::MoveCursorDocumentEnd);

    registerShortcut(QKeySequence(Qt::Key_PageUp), EditCommand::MoveCursorPageUp);
    registerShortcut(QKeySequence(Qt::Key_PageDown), EditCommand::MoveCursorPageDown);

    // 选择
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Left), EditCommand::SelectLeft);
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Right), EditCommand::SelectRight);
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Up), EditCommand::SelectUp);
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Down), EditCommand::SelectDown);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Left), EditCommand::SelectWordLeft);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Right), EditCommand::SelectWordRight);

    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Home), EditCommand::SelectLineStart);
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_End), EditCommand::SelectLineEnd);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_A), EditCommand::SelectAll);

    // 编辑
    registerShortcut(QKeySequence(Qt::Key_Backspace), EditCommand::DeleteLeft);
    registerShortcut(QKeySequence(Qt::Key_Delete), EditCommand::DeleteRight);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Backspace), EditCommand::DeleteWordLeft);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Delete), EditCommand::DeleteWordRight);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K), EditCommand::DeleteLine);

    // 剪贴板
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_X), EditCommand::Cut);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_C), EditCommand::Copy);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_V), EditCommand::Paste);

    // 撤销/重做
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z), EditCommand::Undo);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y), EditCommand::Redo);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), EditCommand::Redo);

    // 其他
    registerShortcut(QKeySequence(Qt::Key_Return), EditCommand::NewLine);
    registerShortcut(QKeySequence(Qt::Key_Enter), EditCommand::NewLine);
    registerShortcut(QKeySequence(Qt::Key_Tab), EditCommand::Tab);
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Tab), EditCommand::Unindent);

    // 缩进
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight), EditCommand::Indent);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft), EditCommand::Unindent);
}

EditCommand InputManager::keyEventToCommand(QKeyEvent* event) const
{
    if (!event)
        return EditCommand::None;

    int key = event->key();
    Qt::KeyboardModifiers modifiers = event->modifiers();

    // 处理没有修饰键的简单按键
    switch (key) {
    case Qt::Key_Left:
        if (modifiers & Qt::ShiftModifier)
            return EditCommand::SelectLeft;
        else if (modifiers & Qt::ControlModifier)
            return EditCommand::MoveCursorWordLeft;
        else
            return EditCommand::MoveCursorLeft;

    case Qt::Key_Right:
        if (modifiers & Qt::ShiftModifier)
            return EditCommand::SelectRight;
        else if (modifiers & Qt::ControlModifier)
            return EditCommand::MoveCursorWordRight;
        else
            return EditCommand::MoveCursorRight;

    case Qt::Key_Up:
        if (modifiers & Qt::ShiftModifier)
            return EditCommand::SelectUp;
        else
            return EditCommand::MoveCursorUp;

    case Qt::Key_Down:
        if (modifiers & Qt::ShiftModifier)
            return EditCommand::SelectDown;
        else
            return EditCommand::MoveCursorDown;

    case Qt::Key_Home:
        if (modifiers & Qt::ShiftModifier) {
            if (modifiers & Qt::ControlModifier)
                return EditCommand::SelectLineStart; // 这里可以扩展为选择到文档开始
            else
                return EditCommand::SelectLineStart;
        }
        else {
            if (modifiers & Qt::ControlModifier)
                return EditCommand::MoveCursorDocumentStart;
            else
                return EditCommand::MoveCursorLineStart;
        }

    case Qt::Key_End:
        if (modifiers & Qt::ShiftModifier) {
            if (modifiers & Qt::ControlModifier)
                return EditCommand::SelectLineEnd; // 这里可以扩展为选择到文档结束
            else
                return EditCommand::SelectLineEnd;
        }
        else {
            if (modifiers & Qt::ControlModifier)
                return EditCommand::MoveCursorDocumentEnd;
            else
                return EditCommand::MoveCursorLineEnd;
        }

    case Qt::Key_PageUp:
        return EditCommand::MoveCursorPageUp;

    case Qt::Key_PageDown:
        return EditCommand::MoveCursorPageDown;

    case Qt::Key_Backspace:
        if (modifiers & Qt::ControlModifier)
            return EditCommand::DeleteWordLeft;
        else
            return EditCommand::DeleteLeft;

    case Qt::Key_Delete:
        if (modifiers & Qt::ControlModifier)
            return EditCommand::DeleteWordRight;
        else
            return EditCommand::DeleteRight;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        return EditCommand::NewLine;

    case Qt::Key_Tab:
        if (modifiers & Qt::ShiftModifier)
            return EditCommand::Unindent;
        else
            return EditCommand::Tab;

    default:
        break;
    }

    return EditCommand::None;
}

QString InputManager::extractTextFromKeyEvent(QKeyEvent* event) const
{
    if (!event)
        return QString();

    // 对于文本插入命令，返回要插入的文本
    QString text = event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        return text;
    }

    // 对于特殊键，返回空字符串
    return QString();
}

// ==============================================================================
// 扩展功能
// ==============================================================================

// 处理组合键序列
bool InputManager::handleKeySequence(const QList<QKeyEvent*>& events)
{
    // 这里可以实现 Vim 风格的组合键序列
    // 例如: dd (删除行), yy (复制行) 等
    Q_UNUSED(events)
        return false;
}

// 设置 Vim 模式
void InputManager::setVimMode(bool enabled)
{
    if (enabled) {
        setupVimShortcuts();
    }
    else {
        setupDefaultShortcuts();
    }
}

void InputManager::setupVimShortcuts()
{
    // 清除现有快捷键
    m_shortcuts.clear();

    // Vim 风格的快捷键
    // 这里只是示例，完整的 Vim 实现会更复杂
    registerShortcut(QKeySequence(Qt::Key_H), EditCommand::MoveCursorLeft);
    registerShortcut(QKeySequence(Qt::Key_J), EditCommand::MoveCursorDown);
    registerShortcut(QKeySequence(Qt::Key_K), EditCommand::MoveCursorUp);
    registerShortcut(QKeySequence(Qt::Key_L), EditCommand::MoveCursorRight);

    registerShortcut(QKeySequence(Qt::Key_W), EditCommand::MoveCursorWordRight);
    registerShortcut(QKeySequence(Qt::Key_B), EditCommand::MoveCursorWordLeft);

    registerShortcut(QKeySequence(Qt::Key_0), EditCommand::MoveCursorLineStart);
    registerShortcut(QKeySequence(Qt::Key_Dollar), EditCommand::MoveCursorLineEnd);

    registerShortcut(QKeySequence(Qt::Key_X), EditCommand::DeleteRight);
    registerShortcut(QKeySequence(Qt::Key_U), EditCommand::Undo);

    // 更多 Vim 命令...
}

// 设置 Emacs 模式
void InputManager::setEmacsMode(bool enabled)
{
    if (enabled) {
        setupEmacsShortcuts();
    }
    else {
        setupDefaultShortcuts();
    }
}

void InputManager::setupEmacsShortcuts()
{
    // 清除现有快捷键
    m_shortcuts.clear();

    // Emacs 风格的快捷键
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), EditCommand::MoveCursorRight);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), EditCommand::MoveCursorLeft);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), EditCommand::MoveCursorDown);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_P), EditCommand::MoveCursorUp);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_A), EditCommand::MoveCursorLineStart);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), EditCommand::MoveCursorLineEnd);

    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_F), EditCommand::MoveCursorWordRight);
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_B), EditCommand::MoveCursorWordLeft);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), EditCommand::DeleteRight);
    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), EditCommand::DeleteLeft);

    registerShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), EditCommand::DeleteLine);

    // 更多 Emacs 命令...
}

// 宏录制和回放
void InputManager::startMacroRecording(const QString& macroName)
{
    m_currentMacro = macroName;
    m_macroRecording = true;
    m_recordedEvents.clear();
}

void InputManager::stopMacroRecording()
{
    if (m_macroRecording) {
        m_macros[m_currentMacro] = m_recordedEvents;
        m_macroRecording = false;
        m_currentMacro.clear();
        m_recordedEvents.clear();
    }
}

void InputManager::playMacro(const QString& macroName)
{
    if (m_macros.contains(macroName)) {
        const auto& events = m_macros[macroName];
        for (const auto& event : events) {
            // 重放事件
            handleKeyEvent(event);
        }
    }
}

// 上下文相关的快捷键
void InputManager::setContext(const QString& context)
{
    m_currentContext = context;

    // 根据上下文加载不同的快捷键配置
    if (context == "editor") {
        setupDefaultShortcuts();
    }
    else if (context == "find") {
        setupFindShortcuts();
    }
    else if (context == "replace") {
        setupReplaceShortcuts();
    }
}

void InputManager::setupFindShortcuts()
{
    // 搜索模式的快捷键
    registerShortcut(QKeySequence(Qt::Key_Escape), EditCommand::None); // 退出搜索
    registerShortcut(QKeySequence(Qt::Key_Return), EditCommand::None); // 确认搜索
    registerShortcut(QKeySequence(Qt::Key_F3), EditCommand::None);     // 查找下一个
    registerShortcut(QKeySequence(Qt::SHIFT | Qt::Key_F3), EditCommand::None); // 查找上一个
}

void InputManager::setupReplaceShortcuts()
{
    // 替换模式的快捷键
    registerShortcut(QKeySequence(Qt::Key_Escape), EditCommand::None); // 退出替换
    registerShortcut(QKeySequence(Qt::Key_Return), EditCommand::None); // 确认替换
    registerShortcut(QKeySequence(Qt::ALT | Qt::Key_R), EditCommand::None); // 全部替换
}

// 手势支持
void InputManager::enableGestureSupport(bool enabled)
{
    m_gestureEnabled = enabled;
}

bool InputManager::handleGesture(const QString& gesture)
{
    if (!m_gestureEnabled)
        return false;

    // 处理手势命令
    if (gesture == "swipe_left") {
        emit commandTriggered(EditCommand::MoveCursorLeft);
        return true;
    }
    else if (gesture == "swipe_right") {
        emit commandTriggered(EditCommand::MoveCursorRight);
        return true;
    }
    // 更多手势...

    return false;
}

// 智能提示和自动完成
void InputManager::setAutoCompletionEnabled(bool enabled)
{
    m_autoCompletionEnabled = enabled;
}

bool InputManager::shouldTriggerAutoCompletion(QKeyEvent* event) const
{
    if (!m_autoCompletionEnabled || !event)
        return false;

    // 触发自动完成的条件
    QString text = event->text();
    if (text.isEmpty())
        return false;

    QChar ch = text.at(0);

    // 字母、数字或下划线触发自动完成
    if (ch.isLetterOrNumber() || ch == '_' || ch == '.') {
        return true;
    }

    return false;
}

// 调试和日志
QString InputManager::getShortcutInfo() const
{
    QStringList info;

    info << "=== 标准快捷键 ===";
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        info << QString("%1 -> %2").arg(it.key().toString()).arg(static_cast<int>(it.value()));
    }

    info << "\n=== 自定义快捷键 ===";
    for (auto it = m_customShortcuts.begin(); it != m_customShortcuts.end(); ++it) {
        info << QString("%1 -> %2").arg(it.key().toString()).arg(it.value());
    }

    return info.join("\n");
}

void InputManager::logKeyEvent(QKeyEvent* event) const
{
    if (!event)
        return;

    QString modifiers;
    if (event->modifiers() & Qt::ControlModifier) modifiers += "Ctrl+";
    if (event->modifiers() & Qt::AltModifier) modifiers += "Alt+";
    if (event->modifiers() & Qt::ShiftModifier) modifiers += "Shift+";
    if (event->modifiers() & Qt::MetaModifier) modifiers += "Meta+";

    QString keyName = QKeySequence(event->key()).toString();

    qDebug() << QString("KeyEvent: %1%2, Text: '%3'")
        .arg(modifiers)
        .arg(keyName)
        .arg(event->text());
}