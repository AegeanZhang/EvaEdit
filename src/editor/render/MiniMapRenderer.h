#ifndef MINIMAP_RENDERER_H
#define MINIMAP_RENDERER_H

#include <QQuickPaintedItem>
#include <QPainter>
#include <QColor>
#include <QRectF>
#include <QPointF>
#include <QPair>
#include <QStringList>
#include <qqmlintegration.h>

// 包含完整类型定义以支持Q_PROPERTY
#include "DocumentModel.h"
#include "TextRenderer.h"

// 前向声明（仅用于非Q_PROPERTY相关的类型）
class QMouseEvent;
class QWheelEvent;

// 缩略图渲染器
class MiniMapRenderer : public QQuickPaintedItem {
    Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(DocumentModel* document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(TextRenderer* textRenderer READ textRenderer WRITE setTextRenderer NOTIFY textRendererChanged)
        Q_PROPERTY(qreal scale READ scale WRITE setScale NOTIFY scaleChanged)
        Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
        Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
        Q_PROPERTY(QColor viewportColor READ viewportColor WRITE setViewportColor NOTIFY viewportColorChanged)

public:
    explicit MiniMapRenderer(QQuickItem* parent = nullptr);

    // 属性访问器
    DocumentModel* document() const;
    void setDocument(DocumentModel* document);

    TextRenderer* textRenderer() const;
    void setTextRenderer(TextRenderer* renderer);

    qreal scale() const;
    void setScale(qreal scale);

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);

    QColor textColor() const;
    void setTextColor(const QColor& color);

    QColor viewportColor() const;
    void setViewportColor(const QColor& color);

    // 核心方法
    void paint(QPainter* painter) override;

    // QML可调用方法
    Q_INVOKABLE void scrollToPosition(qreal y);
    Q_INVOKABLE void setMiniMapWidth(qreal width);
    Q_INVOKABLE qreal getRecommendedWidth() const;
    Q_INVOKABLE void highlightLine(int lineNumber, const QColor& color);
    Q_INVOKABLE void clearHighlights();
    Q_INVOKABLE void setSyntaxHighlighting(bool enabled);
    Q_INVOKABLE void showSearchResults(const QList<int>& positions);
    Q_INVOKABLE void clearSearchResults();
    Q_INVOKABLE void applyTheme(const QString& themeName);
    Q_INVOKABLE void autoScale();
    Q_INVOKABLE QString getDebugInfo() const;

    // 批量更新
    void beginBatchUpdate();
    void endBatchUpdate();

    // 获取可见范围
    QPair<int, int> getVisibleDocumentRange() const;

signals:
    void documentChanged();
    void textRendererChanged();
    void scaleChanged();
    void backgroundColorChanged();
    void textColorChanged();
    void viewportColorChanged();
    void scrollRequested(qreal y);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onDocumentChanged();
    void onViewportChanged();

private:
    DocumentModel* m_document = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    qreal m_scale = 0.1;
    QColor m_backgroundColor = QColor(240, 240, 240);
    QColor m_textColor = QColor(100, 100, 100);
    QColor m_viewportColor = QColor(0, 120, 215, 100);

    // 私有渲染方法
    void paintMiniText(QPainter* painter);
    void paintLineAsBlocks(QPainter* painter, const QString& lineText,
        const QPointF& position, qreal charWidth, qreal lineHeight);
    void paintViewport(QPainter* painter);

    // 私有辅助方法
    QRectF getViewportRect() const;
    QColor getCharacterColor(QChar ch) const;
    qreal yToDocumentPosition(qreal y) const;
    qreal documentPositionToY(qreal position) const;
};

#endif // MINIMAP_RENDERER_H