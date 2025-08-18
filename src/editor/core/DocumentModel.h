#ifndef DOCUMENT_MODEL_H
#define DOCUMENT_MODEL_H

#include "TextStorage.h"
#include "UndoSystem.h"
#include <QObject>
#include <QUrl>
#include <QDateTime>
#include <QStringConverter>
#include <qqmlintegration.h>
#include <memory>

//class DocumentModel;

// 声明不透明指针类型
//Q_DECLARE_OPAQUE_POINTER(DocumentModel*)

// 文档变更事件
struct TextChange {
    Q_GADGET
public:
    Q_PROPERTY(int position MEMBER position)
    Q_PROPERTY(int removedLength MEMBER removedLength)
    Q_PROPERTY(QString insertedText MEMBER insertedText)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)

    int position;
    int removedLength;
    QString insertedText;
    QDateTime timestamp;
};

// 文档模型
class DocumentModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(bool isModified READ isModified WRITE setModified NOTIFY modifiedChanged)
    Q_PROPERTY(bool isReadOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY textChanged)
    Q_PROPERTY(int textLength READ textLength NOTIFY textChanged)
    Q_PROPERTY(QString fullText READ getFullText NOTIFY textChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoAvailable)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY redoAvailable)

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
    Q_INVOKABLE void insertText(int position, const QString& text);
    Q_INVOKABLE void removeText(int position, int length);
    Q_INVOKABLE void replaceText(int position, int length, const QString& text);
    Q_INVOKABLE QString getText(int position, int length) const;
    Q_INVOKABLE QString getFullText() const;
    Q_INVOKABLE int textLength() const;

    // 行操作
    Q_INVOKABLE int lineCount() const;
    Q_INVOKABLE QString getLine(int lineNumber) const;
    Q_INVOKABLE int positionToLine(int position) const;
    Q_INVOKABLE int positionToColumn(int position) const;
    Q_INVOKABLE int lineColumnToPosition(int line, int column) const;

    // 撤销/重做
    bool canUndo() const;
    bool canRedo() const;
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void clearUndoHistory();

    // 文件操作
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    Q_INVOKABLE bool saveToFile(const QString& filePath = QString());

    // 搜索
    Q_INVOKABLE QList<int> findText(const QString& pattern, bool caseSensitive = false, bool wholeWords = false) const;

    // 撤销系统专用方法 - 直接操作存储，不触发撤销命令
    void insertTextDirect(int position, const QString& text);
    void removeTextDirect(int position, int length);
    void replaceTextDirect(int position, int length, const QString& text);
    QString getTextDirect(int position, int length) const;

    // 批量操作支持
    Q_INVOKABLE void beginBatchEdit();
    Q_INVOKABLE void endBatchEdit();

    // 统计信息
    Q_INVOKABLE int getCharacterCount() const;
    Q_INVOKABLE int getWordCount() const;
    Q_INVOKABLE int getParagraphCount() const;

    // 文档状态
    Q_INVOKABLE bool isEmpty() const;
    Q_INVOKABLE bool isLargeFile() const;
    Q_INVOKABLE QDateTime lastModified() const;

    // 快照功能
    Q_INVOKABLE QString createSnapshot() const;
    Q_INVOKABLE bool restoreFromSnapshot(const QString& snapshot);

signals:
    void textChanged(const TextChange& change);
    void modifiedChanged(bool modified);
    void readOnlyChanged(bool readOnly);
    void encodingChanged(Encoding encoding);
    void filePathChanged(const QString& filePath);
    void undoAvailable(bool available);
    void redoAvailable(bool available);
};

#endif // DOCUMENT_MODEL_H