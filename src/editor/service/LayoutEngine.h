#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <QObject>
#include <QFont>
#include <QFontMetrics>
#include <QTextLayout>
#include <QList>
#include <QRectF>
#include <QColor>
#include <QStringList>
#include "TokenTypes.h"

struct LineLayout {
    int lineNumber = 0;
    QTextLayout* layout = nullptr;
    qreal width = 0.0;
    qreal height = 0.0;
    bool visible = false;
    bool dirty = true;
    QList<Token> tokens; // 语法高亮tokens
};

struct ViewportInfo {
    QRectF rect;
    int firstVisibleLine = 0;
    int lastVisibleLine = 0;
    qreal scrollX = 0.0;
    qreal scrollY = 0.0;
};

// 布局引擎
class LayoutEngine : public QObject {
    Q_OBJECT

public:
    explicit LayoutEngine(QObject* parent = nullptr);
    ~LayoutEngine();

    // 字体和度量
    void setFont(const QFont& font);
    QFont font() const;
    QFontMetrics fontMetrics() const;

    qreal lineHeight() const;
    qreal characterWidth() const;
    qreal tabWidth() const;
    void setTabWidth(int characters);

    // 文本包装
    void setTextWidth(qreal width);
    qreal textWidth() const;

    void setWordWrap(bool enabled);
    bool wordWrap() const;

    // 布局管理
    void setText(const QString& text);
    void updateText(int position, int removedLength, const QString& addedText);

    LineLayout* getLineLayout(int lineNumber);
    void invalidateLineLayout(int lineNumber);
    void invalidateAllLayouts();

    // 视口管理
    void setViewport(const ViewportInfo& viewport);
    ViewportInfo viewport() const;

    // 坐标转换
    QPointF positionToPoint(int position) const;
    int pointToPosition(const QPointF& point) const;

    QRectF lineRect(int lineNumber) const;
    QRectF selectionRect(int startPos, int endPos) const;

    // 可见性优化
    QList<int> getVisibleLines() const;
    void ensureLayoutForVisibleLines();

    // 软换行支持
    int visualLineCount() const;
    int logicalLineToVisualLine(int logicalLine) const;
    int visualLineToLogicalLine(int visualLine) const;

    // 高级功能
    qreal getLineRenderHeight(int lineNumber) const;
    qreal getLineRenderWidth(int lineNumber) const;
    void setMaxCachedLayouts(int maxLayouts);
    void clearLayoutCache();
    int getCachedLayoutCount() const;
    qreal getTotalDocumentHeight() const;
    qreal getTotalDocumentWidth() const;
    QString getDebugInfo() const;
    bool validateLayouts() const;

signals:
    void layoutChanged();
    void viewportChanged();
    void lineLayoutUpdated(int lineNumber);

private:
    QFont m_font;
    QFontMetrics m_fontMetrics;
    qreal m_textWidth = -1;
    bool m_wordWrap = false;
    int m_tabWidth = 4;

    QString m_text;
    QList<LineLayout*> m_lineLayouts;
    ViewportInfo m_viewport;

    // 私有辅助方法
    void createLineLayout(int lineNumber);
    void updateLineLayout(LineLayout* layout);
    void clearLineLayouts();
    QStringList splitIntoLines(const QString& text) const;
    void updateVisibleLines();

    // 文本处理辅助方法
    QString getLineText(int lineNumber) const;
    int positionToLine(int position) const;
    int positionToColumn(int position, int lineNumber) const;
    int lineColumnToPosition(int lineNumber, int column) const;
    QString expandTabs(const QString& text) const;
};

#endif // LAYOUT_ENGINE_H