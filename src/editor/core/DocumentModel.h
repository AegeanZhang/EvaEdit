#ifndef DOCUMENT_MODEL_H
#define DOCUMENT_MODEL_H

#include "TextStorage.h"
#include "UndoSystem.h"
#include <QObject>
#include <QUrl>
#include <QDateTime>
#include <QStringConverter>
#include <memory>

// 文档变更事件
struct TextChange {
    int position;
    int removedLength;
    QString insertedText;
    QDateTime timestamp;
};

// 文档模型
class DocumentModel : public QObject {
    Q_OBJECT

public:
    enum DocumentType {
        PlainText,
        Markdown,
        Code
    };

    enum Encoding {
        UTF8,
        UTF16,
        GBK,
        ASCII
    };

private:
    std::unique_ptr<ITextStorage> m_textStorage;
    std::unique_ptr<UndoSystem> m_undoSystem;
    QString m_filePath;
    DocumentType m_type;
    Encoding m_encoding;
    bool m_modified;
    bool m_readOnly;
    QDateTime m_lastModified;
    QList<TextChange> m_changeHistory;

    // Qt6 编码相关的私有方法
    Encoding detectFileEncoding(const QByteArray& data) const;
    QString convertFromEncoding(const QByteArray& data, Encoding encoding) const;
    QByteArray convertToEncoding(const QString& text, Encoding encoding) const;
    QStringConverter::Encoding encodingToQt(Encoding encoding) const;

    // 文件监控相关
    bool isFileModifiedExternally() const;
    void refreshFromFile();

public:
    explicit DocumentModel(QObject* parent = nullptr);
    ~DocumentModel();

    // 文档属性
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);

    DocumentType type() const { return m_type; }
    void setType(DocumentType type);

    Encoding encoding() const { return m_encoding; }
    void setEncoding(Encoding encoding);

    bool isModified() const { return m_modified; }
    void setModified(bool modified);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

    // 文本操作
    void insertText(int position, const QString& text);
    void removeText(int position, int length);
    void replaceText(int position, int length, const QString& text);
    QString getText(int position, int length) const;
    QString getFullText() const;
    int textLength() const;

    // 行操作
    int lineCount() const;
    QString getLine(int lineNumber) const;
    int positionToLine(int position) const;
    int positionToColumn(int position) const;
    int lineColumnToPosition(int line, int column) const;

    // 撤销/重做
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void clearUndoHistory();

    // 文件操作
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath = QString());

    // 搜索
    QList<int> findText(const QString& pattern, bool caseSensitive = false, bool wholeWords = false) const;

    // 撤销系统专用方法 - 直接操作存储，不触发撤销命令
    void insertTextDirect(int position, const QString& text);
    void removeTextDirect(int position, int length);
    void replaceTextDirect(int position, int length, const QString& text);
    QString getTextDirect(int position, int length) const;

    // 批量操作支持
    void beginBatchEdit();
    void endBatchEdit();

    // 统计信息
    int getCharacterCount() const;
    int getWordCount() const;
    int getParagraphCount() const;

    // 文档状态
    bool isEmpty() const;
    bool isLargeFile() const;
    QDateTime lastModified() const;

    // 快照功能
    QString createSnapshot() const;
    bool restoreFromSnapshot(const QString& snapshot);

signals:
    void textChanged(const TextChange& change);
    void modifiedChanged(bool modified);
    void encodingChanged(Encoding encoding);
    void filePathChanged(const QString& filePath);
    void undoAvailable(bool available);
    void redoAvailable(bool available);
};

#endif // DOCUMENT_MODEL_H