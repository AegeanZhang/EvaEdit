#ifndef CURSOR_MANAGER_H
#define CURSOR_MANAGER_H

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QList>
#include <QColor>

struct Cursor {
    int position = 0;
    int anchorPosition = 0; // 选择起始位置

    bool hasSelection() const { return position != anchorPosition; }
    int selectionStart() const { return qMin(position, anchorPosition); }
    int selectionEnd() const { return qMax(position, anchorPosition); }
    int selectionLength() const { return qAbs(position - anchorPosition); }
};

struct Selection {
    int start = 0;
    int end = 0;

    Selection(int s = 0, int e = 0) : start(s), end(e) {}
    bool isEmpty() const { return start == end; }
    int length() const { return end - start; }
    bool contains(int position) const { return position >= start && position < end; }
};

// 光标和选择管理器
class CursorManager : public QObject {
    Q_OBJECT

public:
    explicit CursorManager(QObject* parent = nullptr);

    // 单光标操作
    void setCursorPosition(int position, bool select = false);
    int cursorPosition() const;

    void setAnchorPosition(int position);
    int anchorPosition() const;

    // 选择操作
    void setSelection(int start, int end);
    void selectAll(int textLength);
    void clearSelection();
    bool hasSelection() const;
    Selection selection() const;

    // 多光标操作
    void addCursor(int position);
    void removeCursor(int index);
    void clearAllCursors();
    QList<Cursor> cursors() const;
    int cursorCount() const;

    void setMultiCursorMode(bool enabled);
    bool isMultiCursorMode() const;

    // 光标移动
    void moveCursor(int delta, bool select = false);
    void moveCursorToLine(int line, bool select = false);
    void moveCursorToColumn(int column, bool select = false);
    void moveCursorToLineColumn(int line, int column, bool select = false);

    // 可见性管理
    void ensureCursorVisible();
    QRect cursorRect() const;
    void setCursorRect(const QRect& rect);

    // 闪烁控制
    void startBlinking();
    void stopBlinking();
    void setBlink(bool visible);
    bool isBlinkVisible() const;

    void setBlinkInterval(int milliseconds);
    int blinkInterval() const;

    // 扩展功能
    QList<Selection> getAllSelections() const;
    QList<int> getAllPositions() const;
    bool isPositionSelected(int position) const;
    void setSystemBlinkInterval();
    void pauseBlinking();
    void resumeBlinking();
    void resetBlink();
    Selection getPrimarySelection() const;
    void setCursorWidth(int width);
    void setCursorColor(const QColor& color);
    QString debugString() const;

signals:
    void cursorPositionChanged(int position);
    void selectionChanged(const Selection& selection);
    void cursorVisibilityChanged(bool visible);
    void cursorsChanged(const QList<Cursor>& cursors);
    void ensureVisibleRequested(const QRect& rect);

private slots:
    void onBlinkTimer();

private:
    QList<Cursor> m_cursors;
    int m_primaryCursorIndex = 0;
    bool m_multiCursorMode = false;

    QRect m_cursorRect;
    bool m_blinkVisible = true;
    QTimer* m_blinkTimer;
    int m_blinkInterval = 530; // 系统默认值

    // 样式属性
    int m_cursorWidth = 1;
    QColor m_cursorColor = QColor(Qt::black);

    void updatePrimaryCursor();
    void mergeCursors();
    void sortCursors();
};

#endif // CURSOR_MANAGER_H