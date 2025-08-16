#ifndef TEXT_STORAGE_H
#define TEXT_STORAGE_H

#include <QString>
#include <QByteArray>
#include <QList>
#include <memory>

// 文本存储接口
class ITextStorage {
public:
    virtual ~ITextStorage() = default;

    // 基础操作
    virtual void insert(int position, const QString& text) = 0;
    virtual void remove(int position, int length) = 0;
    virtual void replace(int position, int length, const QString& text) = 0;
    virtual QString getText(int position, int length) const = 0;
    virtual QString getFullText() const = 0;
    virtual int length() const = 0;

    // 行操作
    virtual int getLineCount() const = 0;
    virtual int getLineStart(int lineNumber) const = 0;
    virtual int getLineEnd(int lineNumber) const = 0;
    virtual int getLineLength(int lineNumber) const = 0;
    virtual QString getLine(int lineNumber) const = 0;
    virtual int positionToLine(int position) const = 0;
    virtual int positionToColumn(int position) const = 0;
    virtual int lineColumnToPosition(int line, int column) const = 0;
};

// Piece Table 实现
class PieceTable : public ITextStorage {
public:
    struct Piece {
        enum Source { Original, Added };
        Source source;
        int start;
        int length;

        Piece(Source src, int st, int len) : source(src), start(st), length(len) {}
    };

private:
    QString m_originalText;
    QString m_addedText;
    QList<Piece> m_pieces;
    mutable QList<int> m_lineStarts; // 缓存行起始位置
    mutable bool m_lineIndexDirty = true;

    void updateLineIndex() const;
    int findPieceIndex(int position) const;
    void splitPiece(int pieceIndex, int offset);

public:
    PieceTable();
    explicit PieceTable(const QString& initialText);

    // ITextStorage 接口实现
    void insert(int position, const QString& text) override;
    void remove(int position, int length) override;
    void replace(int position, int length, const QString& text) override;
    QString getText(int position, int length) const override;
    QString getFullText() const override;
    int length() const override;

    int getLineCount() const override;
    int getLineStart(int lineNumber) const override;
    int getLineEnd(int lineNumber) const override;
    int getLineLength(int lineNumber) const override;
    QString getLine(int lineNumber) const override;
    int positionToLine(int position) const override;
    int positionToColumn(int position) const override;
    int lineColumnToPosition(int line, int column) const override;
};

// 大文件分页存储
class ChunkedTextStorage : public ITextStorage {
private:
    struct Chunk {
        QString data;
        bool loaded = false;
        QString filePath; // 用于懒加载
    };

    static constexpr int CHUNK_SIZE = 64 * 1024; // 64KB per chunk
    QList<Chunk> m_chunks;
    QString m_filePath;

    void loadChunk(int chunkIndex) const;
    void unloadChunk(int chunkIndex) const;
    void saveChunkToFile(const Chunk& chunk, int chunkIndex) const;
    void splitChunk(int chunkIndex);

public:
    explicit ChunkedTextStorage(const QString& filePath);

    // ITextStorage 接口实现
    void insert(int position, const QString& text) override;
    void remove(int position, int length) override;
    void replace(int position, int length, const QString& text) override;
    QString getText(int position, int length) const override;
    QString getFullText() const override;
    int length() const override;

    int getLineCount() const override;
    int getLineStart(int lineNumber) const override;
    int getLineEnd(int lineNumber) const override;
    int getLineLength(int lineNumber) const override;
    QString getLine(int lineNumber) const override;
    int positionToLine(int position) const override;
    int positionToColumn(int position) const override;
    int lineColumnToPosition(int line, int column) const override;
};

#endif // TEXT_STORAGE_H