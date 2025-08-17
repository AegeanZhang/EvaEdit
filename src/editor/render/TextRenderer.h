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
class InputManager;
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
        Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
        Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
        Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
        Q_PROPERTY(int scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
        Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
        // Line Number区域的设置
        Q_PROPERTY(bool lineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY lineNumbersChanged)
        Q_PROPERTY(QColor lineNumberSeparatorColor READ lineNumberSeparatorColor WRITE setLineNumberSeparatorColor NOTIFY lineNumberSeparatorColorChanged)
        // 这里想从QML中设置额外的宽度，TextRenderer通过计算来确定行号的宽度，我们这里可以设置这个属性来增加额外的宽度
        Q_PROPERTY(qreal lineNumberExtraWidth READ lineNumberExtraWidth WRITE setLineNumberExtraWidth NOTIFY lineNumberExtraWidthChanged)

public:
    explicit TextRenderer(QQuickItem* parent = nullptr);
    ~TextRenderer();

    // 属性访问器
    DocumentModel* document() const;
    void setDocument(DocumentModel* document);

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

    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);

    QColor lineNumberSeparatorColor() const;
    void setLineNumberSeparatorColor(const QColor& color);

    //int lineNumberAreaWidth() const;
    //void setLineNumberAreaWidth(int width);

    int lineNumberExtraWidth() const;
    void setLineNumberExtraWidth(int width);

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
public:
    // 命令处理方法
    Q_INVOKABLE void handleMoveCursorLeft();
    Q_INVOKABLE void handleMoveCursorRight();
    Q_INVOKABLE void handleMoveCursorUp();
    Q_INVOKABLE void handleMoveCursorDown();
    Q_INVOKABLE void handleMoveCursorWordLeft();
    Q_INVOKABLE void handleMoveCursorWordRight();
    Q_INVOKABLE void handleMoveCursorLineStart();
    Q_INVOKABLE void handleMoveCursorLineEnd();
    Q_INVOKABLE void handleMoveCursorDocumentStart();
    Q_INVOKABLE void handleMoveCursorDocumentEnd();

    Q_INVOKABLE void handleSelectLeft();
    Q_INVOKABLE void handleSelectRight();
    Q_INVOKABLE void handleSelectUp();
    Q_INVOKABLE void handleSelectDown();
    Q_INVOKABLE void handleSelectAll();
    Q_INVOKABLE void handleSelectWord();
    Q_INVOKABLE void handleSelectLine();

    Q_INVOKABLE void handleInsertText(const QString& text);
    Q_INVOKABLE void handleNewLine();
    Q_INVOKABLE void handleTab();
    Q_INVOKABLE void handleDeleteLeft();
    Q_INVOKABLE void handleDeleteRight();
    Q_INVOKABLE void handleDeleteWordLeft();
    Q_INVOKABLE void handleDeleteWordRight();

    Q_INVOKABLE void handleCut();
    Q_INVOKABLE void handleCopy();
    Q_INVOKABLE void handlePaste();

    Q_INVOKABLE void handleUndo();
    Q_INVOKABLE void handleRedo();

signals:
    void documentChanged();
    void wordWrapChanged();
    void fontChanged();
    void backgroundColorChanged();
    void textColorChanged();
    void scrollXChanged();
    void scrollYChanged();
    void painted();
    void lineNumbersChanged();
    void lineNumberSeparatorColorChanged();
    void lineNumberExtraWidthChanged();

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
    InputManager* m_inputManager = nullptr;

    bool m_wordWrap = false;
    QFont m_font;
    QColor m_backgroundColor = Qt::white;
    QColor m_textColor = Qt::black;
    int m_scrollX = 0;
    int m_scrollY = 0;

    bool m_showLineNumbers = true;
    QColor m_lineNumberSeparatorColor = Qt::white;
    int m_lineNumberExtraWidth = 20;

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
    int getClickCount(QMouseEvent* event);

    // 移动方向枚举
    enum class MoveDirection {
        Left,
        Right,
        Up,
        Down
    };

    // 移动单位枚举
    enum class MoveUnit {
        Character,    // 字符级
        Word,         // 单词级
        Line,         // 行级（到行首/行尾）
        Page,         // 页面级
        Document      // 文档级
    };

    void handleVerticalMovement(QKeyEvent* event);
    void handleHorizontalMovement(QKeyEvent* event);
    // 统一的移动处理
    void handleMovement(MoveDirection direction, bool extend, MoveUnit unit = MoveUnit::Character);

    int calculateMovementTarget(int currentPos, MoveDirection direction, MoveUnit unit) const;
    int calculateCharacterMovement(int currentPos, MoveDirection direction) const;
    int calculateWordMovement(int currentPos, MoveDirection direction) const;
    int calculateLineMovement(int currentPos, MoveDirection direction) const;
    int calculatePageMovement(int currentPos, MoveDirection direction) const;
    int calculateDocumentMovement(int currentPos, MoveDirection direction) const;

    int calculateVerticalCharacterMovement(int currentPos, MoveDirection direction) const;
    int findWordBoundary(int position, bool forward) const;
    int findParagraphBoundary(int position, bool forward) const;
    bool isWordCharacter(QChar ch) const;

    void initializeComponents();
    void connectSignals();
    void setupInputManager();
};

#endif // TEXT_RENDERER_H