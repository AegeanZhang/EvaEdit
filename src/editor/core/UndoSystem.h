#ifndef UNDO_SYSTEM_H
#define UNDO_SYSTEM_H

#include <QObject>
#include <QDateTime>
#include <QStringList>
#include <memory>
#include <vector>

// 前向声明
class DocumentModel;

// 编辑命令接口
class IEditCommand {
public:
    virtual ~IEditCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual bool canMerge(const IEditCommand* other) const = 0;
    virtual void merge(const IEditCommand* other) = 0;
    virtual QString description() const = 0;
};

// 文本插入命令
class InsertTextCommand : public IEditCommand {
private:
    DocumentModel* m_document;
    int m_position;
    QString m_text;
    QDateTime m_timestamp;

public:
    InsertTextCommand(DocumentModel* doc, int pos, const QString& text);

    void execute() override;
    void undo() override;
    bool canMerge(const IEditCommand* other) const override;
    void merge(const IEditCommand* other) override;
    QString description() const override;
};

// 文本删除命令
class RemoveTextCommand : public IEditCommand {
private:
    DocumentModel* m_document;
    int m_position;
    int m_length;
    QString m_removedText;

public:
    RemoveTextCommand(DocumentModel* doc, int pos, int length);

    void execute() override;
    void undo() override;
    bool canMerge(const IEditCommand* other) const override;
    void merge(const IEditCommand* other) override;
    QString description() const override;
};

// 撤销系统
class UndoSystem : public QObject {
    Q_OBJECT

private:
    std::vector<std::unique_ptr<IEditCommand>> m_undoStack;
    std::vector<std::unique_ptr<IEditCommand>> m_redoStack;
    int m_maxUndoSteps;
    bool m_mergeEnabled;
    QDateTime m_lastCommandTime;
    static constexpr int MERGE_TIME_LIMIT_MS = 1000;

public:
    explicit UndoSystem(QObject* parent = nullptr);

    // 核心功能
    void executeCommand(std::unique_ptr<IEditCommand> command);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clear();

    // 配置管理
    void setMaxUndoSteps(int maxSteps);
    int maxUndoSteps() const;

    void setMergeEnabled(bool enabled);
    bool isMergeEnabled() const;

    // 批量编辑支持
    void beginBatchEdit(const QString& description = QString());
    void endBatchEdit();

    // 历史记录查询
    QStringList getUndoHistory() const;
    QStringList getRedoHistory() const;

    // 命令创建辅助函数
    std::unique_ptr<IEditCommand> createInsertCommand(DocumentModel* doc, int pos, const QString& text);
    std::unique_ptr<IEditCommand> createRemoveCommand(DocumentModel* doc, int pos, int length);
    std::unique_ptr<IEditCommand> createReplaceCommand(DocumentModel* doc, int pos, const QString& oldText, const QString& newText);

signals:
    void undoAvailable(bool available);
    void redoAvailable(bool available);
    void stackChanged();
};

#endif // UNDO_SYSTEM_H