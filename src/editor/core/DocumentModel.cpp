#include "DocumentModel.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QMimeDatabase>
#include <QStringConverter>
#include <QDateTime>
#include <QTextStream>

DocumentModel::DocumentModel(QObject* parent)
    : QObject(parent)
    , m_textStorage(std::make_unique<PieceTable>())
    , m_undoSystem(std::make_unique<UndoSystem>(this))
    , m_type(PlainText)
    , m_encoding(UTF8)
    , m_modified(false)
    , m_readOnly(false)
{
    // 连接撤销系统信号
    connect(m_undoSystem.get(), &UndoSystem::undoAvailable,
        this, &DocumentModel::undoAvailable);
    connect(m_undoSystem.get(), &UndoSystem::redoAvailable,
        this, &DocumentModel::redoAvailable);
    connect(m_undoSystem.get(), &UndoSystem::stackChanged,
        this, [this]() {
            setModified(true);
        });
}

DocumentModel::~DocumentModel()
{
    // 智能指针会自动清理资源
}

// ==============================================================================
// 文档属性实现
// ==============================================================================

void DocumentModel::setFilePath(const QString& path)
{
    if (m_filePath == path)
        return;

    m_filePath = path;

    // 根据文件扩展名推断文档类型
    if (!path.isEmpty()) {
        QFileInfo fileInfo(path);
        QString suffix = fileInfo.suffix().toLower();

        if (suffix == "md" || suffix == "markdown") {
            setType(Markdown);
        }
        else if (suffix == "cpp" || suffix == "h" || suffix == "c" ||
            suffix == "hpp" || suffix == "cc" || suffix == "cxx" ||
            suffix == "py" || suffix == "js" || suffix == "ts" ||
            suffix == "java" || suffix == "cs" || suffix == "go" ||
            suffix == "rs" || suffix == "swift" || suffix == "kt") {
            setType(Code);
        }
        else {
            setType(PlainText);
        }
    }

    emit filePathChanged(m_filePath);
}

void DocumentModel::setType(DocumentType type)
{
    if (m_type == type)
        return;

    m_type = type;
    // 类型变化可能影响语法高亮等，这里可以发出相应信号
}

void DocumentModel::setEncoding(Encoding encoding)
{
    if (m_encoding == encoding)
        return;

    m_encoding = encoding;
    emit encodingChanged(m_encoding);
}

void DocumentModel::setModified(bool modified)
{
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged(m_modified);
}

void DocumentModel::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;
    // 只读状态变化可能需要更新UI
    emit readOnlyChanged(m_readOnly);
}

// ==============================================================================
// 文本操作实现
// ==============================================================================

void DocumentModel::insertText(int position, const QString& text)
{
    if (m_readOnly || text.isEmpty())
        return;

    // 边界检查
    position = qBound(0, position, textLength());

    // 创建并执行撤销命令
    auto command = m_undoSystem->createInsertCommand(this, position, text);
    m_undoSystem->executeCommand(std::move(command));

    // 创建变更记录
    TextChange change;
    change.position = position;
    change.removedLength = 0;
    change.insertedText = text;
    change.timestamp = QDateTime::currentDateTime();

    m_changeHistory.append(change);

    // 限制历史记录数量
    if (m_changeHistory.size() > 1000) {
        m_changeHistory.removeFirst();
    }

    setModified(true);
}

void DocumentModel::removeText(int position, int length)
{
    if (m_readOnly || length <= 0)
        return;

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    if (length == 0)
        return;

    // 创建并执行撤销命令
    auto command = m_undoSystem->createRemoveCommand(this, position, length);
    m_undoSystem->executeCommand(std::move(command));

    // 创建变更记录
    TextChange change;
    change.position = position;
    change.removedLength = length;
    change.insertedText = "";
    change.timestamp = QDateTime::currentDateTime();

    m_changeHistory.append(change);

    // 限制历史记录数量
    if (m_changeHistory.size() > 1000) {
        m_changeHistory.removeFirst();
    }

    setModified(true);
}

void DocumentModel::replaceText(int position, int length, const QString& text)
{
    if (m_readOnly)
        return;

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    // 获取要替换的文本
    QString oldText = length > 0 ? getText(position, length) : "";

    // 创建并执行撤销命令
    auto command = m_undoSystem->createReplaceCommand(this, position, oldText, text);
    m_undoSystem->executeCommand(std::move(command));

    // 创建变更记录
    TextChange change;
    change.position = position;
    change.removedLength = length;
    change.insertedText = text;
    change.timestamp = QDateTime::currentDateTime();

    m_changeHistory.append(change);

    // 限制历史记录数量
    if (m_changeHistory.size() > 1000) {
        m_changeHistory.removeFirst();
    }

    setModified(true);
}

QString DocumentModel::getText(int position, int length) const
{
    if (!m_textStorage)
        return QString();

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    if (length <= 0)
        return QString();

    return m_textStorage->getText(position, length);
}

QString DocumentModel::getFullText() const
{
    if (!m_textStorage)
        return QString();

    return m_textStorage->getFullText();
}

int DocumentModel::textLength() const
{
    if (!m_textStorage)
        return 0;

    return m_textStorage->length();
}

// ==============================================================================
// 行操作实现
// ==============================================================================

int DocumentModel::lineCount() const
{
    if (!m_textStorage)
        return 0;

    return m_textStorage->getLineCount();
}

QString DocumentModel::getLine(int lineNumber) const
{
    if (!m_textStorage || lineNumber < 0 || lineNumber >= lineCount())
        return QString();

    return m_textStorage->getLine(lineNumber);
}

int DocumentModel::positionToLine(int position) const
{
    if (!m_textStorage)
        return 0;

    position = qBound(0, position, textLength());
    return m_textStorage->positionToLine(position);
}

int DocumentModel::positionToColumn(int position) const
{
    if (!m_textStorage)
        return 0;

    position = qBound(0, position, textLength());
    return m_textStorage->positionToColumn(position);
}

int DocumentModel::lineColumnToPosition(int line, int column) const
{
    if (!m_textStorage)
        return 0;

    line = qBound(0, line, lineCount() - 1);
    QString lineText = getLine(line);
    column = qBound(0, column, lineText.length());

    return m_textStorage->lineColumnToPosition(line, column);
}

// ==============================================================================
// 撤销/重做实现
// ==============================================================================

bool DocumentModel::canUndo() const
{
    return m_undoSystem && m_undoSystem->canUndo();
}

bool DocumentModel::canRedo() const
{
    return m_undoSystem && m_undoSystem->canRedo();
}

void DocumentModel::undo()
{
    if (m_undoSystem && !m_readOnly) {
        m_undoSystem->undo();
    }
}

void DocumentModel::redo()
{
    if (m_undoSystem && !m_readOnly) {
        m_undoSystem->redo();
    }
}

void DocumentModel::clearUndoHistory()
{
    if (m_undoSystem) {
        m_undoSystem->clear();
    }
}

// ==============================================================================
// 文件操作实现
// ==============================================================================

bool DocumentModel::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filePath << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    // 检测文件编码
    Encoding detectedEncoding = detectFileEncoding(data);
    setEncoding(detectedEncoding);

    // 转换文本
    QString text = convertFromEncoding(data, detectedEncoding);

    // 判断是否为大文件（超过64MB使用分块存储）
    if (data.size() > 64 * 1024 * 1024) {
        m_textStorage = std::make_unique<ChunkedTextStorage>(filePath);
    }
    else {
        m_textStorage = std::make_unique<PieceTable>(text);
    }

    setFilePath(filePath);
    setModified(false);

    // 更新最后修改时间
    QFileInfo fileInfo(filePath);
    m_lastModified = fileInfo.lastModified();

    // 清空变更历史
    m_changeHistory.clear();

    // 清空撤销历史
    clearUndoHistory();

    // 发出文本变更信号
    TextChange change;
    change.position = 0;
    change.removedLength = 0;
    change.insertedText = text;
    change.timestamp = QDateTime::currentDateTime();
    emit textChanged(change);

    return true;
}

bool DocumentModel::saveToFile(const QString& filePath)
{
    QString targetPath = filePath.isEmpty() ? m_filePath : filePath;

    if (targetPath.isEmpty()) {
        qWarning() << "没有指定保存路径";
        return false;
    }

    // 确保目录存在
    QDir dir = QFileInfo(targetPath).dir();
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "无法创建目录:" << dir.path();
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "无法写入文件:" << targetPath << file.errorString();
        return false;
    }

    // 获取文本内容
    QString text = getFullText();

    // 根据编码转换文本
    QByteArray data = convertToEncoding(text, m_encoding);

    qint64 written = file.write(data);
    file.close();

    if (written != data.size()) {
        qWarning() << "文件写入不完整:" << targetPath;
        return false;
    }

    // 更新文档状态
    if (filePath != m_filePath) {
        setFilePath(targetPath);
    }

    setModified(false);

    // 更新最后修改时间
    QFileInfo fileInfo(targetPath);
    m_lastModified = fileInfo.lastModified();

    return true;
}

// ==============================================================================
// 搜索实现
// ==============================================================================

QList<int> DocumentModel::findText(const QString& pattern, bool caseSensitive, bool wholeWords) const
{
    QList<int> results;

    if (pattern.isEmpty() || !m_textStorage)
        return results;

    QString text = getFullText();
    if (text.isEmpty())
        return results;

    Qt::CaseSensitivity sensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if (wholeWords) {
        // 使用正则表达式进行整词匹配
        QString regexPattern = QString("\\b%1\\b").arg(QRegularExpression::escape(pattern));
        QRegularExpression regex(regexPattern);
        if (!caseSensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }

        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            results.append(match.capturedStart());
        }
    }
    else {
        // 简单字符串搜索
        int index = 0;
        while ((index = text.indexOf(pattern, index, sensitivity)) != -1) {
            results.append(index);
            index += pattern.length();
        }
    }

    return results;
}

// ==============================================================================
// 撤销系统专用方法 - 直接操作存储，不触发撤销命令
// ==============================================================================

void DocumentModel::insertTextDirect(int position, const QString& text)
{
    if (!m_textStorage || text.isEmpty())
        return;

    // 边界检查
    position = qBound(0, position, textLength());

    // 直接操作存储，不创建撤销命令
    m_textStorage->insert(position, text);
}

void DocumentModel::removeTextDirect(int position, int length)
{
    if (!m_textStorage || length <= 0)
        return;

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    if (length == 0)
        return;

    // 直接操作存储，不创建撤销命令
    m_textStorage->remove(position, length);
}

void DocumentModel::replaceTextDirect(int position, int length, const QString& text)
{
    if (!m_textStorage)
        return;

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    // 直接操作存储，不创建撤销命令
    m_textStorage->replace(position, length, text);
}

QString DocumentModel::getTextDirect(int position, int length) const
{
    if (!m_textStorage)
        return QString();

    // 边界检查
    position = qBound(0, position, textLength());
    length = qMin(length, textLength() - position);

    if (length <= 0)
        return QString();

    return m_textStorage->getText(position, length);
}

// ==============================================================================
// Qt6 编码相关的私有方法实现
// ==============================================================================

QStringConverter::Encoding DocumentModel::encodingToQt(Encoding encoding) const
{
    switch (encoding) {
    case UTF8:
        return QStringConverter::Utf8;
    case UTF16:
        return QStringConverter::Utf16;
    case ASCII:
        return QStringConverter::Latin1;
    case GBK:
        // Qt6 中 GBK 需要特殊处理，这里暂时使用 UTF-8
        return QStringConverter::Utf8;
    default:
        return QStringConverter::Utf8;
    }
}

DocumentModel::Encoding DocumentModel::detectFileEncoding(const QByteArray& data) const
{
    // 检查 BOM
    if (data.startsWith("\xEF\xBB\xBF")) {
        return UTF8;
    }
    if (data.startsWith("\xFF\xFE") || data.startsWith("\xFE\xFF")) {
        return UTF16;
    }

    // 检查是否包含非ASCII字符
    bool hasNonAscii = false;
    for (char byte : data) {
        if (static_cast<unsigned char>(byte) > 127) {
            hasNonAscii = true;
            break;
        }
    }

    if (!hasNonAscii) {
        return ASCII;
    }

    // 尝试UTF-8解码 - 使用 Qt6 正确的 API
    QStringConverter::Encoding qtEncoding = QStringConverter::Utf8;
    QStringDecoder decoder(qtEncoding);
    if (decoder.isValid()) {
        QString decoded = decoder.decode(data);
        if (!decoder.hasError()) {
            return UTF8;
        }
    }

    // UTF-8解码失败，可能是GBK，但暂时默认返回UTF-8
    return UTF8;
}

QString DocumentModel::convertFromEncoding(const QByteArray& data, Encoding encoding) const
{
    QStringConverter::Encoding qtEncoding = encodingToQt(encoding);

    QStringDecoder decoder(qtEncoding);
    if (decoder.isValid()) {
        QString result = decoder.decode(data);
        if (!decoder.hasError()) {
            return result;
        }
    }

    // fallback 到 UTF-8
    return QString::fromUtf8(data);
}

QByteArray DocumentModel::convertToEncoding(const QString& text, Encoding encoding) const
{
    QStringConverter::Encoding qtEncoding = encodingToQt(encoding);

    QStringEncoder encoder(qtEncoding);
    if (encoder.isValid()) {
        QByteArray result = encoder.encode(text);
        if (!encoder.hasError()) {
            return result;
        }
    }

    // fallback 到 UTF-8
    return text.toUtf8();
}

// ==============================================================================
// 批量操作支持
// ==============================================================================

void DocumentModel::beginBatchEdit()
{
    if (m_undoSystem) {
        m_undoSystem->beginBatchEdit();
    }
}

void DocumentModel::endBatchEdit()
{
    if (m_undoSystem) {
        m_undoSystem->endBatchEdit();
    }
}

// ==============================================================================
// 统计信息
// ==============================================================================

int DocumentModel::getCharacterCount() const
{
    return textLength();
}

int DocumentModel::getWordCount() const
{
    QString text = getFullText();
    if (text.isEmpty())
        return 0;

    // 简单的单词计数
    QRegularExpression wordRegex("\\b\\w+\\b");
    QRegularExpressionMatchIterator iterator = wordRegex.globalMatch(text);

    int count = 0;
    while (iterator.hasNext()) {
        iterator.next();
        count++;
    }

    return count;
}

int DocumentModel::getParagraphCount() const
{
    QString text = getFullText();
    if (text.isEmpty())
        return 0;

    // 计算段落数（以双换行符分隔）
    QStringList paragraphs = text.split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts);
    return paragraphs.size();
}

// ==============================================================================
// 文档状态
// ==============================================================================

bool DocumentModel::isEmpty() const
{
    return textLength() == 0;
}

bool DocumentModel::isLargeFile() const
{
    // 判断是否为大文件（超过10MB）
    return textLength() > 10 * 1024 * 1024;
}

QDateTime DocumentModel::lastModified() const
{
    return m_lastModified;
}

// ==============================================================================
// 快照功能
// ==============================================================================

QString DocumentModel::createSnapshot() const
{
    // 创建文档快照，可用于崩溃恢复
    return getFullText();
}

bool DocumentModel::restoreFromSnapshot(const QString& snapshot)
{
    if (m_readOnly)
        return false;

    // 从快照恢复文档
    m_textStorage = std::make_unique<PieceTable>(snapshot);
    clearUndoHistory();
    setModified(true);

    // 发出文本变更信号
    TextChange change;
    change.position = 0;
    change.removedLength = 0;
    change.insertedText = snapshot;
    change.timestamp = QDateTime::currentDateTime();
    emit textChanged(change);

    return true;
}

// ==============================================================================
// 文件监控相关
// ==============================================================================

bool DocumentModel::isFileModifiedExternally() const
{
    if (m_filePath.isEmpty())
        return false;

    QFileInfo fileInfo(m_filePath);
    if (!fileInfo.exists())
        return true; // 文件被删除

    return fileInfo.lastModified() > m_lastModified;
}

void DocumentModel::refreshFromFile()
{
    if (m_filePath.isEmpty())
        return;

    if (m_modified) {
        // 如果有未保存的修改，需要用户确认是否重新加载
        // 这里简化处理，直接返回
        return;
    }

    loadFromFile(m_filePath);
}