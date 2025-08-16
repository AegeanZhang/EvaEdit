#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <QObject>
#include <QList>
#include <QRect>
#include <QColor>
#include <QStringList>
#include <QDateTime>

// 前向声明
class DocumentModel;
struct TextChange;

enum class SelectionMode {
    Character,  // 字符选择
    Word,       // 单词选择
    Line,       // 行选择
    Block       // 块选择（矩形选择）
};

struct SelectionRange {
    int start = 0;
    int end = 0;
    SelectionMode mode = SelectionMode::Character;

    SelectionRange(int s = 0, int e = 0, SelectionMode m = SelectionMode::Character)
        : start(s), end(e), mode(m) {
    }

    bool isEmpty() const { return start == end; }
    int length() const { return qAbs(end - start); }
    bool contains(int position) const {
        return position >= qMin(start, end) && position < qMax(start, end);
    }

    SelectionRange normalized() const {
        return start <= end ? *this : SelectionRange(end, start, mode);
    }
};

// 选择管理器
class SelectionManager : public QObject {
    Q_OBJECT

public:
    explicit SelectionManager(QObject* parent = nullptr);

    // 文档关联
    void setDocument(DocumentModel* document);
    DocumentModel* document() const;

    // 选择操作
    void setSelection(const SelectionRange& range);
    void addSelection(const SelectionRange& range);
    void removeSelection(int index);
    void clearSelections();

    QList<SelectionRange> selections() const;
    SelectionRange primarySelection() const;
    bool hasSelection() const;

    // 选择模式
    void setSelectionMode(SelectionMode mode);
    SelectionMode selectionMode() const;

    // 文本相关选择
    void selectWord(int position);
    void selectLine(int lineNumber);
    void selectParagraph(int position);
    void selectAll(int textLength);

    // 块选择（矩形选择）
    void startBlockSelection(int startLine, int startColumn);
    void updateBlockSelection(int endLine, int endColumn);
    void endBlockSelection();
    bool isBlockSelectionActive() const;

    // 选择扩展
    void extendSelectionToWord(int position);
    void extendSelectionToLine(int lineNumber);
    void extendSelectionTo(int position);

    // 选择合并
    void mergeOverlappingSelections();
    void sortSelections();

    // 视觉效果
    QList<QRect> getSelectionRects() const;
    void setSelectionColor(const QColor& color);
    QColor selectionColor() const;

    // 剪贴板支持
    QString getSelectedText() const;
    QStringList getSelectedTexts() const; // 多选择时

    // 高级选择功能
    void selectAllMatches(const QString& pattern, bool caseSensitive = false, bool wholeWords = false);
    void invertSelection(int textLength);
    void selectWordsInRange(int start, int end);
    void smartSelect(int position);

    // 调试功能
    QString debugString() const;

signals:
    void selectionsChanged(const QList<SelectionRange>& selections);
    void selectionModeChanged(SelectionMode mode);
    void blockSelectionChanged(bool active);

private slots:
    void onDocumentTextChanged(const TextChange& change);

private:
    QList<SelectionRange> m_selections;
    SelectionMode m_selectionMode = SelectionMode::Character;

    // 块选择状态
    bool m_blockSelectionActive = false;
    int m_blockStartLine = 0;
    int m_blockStartColumn = 0;
    int m_blockEndLine = 0;
    int m_blockEndColumn = 0;

    QColor m_selectionColor = QColor(0, 120, 215, 100); // 默认蓝色

    DocumentModel* m_document = nullptr; // 引用文档模型

    // 私有辅助方法
    void expandSelectionToWord(SelectionRange& range);
    void expandSelectionToLine(SelectionRange& range);
    QList<SelectionRange> createBlockSelection() const;
    void adjustSelectionForTextChange(SelectionRange& selection, const TextChange& change);
    void selectQuotedText(int position, QChar quote);
    void selectBracketContent(int position);
};

#endif // SELECTION_MANAGER_H