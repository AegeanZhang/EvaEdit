#ifndef __FILE_SYSTEM_MODEL_H__
#define __FILE_SYSTEM_MODEL_H__

#include <QFileSystemModel>
#include <QQuickTextDocument>

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
public:
    explicit FileSystemModel(QObject* parent = nullptr);

    // Functions invokable from QML
    Q_INVOKABLE QString readFile(const QString& filePath);
    Q_INVOKABLE int currentLineNumber(QQuickTextDocument* textDocument, int cursorPosition);
    Q_INVOKABLE void setDirectory(const QString& path);

    // Overridden functions
    int columnCount(const QModelIndex& parent) const override;

    // Member functions from here
    QModelIndex rootIndex() const;
    void setRootIndex(const QModelIndex index);
    void setInitialDirectory(const QString& path = getDefaultRootDir());

    static QString getDefaultRootDir();

signals:
    void rootIndexChanged();

private:
    QModelIndex m_rootIndex;
};

#endif // __FILE_SYSTEM_MODEL_H__
