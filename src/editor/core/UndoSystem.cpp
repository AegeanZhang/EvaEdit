#include "UndoSystem.h"
#include "DocumentModel.h"
#include <QDebug>

// ==============================================================================
// InsertTextCommand 实现
// ==============================================================================

InsertTextCommand::InsertTextCommand(DocumentModel* doc, int pos, const QString& text)
    : m_document(doc)
    , m_position(pos)
    , m_text(text)
    , m_timestamp(QDateTime::currentDateTime())
{
    Q_ASSERT(m_document != nullptr);
    Q_ASSERT(m_position >= 0);
    Q_ASSERT(!m_text.isEmpty());
}

void InsertTextCommand::execute()
{
    if (!m_document || m_document->isReadOnly())
        return;

    // 边界检查
    m_position = qBound(0, m_position, m_document->textLength());

    // 执行插入操作（直接调用存储层，避免递归）
    m_document->insertTextDirect(m_position, m_text);

    // 手动触发变更信号，避免通过公共接口产生新的撤销命令
    TextChange change;
    change.position = m_position;
    change.removedLength = 0;
    change.insertedText = m_text;
    change.timestamp = m_timestamp;

    m_document->setModified(true);
    emit m_document->textChanged(change);
}

void InsertTextCommand::undo()
{
    if (!m_document || m_document->isReadOnly())
        return;

    // 撤销插入 = 删除插入的文本
    int currentLength = m_document->textLength();
    int removePosition = qBound(0, m_position, currentLength);
    int removeLength = qMin(m_text.length(), currentLength - removePosition);

    if (removeLength > 0) {
        m_document->removeTextDirect(removePosition, removeLength);

        // 手动触发变更信号
        TextChange change;
        change.position = removePosition;
        change.removedLength = removeLength;
        change.insertedText = "";
        change.timestamp = QDateTime::currentDateTime();

        emit m_document->textChanged(change);
    }
}

bool InsertTextCommand::canMerge(const IEditCommand* other) const
{
    const InsertTextCommand* otherInsert = dynamic_cast<const InsertTextCommand*>(other);
    if (!otherInsert)
        return false;

    // 检查是否可以合并：
    // 1. 同一个文档
    // 2. 连续的位置插入
    // 3. 时间间隔在限制内
    // 4. 都是简单字符（非换行符等特殊字符）

    if (otherInsert->m_document != m_document)
        return false;

    // 检查时间间隔
    qint64 timeDiff = m_timestamp.msecsTo(otherInsert->m_timestamp);
    if (timeDiff > 1000 || timeDiff < 0) // 1秒内的操作才能合并
        return false;

    // 检查位置连续性
    if (otherInsert->m_position != m_position + m_text.length())
        return false;

    // 检查是否为简单字符插入（避免合并复杂操作）
    if (m_text.contains('\n') || otherInsert->m_text.contains('\n'))
        return false;

    // 限制合并后的文本长度，避免过长的撤销单元
    if (m_text.length() + otherInsert->m_text.length() > 100)
        return false;

    return true;
}

void InsertTextCommand::merge(const IEditCommand* other)
{
    const InsertTextCommand* otherInsert = dynamic_cast<const InsertTextCommand*>(other);
    if (!otherInsert || !canMerge(other))
        return;

    // 合并文本
    m_text += otherInsert->m_text;

    // 更新时间戳为最新的
    m_timestamp = otherInsert->m_timestamp;
}

QString InsertTextCommand::description() const
{
    if (m_text.length() == 1) {
        QChar ch = m_text.at(0);
        if (ch == '\n')
            return QString("插入换行符");
        else if (ch == '\t')
            return QString("插入制表符");
        else if (ch.isPrint())
            return QString("插入字符 '%1'").arg(ch);
        else
            return QString("插入字符");
    }
    else if (m_text.length() <= 20) {
        return QString("插入文本 '%1'").arg(m_text);
    }
    else {
        return QString("插入文本 (%1 字符)").arg(m_text.length());
    }
}

// ==============================================================================
// RemoveTextCommand 实现
// ==============================================================================

RemoveTextCommand::RemoveTextCommand(DocumentModel* doc, int pos, int length)
    : m_document(doc)
    , m_position(pos)
    , m_length(length)
{
    Q_ASSERT(m_document != nullptr);
    Q_ASSERT(m_position >= 0);
    Q_ASSERT(m_length > 0);

    // 在创建命令时保存要删除的文本，用于撤销
    if (m_document) {
        int docLength = m_document->textLength();
        m_position = qBound(0, m_position, docLength);
        m_length = qMin(m_length, docLength - m_position);

        if (m_length > 0) {
            m_removedText = m_document->getTextDirect(m_position, m_length);
        }
    }
}

void RemoveTextCommand::execute()
{
    if (!m_document || m_document->isReadOnly() || m_length <= 0)
        return;

    // 执行删除操作
    int currentLength = m_document->textLength();
    int removePosition = qBound(0, m_position, currentLength);
    int removeLength = qMin(m_length, currentLength - removePosition);

    if (removeLength > 0) {
        m_document->removeTextDirect(removePosition, removeLength);

        // 手动触发变更信号
        TextChange change;
        change.position = removePosition;
        change.removedLength = removeLength;
        change.insertedText = "";
        change.timestamp = QDateTime::currentDateTime();

        m_document->setModified(true);
        emit m_document->textChanged(change);
    }
}

void RemoveTextCommand::undo()
{
    if (!m_document || m_document->isReadOnly() || m_removedText.isEmpty())
        return;

    // 撤销删除 = 重新插入删除的文本
    int currentLength = m_document->textLength();
    int insertPosition = qBound(0, m_position, currentLength);

    m_document->insertTextDirect(insertPosition, m_removedText);

    // 手动触发变更信号
    TextChange change;
    change.position = insertPosition;
    change.removedLength = 0;
    change.insertedText = m_removedText;
    change.timestamp = QDateTime::currentDateTime();

    emit m_document->textChanged(change);
}

bool RemoveTextCommand::canMerge(const IEditCommand* other) const
{
    const RemoveTextCommand* otherRemove = dynamic_cast<const RemoveTextCommand*>(other);
    if (!otherRemove)
        return false;

    // 删除命令的合并比较复杂，这里采用保守策略
    // 只合并连续的单字符删除（退格键或删除键）

    if (otherRemove->m_document != m_document)
        return false;

    // 只合并单字符删除
    if (m_length != 1 || otherRemove->m_length != 1)
        return false;

    // 检查是否为连续删除（退格键模式：位置递减，删除键模式：位置相同）
    bool isBackspace = (otherRemove->m_position == m_position - 1); // 退格键
    bool isDelete = (otherRemove->m_position == m_position);         // 删除键

    if (!isBackspace && !isDelete)
        return false;

    // 避免合并换行符
    if (m_removedText.contains('\n') || otherRemove->m_removedText.contains('\n'))
        return false;

    // 限制合并长度
    if (m_removedText.length() + otherRemove->m_removedText.length() > 50)
        return false;

    return true;
}

void RemoveTextCommand::merge(const IEditCommand* other)
{
    const RemoveTextCommand* otherRemove = dynamic_cast<const RemoveTextCommand*>(other);
    if (!otherRemove || !canMerge(other))
        return;

    if (otherRemove->m_position == m_position - 1) {
        // 退格键模式：新删除的字符在前面
        m_removedText = otherRemove->m_removedText + m_removedText;
        m_position = otherRemove->m_position;
        m_length += otherRemove->m_length;
    }
    else if (otherRemove->m_position == m_position) {
        // 删除键模式：新删除的字符在后面
        m_removedText = m_removedText + otherRemove->m_removedText;
        m_length += otherRemove->m_length;
    }
}

QString RemoveTextCommand::description() const
{
    if (m_removedText.length() == 1) {
        QChar ch = m_removedText.at(0);
        if (ch == '\n')
            return QString("删除换行符");
        else if (ch == '\t')
            return QString("删除制表符");
        else if (ch.isPrint())
            return QString("删除字符 '%1'").arg(ch);
        else
            return QString("删除字符");
    }
    else if (m_removedText.length() <= 20) {
        return QString("删除文本 '%1'").arg(m_removedText);
    }
    else {
        return QString("删除文本 (%1 字符)").arg(m_removedText.length());
    }
}

// ==============================================================================
// ReplaceTextCommand 实现
// ==============================================================================

class ReplaceTextCommand : public IEditCommand {
private:
    DocumentModel* m_document;
    int m_position;
    QString m_oldText;
    QString m_newText;
    QDateTime m_timestamp;

public:
    ReplaceTextCommand(DocumentModel* doc, int pos, const QString& oldText, const QString& newText)
        : m_document(doc)
        , m_position(pos)
        , m_oldText(oldText)
        , m_newText(newText)
        , m_timestamp(QDateTime::currentDateTime())
    {
        Q_ASSERT(m_document != nullptr);
        Q_ASSERT(m_position >= 0);
    }

    void execute() override
    {
        if (!m_document || m_document->isReadOnly())
            return;

        m_document->replaceTextDirect(m_position, m_oldText.length(), m_newText);

        TextChange change;
        change.position = m_position;
        change.removedLength = m_oldText.length();
        change.insertedText = m_newText;
        change.timestamp = m_timestamp;

        m_document->setModified(true);
        emit m_document->textChanged(change);
    }

    void undo() override
    {
        if (!m_document || m_document->isReadOnly())
            return;

        m_document->replaceTextDirect(m_position, m_newText.length(), m_oldText);

        TextChange change;
        change.position = m_position;
        change.removedLength = m_newText.length();
        change.insertedText = m_oldText;
        change.timestamp = QDateTime::currentDateTime();

        emit m_document->textChanged(change);
    }

    bool canMerge(const IEditCommand* other) const override
    {
        // 替换命令一般不合并，保持操作的原子性
        Q_UNUSED(other)
            return false;
    }

    void merge(const IEditCommand* other) override
    {
        Q_UNUSED(other)
            // 不支持合并
    }

    QString description() const override
    {
        return QString("替换文本 '%1' -> '%2'").arg(m_oldText.left(20)).arg(m_newText.left(20));
    }
};

// ==============================================================================
// UndoSystem 实现
// ==============================================================================

UndoSystem::UndoSystem(QObject* parent)
    : QObject(parent)
    , m_maxUndoSteps(1000)
    , m_mergeEnabled(true)
    , m_lastCommandTime(QDateTime::currentDateTime())
{
}

void UndoSystem::executeCommand(std::unique_ptr<IEditCommand> command)
{
    if (!command)
        return;

    QDateTime currentTime = QDateTime::currentDateTime();

    // 尝试与上一个命令合并
    if (m_mergeEnabled && !m_undoStack.empty()) {
        IEditCommand* lastCommand = m_undoStack.back().get();

        if (lastCommand->canMerge(command.get()) &&
            m_lastCommandTime.msecsTo(currentTime) <= MERGE_TIME_LIMIT_MS) {

            // 合并命令
            lastCommand->merge(command.get());
            m_lastCommandTime = currentTime;

            // 执行合并后的命令（只执行新增部分）
            command->execute();

            emit stackChanged();
            return;
        }
    }

    // 执行新命令
    command->execute();

    // 将命令推入撤销栈
    m_undoStack.push_back(std::move(command));
    m_lastCommandTime = currentTime;

    // 清空重做栈（执行新命令后，之前的重做历史无效）
    if (!m_redoStack.empty()) {
        m_redoStack.clear();
        emit redoAvailable(false);
    }

    // 限制撤销栈大小
    while (m_undoStack.size() > static_cast<size_t>(m_maxUndoSteps)) {
        m_undoStack.erase(m_undoStack.begin());
    }

    // 发送信号
    emit undoAvailable(true);
    emit stackChanged();
}

bool UndoSystem::canUndo() const
{
    return !m_undoStack.empty();
}

bool UndoSystem::canRedo() const
{
    return !m_redoStack.empty();
}

void UndoSystem::undo()
{
    if (m_undoStack.empty())
        return;

    // 从撤销栈弹出命令并执行撤销
    std::unique_ptr<IEditCommand> command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    command->undo();

    // 将命令移入重做栈
    m_redoStack.push_back(std::move(command));

    // 发送信号
    emit undoAvailable(!m_undoStack.empty());
    emit redoAvailable(true);
    emit stackChanged();
}

void UndoSystem::redo()
{
    if (m_redoStack.empty())
        return;

    // 从重做栈弹出命令并重新执行
    std::unique_ptr<IEditCommand> command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    command->execute();

    // 将命令移回撤销栈
    m_undoStack.push_back(std::move(command));

    // 发送信号
    emit undoAvailable(true);
    emit redoAvailable(!m_redoStack.empty());
    emit stackChanged();
}

void UndoSystem::clear()
{
    bool hadUndo = !m_undoStack.empty();
    bool hadRedo = !m_redoStack.empty();

    m_undoStack.clear();
    m_redoStack.clear();

    if (hadUndo) {
        emit undoAvailable(false);
    }
    if (hadRedo) {
        emit redoAvailable(false);
    }

    emit stackChanged();
}

void UndoSystem::setMaxUndoSteps(int maxSteps)
{
    m_maxUndoSteps = qMax(1, maxSteps);

    // 如果当前栈大小超过新限制，裁剪旧的命令
    while (m_undoStack.size() > static_cast<size_t>(m_maxUndoSteps)) {
        m_undoStack.erase(m_undoStack.begin());
    }

    if (m_undoStack.empty()) {
        emit undoAvailable(false);
    }
}

// 获取撤销/重做历史描述
QStringList UndoSystem::getUndoHistory() const
{
    QStringList history;
    for (auto it = m_undoStack.rbegin(); it != m_undoStack.rend(); ++it) {
        history.append((*it)->description()); // 最新的在前面
    }
    return history;
}

QStringList UndoSystem::getRedoHistory() const
{
    QStringList history;
    for (auto it = m_redoStack.rbegin(); it != m_redoStack.rend(); ++it) {
        history.append((*it)->description()); // 按执行顺序
    }
    return history;
}

int UndoSystem::maxUndoSteps() const
{
    return m_maxUndoSteps;
}

void UndoSystem::setMergeEnabled(bool enabled)
{
    m_mergeEnabled = enabled;
}

bool UndoSystem::isMergeEnabled() const
{
    return m_mergeEnabled;
}

// ==============================================================================
// 额外的编辑命令实现
// ==============================================================================

// 批量编辑命令 - 将多个命令组合为一个撤销单元
class BatchEditCommand : public IEditCommand {
private:
    std::vector<std::unique_ptr<IEditCommand>> m_commands;
    QString m_description;

public:
    explicit BatchEditCommand(const QString& description = "批量编辑")
        : m_description(description)
    {
    }

    void addCommand(std::unique_ptr<IEditCommand> command)
    {
        if (command) {
            m_commands.push_back(std::move(command));
        }
    }

    void execute() override
    {
        for (auto& command : m_commands) {
            command->execute();
        }
    }

    void undo() override
    {
        // 逆序撤销
        for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it) {
            (*it)->undo();
        }
    }

    bool canMerge(const IEditCommand* other) const override
    {
        Q_UNUSED(other)
            return false; // 批量命令不合并
    }

    void merge(const IEditCommand* other) override
    {
        Q_UNUSED(other)
            // 不支持合并
    }

    QString description() const override
    {
        return m_description;
    }

    bool isEmpty() const
    {
        return m_commands.empty();
    }

    size_t commandCount() const
    {
        return m_commands.size();
    }
};

// 为 UndoSystem 添加批量编辑支持
void UndoSystem::beginBatchEdit(const QString& description)
{
    // 可以通过成员变量跟踪批量编辑状态
    // 这里简化实现，使用信号通知外部组件
    Q_UNUSED(description)
        // emit batchEditStarted(description);
}

void UndoSystem::endBatchEdit()
{
    // emit batchEditEnded();
}

// 创建辅助函数，简化命令创建
std::unique_ptr<IEditCommand> UndoSystem::createInsertCommand(DocumentModel* doc, int pos, const QString& text)
{
    return std::make_unique<InsertTextCommand>(doc, pos, text);
}

std::unique_ptr<IEditCommand> UndoSystem::createRemoveCommand(DocumentModel* doc, int pos, int length)
{
    return std::make_unique<RemoveTextCommand>(doc, pos, length);
}

std::unique_ptr<IEditCommand> UndoSystem::createReplaceCommand(DocumentModel* doc, int pos, const QString& oldText, const QString& newText)
{
    return std::make_unique<ReplaceTextCommand>(doc, pos, oldText, newText);
}