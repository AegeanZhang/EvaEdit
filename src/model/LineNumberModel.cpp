#include "LineNumberModel.h"

#include <QQmlInfo>
#include <QFontMetrics>

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
