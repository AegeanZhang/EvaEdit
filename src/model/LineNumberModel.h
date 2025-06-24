#ifndef __LINE_NUMBER_MODEL_H__
#define __LINE_NUMBER_MODEL_H__

#include <QAbstractItemModel>
#include <QQmlEngine>

class LineNumberModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int lineCount READ lineCount WRITE setLineCount NOTIFY lineCountChanged)

public:
    explicit LineNumberModel(QObject* parent = nullptr);

    int lineCount() const;
    void setLineCount(int lineCount);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

signals:
    void lineCountChanged();

private:
    int m_lineCount = 0;
};

#endif // __LINE_NUMBER_MODEL_H__