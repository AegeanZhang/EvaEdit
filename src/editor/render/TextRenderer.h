#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <QQuickPaintedItem>
#include <QPainter>
#include <QTextLayout>
#include <QFont>
#include <QColor>
#include <QRectF>
#include <QPoint>
#include <QList>
#include <QTimer>
#include <qqmlintegration.h>

// 前向声明
class DocumentModel;
class LayoutEngine;
class CursorManager;
class SelectionManager;
class SyntaxHighlighter;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;
class QInputMethodEvent;

// 必要的结构体声明
struct Token;
struct SelectionRange;
struct Cursor;

// 声明不透明指针类型
Q_DECLARE_OPAQUE_POINTER(DocumentModel*)

// 文本渲染器
class TextRenderer : public QQuickPaintedItem {
    Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(DocumentModel* document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(bool lineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY lineNumbersChanged)
        Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
        Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
        Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
        Q_PROPERTY(int scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
        Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)

public:
    explicit TextRenderer(QQuickItem* parent = nullptr);
    ~TextRenderer();

    // 属性访问器
    DocumentModel* document() const;
    void setDocument(DocumentModel* document);

    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    QFont font() const;
    void setFont(const QFont& font);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);

    QColor textColor() const;
    void setTextColor(const QColor& color);

    int scrollX() const;
    void setScrollX(int x);

    int scrollY() const;
    void setScrollY(int y);

    // 渲染控制
    void paint(QPainter* painter) override;

    // 坐标转换
    Q_INVOKABLE QPoint positionToPoint(int position) const;
    Q_INVOKABLE int pointToPosition(const QPointF& point) const;
    Q_INVOKABLE QRect lineRect(int lineNumber) const;

    // 可见性
    Q_INVOKABLE QList<int> getVisibleLines() const;
    Q_INVOKABLE void ensurePositionVisible(int position);
    Q_INVOKABLE void ensureLineVisible(int lineNumber);

signals:
    void documentChanged();
    void lineNumbersChanged();
    void wordWrapChanged();
    void fontChanged();
    void backgroundColorChanged();
    void textColorChanged();
    void scrollXChanged();
    void scrollYChanged();
    void painted();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private slots:
    void onDocumentChanged();
    void onLayoutChanged();
    void onCursorChanged();
    void onSelectionChanged();
    void onGeometryChanged();

private:
    DocumentModel* m_document = nullptr;
    LayoutEngine* m_layoutEngine = nullptr;
    CursorManager* m_cursorManager = nullptr;
    SelectionManager* m_selectionManager = nullptr;
    SyntaxHighlighter* m_syntaxHighlighter = nullptr;

    bool m_showLineNumbers = true;
    bool m_wordWrap = false;
    QFont m_font;
    QColor m_backgroundColor = Qt::white;
    QColor m_textColor = Qt::black;
    int m_scrollX = 0;
    int m_scrollY = 0;

    qreal m_lineNumberWidth = 0;
    int m_lastClickCount = 0;
    qint64 m_lastClickTime = 0;

    // 私有渲染方法
    void paintBackground(QPainter* painter, const QRectF& rect);
    void paintLineNumbers(QPainter* painter, const QRectF& rect);
    void paintText(QPainter* painter, const QRectF& rect);
    void paintHighlightedLine(QPainter* painter, const QString& lineText,
        const QList<Token>& tokens, const QPointF& position);
    void paintSelections(QPainter* painter, const QRectF& rect);
    void paintCursors(QPainter* painter, const QRectF& rect);
    void paintCurrentLine(QPainter* painter, const QRectF& rect);

    // 私有辅助方法
    void updateLineNumberWidth();
    QRectF textArea() const;
    QRectF lineNumberArea() const;
    QList<QRect> getSelectionRects(const SelectionRange& selection) const;
    void handleVerticalMovement(QKeyEvent* event);
    int getClickCount(QMouseEvent* event);

    void initializeComponents();
    void connectSignals();
};

#endif // TEXT_RENDERER_H