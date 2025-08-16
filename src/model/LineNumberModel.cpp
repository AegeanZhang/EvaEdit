#include "LineNumberModel.h"

#include <QQmlInfo>
#include <QFontMetrics>
#include <QTextCursor>
#include <QTextBlock>

#include <algorithm>

/*!
    When using an integer model based on the line count of the editor,
    any changes in that line count cause all delegates to be destroyed
    and recreated. That's inefficient, so instead, we add/remove model
    items as necessary ourselves, based on the lineCount property.
*/
LineNumberModel::LineNumberModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int LineNumberModel::lineCount() const
{
    return m_lineCount;
}

void LineNumberModel::setLineCount(int lineCount)
{
    if (lineCount < 0) {
        qmlWarning(this) << "lineCount must be greater than zero";
        return;
    }

    if (m_lineCount == lineCount)
        return;

    if (m_lineCount < lineCount) {
        beginInsertRows(QModelIndex(), m_lineCount, lineCount - 1);
        m_lineCount = lineCount;
        endInsertRows();
    }
    else if (m_lineCount > lineCount) {
        beginRemoveRows(QModelIndex(), lineCount, m_lineCount - 1);
        m_lineCount = lineCount;
        endRemoveRows();
    }

    emit lineCountChanged();
}

int LineNumberModel::rowCount(const QModelIndex&) const
{
    return m_lineCount;
}

QVariant LineNumberModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index) || role != Qt::DisplayRole)
        return QVariant();

    return index.row();
}

int LineNumberModel::calculateRowHeight(const QFont& font)
{
    QFontMetrics fm(font);
    int asciiHeight = fm.boundingRect(QStringLiteral("A")).height();
    int chineseHeight = fm.boundingRect(QStringLiteral("中")).height();
    int specialHeight = fm.boundingRect(QStringLiteral("😀")).height();

    return std::max({ asciiHeight, chineseHeight, specialHeight });
}

/*void LineNumberModel::setFixedLineHeight(QQuickTextDocument* textDocument, int height)
{
    if (!textDocument || !textDocument->textDocument()) {
        return;
    }

    QTextDocument* doc = textDocument->textDocument();
    QTextCursor cursor(doc);

    // 选择整个文档
    cursor.select(QTextCursor::Document);

    // 创建固定行高的格式
    QTextBlockFormat blockFormat = cursor.blockFormat();
    blockFormat.setLineHeight(height, QTextBlockFormat::FixedHeight);

    // 应用格式
    cursor.setBlockFormat(blockFormat);

    // 设置默认格式，确保新文本也使用固定行高
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::Start);
    QTextBlockFormat defaultFormat;
    defaultFormat.setLineHeight(height, QTextBlockFormat::FixedHeight);
    doc->setDefaultTextOption(QTextOption());
}*/

void LineNumberModel::setFixedLineHeight(QQuickTextDocument* textDocument, int height)
{
    if (!textDocument || !textDocument->textDocument()) {
        return;
    }

    QTextDocument* doc = textDocument->textDocument();

    // 1. 设置文档的默认块格式，确保新文本也使用固定行高
    QTextBlockFormat defaultBlockFormat;
    defaultBlockFormat.setLineHeight(height, QTextBlockFormat::FixedHeight);

    // 使用QTextCursor设置默认块格式
    QTextCursor defaultCursor(doc);
    defaultCursor.movePosition(QTextCursor::Start);
    defaultCursor.setBlockFormat(defaultBlockFormat);

    // 2. 对现有的所有文本块应用固定行高
    QTextCursor cursor(doc);
    cursor.beginEditBlock(); // 开始编辑块，提高性能

    // 遍历所有文本块
    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        cursor.setPosition(block.position());
        QTextBlockFormat blockFormat = block.blockFormat();
        blockFormat.setLineHeight(height, QTextBlockFormat::FixedHeight);
        cursor.setBlockFormat(blockFormat);
        block = block.next();
    }

    cursor.endEditBlock(); // 结束编辑块

    // 3. 设置文档的默认文本选项（保持原有选项，不覆盖）
    QTextOption textOption = doc->defaultTextOption();
    doc->setDefaultTextOption(textOption);
}