#include "TextStorage.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

// ==============================================================================
// PieceTable 实现
// ==============================================================================

PieceTable::PieceTable()
{
    // 初始化空的piece table
    m_lineStarts.append(0);
}

PieceTable::PieceTable(const QString& initialText)
    : m_originalText(initialText)
{
    if (!initialText.isEmpty()) {
        m_pieces.append(Piece(Piece::Original, 0, initialText.length()));
    }
    m_lineIndexDirty = true;
}

void PieceTable::insert(int position, const QString& text)
{
    if (text.isEmpty())
        return;

    // 边界检查
    position = qBound(0, position, length());

    // 添加新文本到added buffer
    int addedStart = m_addedText.length();
    m_addedText.append(text);

    if (m_pieces.isEmpty()) {
        // 如果没有pieces，直接添加新piece
        m_pieces.append(Piece(Piece::Added, addedStart, text.length()));
    }
    else {
        // 找到插入位置对应的piece
        int currentPos = 0;
        int pieceIndex = -1;
        int offsetInPiece = 0;

        for (int i = 0; i < m_pieces.size(); ++i) {
            if (currentPos + m_pieces[i].length >= position) {
                pieceIndex = i;
                offsetInPiece = position - currentPos;
                break;
            }
            currentPos += m_pieces[i].length;
        }

        if (pieceIndex == -1) {
            // 在末尾插入
            m_pieces.append(Piece(Piece::Added, addedStart, text.length()));
        }
        else if (offsetInPiece == 0) {
            // 在piece开始处插入
            m_pieces.insert(pieceIndex, Piece(Piece::Added, addedStart, text.length()));
        }
        else if (offsetInPiece == m_pieces[pieceIndex].length) {
            // 在piece结束处插入
            m_pieces.insert(pieceIndex + 1, Piece(Piece::Added, addedStart, text.length()));
        }
        else {
            // 在piece中间插入，需要分割
            splitPiece(pieceIndex, offsetInPiece);
            m_pieces.insert(pieceIndex + 1, Piece(Piece::Added, addedStart, text.length()));
        }
    }

    m_lineIndexDirty = true;
}

void PieceTable::remove(int position, int length)
{
    if (length <= 0)
        return;

    // 边界检查
    position = qBound(0, position, this->length());
    length = qMin(length, this->length() - position);

    if (length == 0)
        return;

    int endPosition = position + length;
    int currentPos = 0;

    // 找到需要删除的pieces范围
    QList<int> piecesToRemove;
    QList<QPair<int, int>> piecesToModify; // piece index, new length

    for (int i = 0; i < m_pieces.size(); ++i) {
        int pieceStart = currentPos;
        int pieceEnd = currentPos + m_pieces[i].length;

        if (pieceEnd <= position) {
            // piece完全在删除范围之前
            currentPos = pieceEnd;
            continue;
        }

        if (pieceStart >= endPosition) {
            // piece完全在删除范围之后
            break;
        }

        // piece与删除范围有交集
        if (pieceStart >= position && pieceEnd <= endPosition) {
            // piece完全在删除范围内
            piecesToRemove.append(i);
        }
        else if (pieceStart < position && pieceEnd > endPosition) {
            // 删除范围完全在piece内，需要分割
            int offsetStart = position - pieceStart;
            int offsetEnd = endPosition - pieceStart;

            splitPiece(i, offsetStart);
            splitPiece(i + 1, offsetEnd - offsetStart);
            piecesToRemove.append(i + 1);
            i += 2; // 跳过新创建的pieces
            currentPos = pieceEnd;
        }
        else if (pieceStart < position) {
            // piece开始在删除范围之前，结束在删除范围内
            int newLength = position - pieceStart;
            piecesToModify.append(qMakePair(i, newLength));
        }
        else {
            // piece开始在删除范围内，结束在删除范围之后
            int removeLength = endPosition - pieceStart;
            Piece& piece = m_pieces[i];
            piece.start += removeLength;
            piece.length -= removeLength;
        }

        currentPos = pieceEnd;
    }

    // 执行删除操作（从后往前删除，避免索引变化）
    for (int i = piecesToRemove.size() - 1; i >= 0; --i) {
        m_pieces.removeAt(piecesToRemove[i]);
    }

    // 执行修改操作
    for (const auto& modify : piecesToModify) {
        m_pieces[modify.first].length = modify.second;
    }

    m_lineIndexDirty = true;
}

void PieceTable::replace(int position, int length, const QString& text)
{
    remove(position, length);
    insert(position, text);
}

QString PieceTable::getText(int position, int length) const
{
    if (length <= 0)
        return QString();

    // 边界检查
    position = qBound(0, position, this->length());
    length = qMin(length, this->length() - position);

    if (length == 0)
        return QString();

    QString result;
    result.reserve(length);

    int currentPos = 0;
    int remainingLength = length;

    for (const Piece& piece : m_pieces) {
        if (remainingLength <= 0)
            break;

        int pieceEnd = currentPos + piece.length;

        if (pieceEnd <= position) {
            currentPos = pieceEnd;
            continue;
        }

        if (currentPos >= position + length)
            break;

        // 计算在当前piece中需要提取的范围
        int extractStart = qMax(position - currentPos, 0);
        int extractEnd = qMin(position + length - currentPos, piece.length);
        int extractLength = extractEnd - extractStart;

        if (extractLength > 0) {
            const QString& sourceText = (piece.source == Piece::Original) ?
                m_originalText : m_addedText;

            result.append(sourceText.mid(piece.start + extractStart, extractLength));
            remainingLength -= extractLength;
        }

        currentPos = pieceEnd;
    }

    return result;
}

QString PieceTable::getFullText() const
{
    return getText(0, length());
}

int PieceTable::length() const
{
    int totalLength = 0;
    for (const Piece& piece : m_pieces) {
        totalLength += piece.length;
    }
    return totalLength;
}

void PieceTable::splitPiece(int pieceIndex, int offset)
{
    if (pieceIndex < 0 || pieceIndex >= m_pieces.size() || offset <= 0)
        return;

    const Piece& originalPiece = m_pieces[pieceIndex];

    if (offset >= originalPiece.length)
        return;

    // 创建两个新piece
    Piece firstPart(originalPiece.source, originalPiece.start, offset);
    Piece secondPart(originalPiece.source, originalPiece.start + offset,
        originalPiece.length - offset);

    // 替换原piece
    m_pieces[pieceIndex] = firstPart;
    m_pieces.insert(pieceIndex + 1, secondPart);
}

int PieceTable::findPieceIndex(int position) const
{
    int currentPos = 0;
    for (int i = 0; i < m_pieces.size(); ++i) {
        if (currentPos + m_pieces[i].length > position) {
            return i;
        }
        currentPos += m_pieces[i].length;
    }
    return m_pieces.size() - 1;
}

// 行操作实现
void PieceTable::updateLineIndex() const
{
    if (!m_lineIndexDirty)
        return;

    m_lineStarts.clear();
    m_lineStarts.append(0);

    QString fullText = getFullText();
    for (int i = 0; i < fullText.length(); ++i) {
        if (fullText[i] == '\n') {
            m_lineStarts.append(i + 1);
        }
    }

    m_lineIndexDirty = false;
}

int PieceTable::getLineCount() const
{
    updateLineIndex();
    return m_lineStarts.size();
}

int PieceTable::getLineStart(int lineNumber) const
{
    updateLineIndex();
    if (lineNumber < 0 || lineNumber >= m_lineStarts.size())
        return 0;
    return m_lineStarts[lineNumber];
}

int PieceTable::getLineEnd(int lineNumber) const
{
    updateLineIndex();
    if (lineNumber < 0 || lineNumber >= m_lineStarts.size())
        return 0;

    if (lineNumber == m_lineStarts.size() - 1) {
        return length();
    }
    return m_lineStarts[lineNumber + 1] - 1; // 不包括换行符
}

int PieceTable::getLineLength(int lineNumber) const
{
    return getLineEnd(lineNumber) - getLineStart(lineNumber);
}

QString PieceTable::getLine(int lineNumber) const
{
    int start = getLineStart(lineNumber);
    int end = getLineEnd(lineNumber);
    return getText(start, end - start);
}

int PieceTable::positionToLine(int position) const
{
    updateLineIndex();
    position = qBound(0, position, length());

    // 二分查找
    int left = 0, right = m_lineStarts.size() - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (m_lineStarts[mid] <= position) {
            if (mid == m_lineStarts.size() - 1 || m_lineStarts[mid + 1] > position) {
                return mid;
            }
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }
    return 0;
}

int PieceTable::positionToColumn(int position) const
{
    int line = positionToLine(position);
    return position - getLineStart(line);
}

int PieceTable::lineColumnToPosition(int line, int column) const
{
    updateLineIndex();
    if (line < 0 || line >= m_lineStarts.size())
        return 0;

    int lineStart = getLineStart(line);
    int lineLength = getLineLength(line);
    column = qBound(0, column, lineLength);

    return lineStart + column;
}

// ==============================================================================
// ChunkedTextStorage 实现
// ==============================================================================

ChunkedTextStorage::ChunkedTextStorage(const QString& filePath)
    : m_filePath(filePath)
{
    // 初始化时加载文件并分块
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件进行分块存储:" << filePath;
        return;
    }

    QTextStream stream(&file);
    stream.setAutoDetectUnicode(true);

    QString buffer;
    int chunkIndex = 0;

    while (!stream.atEnd()) {
        QString line = stream.readLine() + '\n';

        if (buffer.length() + line.length() > CHUNK_SIZE) {
            // 当前chunk已满，创建新chunk
            if (!buffer.isEmpty()) {
                Chunk chunk;
                chunk.data = buffer;
                chunk.loaded = true;
                chunk.filePath = QString("%1.chunk%2").arg(filePath).arg(chunkIndex);
                m_chunks.append(chunk);

                // 可选择性地将chunk保存到临时文件
                saveChunkToFile(chunk, chunkIndex);

                buffer.clear();
                chunkIndex++;
            }
        }

        buffer.append(line);
    }

    // 处理最后一个chunk
    if (!buffer.isEmpty()) {
        Chunk chunk;
        chunk.data = buffer;
        chunk.loaded = true;
        chunk.filePath = QString("%1.chunk%2").arg(filePath).arg(chunkIndex);
        m_chunks.append(chunk);
        saveChunkToFile(chunk, chunkIndex);
    }

    file.close();
}

void ChunkedTextStorage::loadChunk(int chunkIndex) const
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size())
        return;

    Chunk& chunk = const_cast<Chunk&>(m_chunks[chunkIndex]);
    if (chunk.loaded)
        return;

    // 从文件加载chunk
    QFile file(chunk.filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setAutoDetectUnicode(true);
        chunk.data = stream.readAll();
        chunk.loaded = true;
        file.close();
    }
    else {
        qWarning() << "无法加载chunk:" << chunk.filePath;
    }
}

void ChunkedTextStorage::unloadChunk(int chunkIndex) const
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size())
        return;

    Chunk& chunk = const_cast<Chunk&>(m_chunks[chunkIndex]);
    if (!chunk.loaded)
        return;

    // 将chunk保存到文件并从内存中卸载
    saveChunkToFile(chunk, chunkIndex);
    chunk.data.clear();
    chunk.loaded = false;
}

void ChunkedTextStorage::saveChunkToFile(const Chunk& chunk, int chunkIndex) const
{
    QDir dir = QFileInfo(m_filePath).dir();
    QString tempDir = dir.filePath(".evaedit_chunks");

    if (!QDir(tempDir).exists()) {
        QDir().mkpath(tempDir);
    }

    QString chunkFilePath = QDir(tempDir).filePath(QString("chunk_%1").arg(chunkIndex));

    QFile file(chunkFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setAutoDetectUnicode(true);
        stream << chunk.data;
        file.close();

        // 更新chunk的文件路径
        const_cast<Chunk&>(chunk).filePath = chunkFilePath;
    }
}

void ChunkedTextStorage::insert(int position, const QString& text)
{
    if (text.isEmpty())
        return;

    // 找到插入位置所在的chunk
    int currentPos = 0;
    int chunkIndex = -1;
    int offsetInChunk = 0;

    for (int i = 0; i < m_chunks.size(); ++i) {
        loadChunk(i);
        int chunkLength = m_chunks[i].data.length();

        if (currentPos + chunkLength >= position) {
            chunkIndex = i;
            offsetInChunk = position - currentPos;
            break;
        }

        currentPos += chunkLength;
        unloadChunk(i); // 卸载不需要的chunk
    }

    if (chunkIndex == -1) {
        // 在末尾插入，创建新chunk
        Chunk newChunk;
        newChunk.data = text;
        newChunk.loaded = true;
        newChunk.filePath = QString("%1.chunk%2").arg(m_filePath).arg(m_chunks.size());
        m_chunks.append(newChunk);
    }
    else {
        // 在现有chunk中插入
        loadChunk(chunkIndex);
        Chunk& chunk = m_chunks[chunkIndex];
        chunk.data.insert(offsetInChunk, text);

        // 如果chunk过大，分割它
        if (chunk.data.length() > CHUNK_SIZE * 2) {
            splitChunk(chunkIndex);
        }
    }
}

void ChunkedTextStorage::remove(int position, int length)
{
    if (length <= 0)
        return;

    int endPosition = position + length;
    int currentPos = 0;

    for (int i = 0; i < m_chunks.size(); ++i) {
        loadChunk(i);
        Chunk& chunk = m_chunks[i];
        int chunkStart = currentPos;
        int chunkEnd = currentPos + chunk.data.length();

        if (chunkEnd <= position) {
            // chunk完全在删除范围之前
            currentPos = chunkEnd;
            unloadChunk(i);
            continue;
        }

        if (chunkStart >= endPosition) {
            // chunk完全在删除范围之后
            break;
        }

        // chunk与删除范围有交集
        int removeStart = qMax(position - chunkStart, 0);
        int removeEnd = qMin(endPosition - chunkStart, chunk.data.length());
        int removeLength = removeEnd - removeStart;

        if (removeLength > 0) {
            chunk.data.remove(removeStart, removeLength);
        }

        currentPos = chunkEnd;
    }

    // 清理空chunk
    for (int i = m_chunks.size() - 1; i >= 0; --i) {
        if (m_chunks[i].loaded && m_chunks[i].data.isEmpty()) {
            // 删除chunk文件
            QFile::remove(m_chunks[i].filePath);
            m_chunks.removeAt(i);
        }
    }
}

void ChunkedTextStorage::replace(int position, int length, const QString& text)
{
    remove(position, length);
    insert(position, text);
}

QString ChunkedTextStorage::getText(int position, int length) const
{
    if (length <= 0)
        return QString();

    QString result;
    result.reserve(length);

    int currentPos = 0;
    int remainingLength = length;

    for (int i = 0; i < m_chunks.size() && remainingLength > 0; ++i) {
        loadChunk(i);
        const Chunk& chunk = m_chunks[i];
        int chunkStart = currentPos;
        int chunkEnd = currentPos + chunk.data.length();

        if (chunkEnd <= position) {
            currentPos = chunkEnd;
            unloadChunk(i);
            continue;
        }

        if (chunkStart >= position + length)
            break;

        // 计算在当前chunk中需要提取的范围
        int extractStart = qMax(position - chunkStart, 0);
        int extractEnd = qMin(position + length - chunkStart, chunk.data.length());
        int extractLength = extractEnd - extractStart;

        if (extractLength > 0) {
            result.append(chunk.data.mid(extractStart, extractLength));
            remainingLength -= extractLength;
        }

        currentPos = chunkEnd;

        // 如果不再需要这个chunk，卸载它
        if (extractEnd == chunk.data.length()) {
            unloadChunk(i);
        }
    }

    return result;
}

QString ChunkedTextStorage::getFullText() const
{
    return getText(0, length());
}

int ChunkedTextStorage::length() const
{
    int totalLength = 0;
    for (int i = 0; i < m_chunks.size(); ++i) {
        if (m_chunks[i].loaded) {
            totalLength += m_chunks[i].data.length();
        }
        else {
            // 需要加载chunk来获取长度，或者缓存长度信息
            loadChunk(i);
            totalLength += m_chunks[i].data.length();
            unloadChunk(i);
        }
    }
    return totalLength;
}

void ChunkedTextStorage::splitChunk(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size())
        return;

    Chunk& originalChunk = m_chunks[chunkIndex];
    if (!originalChunk.loaded)
        loadChunk(chunkIndex);

    if (originalChunk.data.length() <= CHUNK_SIZE)
        return;

    // 分割chunk
    int splitPoint = CHUNK_SIZE;

    // 尝试在换行符处分割
    for (int i = CHUNK_SIZE; i > CHUNK_SIZE / 2; --i) {
        if (i < originalChunk.data.length() && originalChunk.data[i] == '\n') {
            splitPoint = i + 1;
            break;
        }
    }

    QString firstPart = originalChunk.data.left(splitPoint);
    QString secondPart = originalChunk.data.mid(splitPoint);

    // 更新原chunk
    originalChunk.data = firstPart;

    // 创建新chunk
    Chunk newChunk;
    newChunk.data = secondPart;
    newChunk.loaded = true;
    newChunk.filePath = QString("%1.split%2").arg(originalChunk.filePath).arg(chunkIndex);

    m_chunks.insert(chunkIndex + 1, newChunk);
}

// 行操作实现（基于chunk的优化版本）
int ChunkedTextStorage::getLineCount() const
{
    int lineCount = 1; // 至少有一行

    for (int i = 0; i < m_chunks.size(); ++i) {
        loadChunk(i);
        const QString& chunkData = m_chunks[i].data;

        for (int j = 0; j < chunkData.length(); ++j) {
            if (chunkData[j] == '\n') {
                lineCount++;
            }
        }

        unloadChunk(i);
    }

    return lineCount;
}

int ChunkedTextStorage::getLineStart(int lineNumber) const
{
    if (lineNumber <= 0)
        return 0;

    int currentLine = 0;
    int currentPos = 0;

    for (int i = 0; i < m_chunks.size(); ++i) {
        loadChunk(i);
        const QString& chunkData = m_chunks[i].data;

        for (int j = 0; j < chunkData.length(); ++j) {
            if (chunkData[j] == '\n') {
                currentLine++;
                if (currentLine == lineNumber) {
                    unloadChunk(i);
                    return currentPos + j + 1;
                }
            }
        }

        currentPos += chunkData.length();
        unloadChunk(i);
    }

    return currentPos;
}

int ChunkedTextStorage::getLineEnd(int lineNumber) const
{
    int start = getLineStart(lineNumber);
    int pos = start;

    // 从start开始找到下一个换行符或文件结尾
    QString text = getText(start, length() - start);
    int newlinePos = text.indexOf('\n');

    if (newlinePos == -1) {
        return length();
    }

    return start + newlinePos;
}

int ChunkedTextStorage::getLineLength(int lineNumber) const
{
    return getLineEnd(lineNumber) - getLineStart(lineNumber);
}

QString ChunkedTextStorage::getLine(int lineNumber) const
{
    int start = getLineStart(lineNumber);
    int end = getLineEnd(lineNumber);
    return getText(start, end - start);
}

int ChunkedTextStorage::positionToLine(int position) const
{
    position = qBound(0, position, length());

    int currentLine = 0;
    int currentPos = 0;

    for (int i = 0; i < m_chunks.size(); ++i) {
        loadChunk(i);
        const QString& chunkData = m_chunks[i].data;

        if (currentPos + chunkData.length() > position) {
            // position在当前chunk中
            int offsetInChunk = position - currentPos;
            for (int j = 0; j < offsetInChunk; ++j) {
                if (chunkData[j] == '\n') {
                    currentLine++;
                }
            }
            unloadChunk(i);
            return currentLine;
        }

        // position在当前chunk之后，计算当前chunk的行数
        for (int j = 0; j < chunkData.length(); ++j) {
            if (chunkData[j] == '\n') {
                currentLine++;
            }
        }

        currentPos += chunkData.length();
        unloadChunk(i);
    }

    return currentLine;
}

int ChunkedTextStorage::positionToColumn(int position) const
{
    int line = positionToLine(position);
    return position - getLineStart(line);
}

int ChunkedTextStorage::lineColumnToPosition(int line, int column) const
{
    int lineStart = getLineStart(line);
    int lineLength = getLineLength(line);
    column = qBound(0, column, lineLength);
    return lineStart + column;
}