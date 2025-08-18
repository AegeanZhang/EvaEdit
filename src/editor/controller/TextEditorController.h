#ifndef TEXT_EDITOR_CONTROLLER_H
#define TEXT_EDITOR_CONTROLLER_H

#include <QObject>
#include <QQuickItem>
#include <qqmlintegration.h>
//#include "../EditorTypes.h"
#include "../core/DocumentModel.h"
#include "../render/TextRenderer.h"

//class TextRenderer;
class InputHandler;
//class DocumentModel;
class InputManager;
class QKeyEvent;
class QInputMethodEvent;

// 声明不透明指针类型
//Q_DECLARE_OPAQUE_POINTER(DocumentModel*)
//Q_DECLARE_OPAQUE_POINTER(TextRenderer*)

// 编辑器控制器 - 协调渲染器、命令处理器和模型之间的交互
class TextEditorController : public QObject {
    Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(TextRenderer* renderer READ renderer WRITE setRenderer NOTIFY rendererChanged)
        Q_PROPERTY(DocumentModel* document READ document WRITE setDocument NOTIFY documentChanged)

public:
    explicit TextEditorController(QObject* parent = nullptr);
    ~TextEditorController();

    // 属性访问器
    TextRenderer* renderer() const;
    void setRenderer(TextRenderer* renderer);

    DocumentModel* document() const;
    void setDocument(DocumentModel* document);

    // 暴露给QML的命令方法
    Q_INVOKABLE void moveCursorLeft();
    Q_INVOKABLE void moveCursorRight();
    Q_INVOKABLE void moveCursorUp();
    Q_INVOKABLE void moveCursorDown();
    Q_INVOKABLE void moveCursorWordLeft();
    Q_INVOKABLE void moveCursorWordRight();
    Q_INVOKABLE void moveCursorLineStart();
    Q_INVOKABLE void moveCursorLineEnd();
    Q_INVOKABLE void moveCursorDocumentStart();
    Q_INVOKABLE void moveCursorDocumentEnd();

    Q_INVOKABLE void selectLeft();
    Q_INVOKABLE void selectRight();
    Q_INVOKABLE void selectUp();
    Q_INVOKABLE void selectDown();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectWord();
    Q_INVOKABLE void selectLine();

    Q_INVOKABLE void insertText(const QString& text);
    Q_INVOKABLE void newLine();
    Q_INVOKABLE void tab();
    Q_INVOKABLE void deleteLeft();
    Q_INVOKABLE void deleteRight();
    Q_INVOKABLE void deleteWordLeft();
    Q_INVOKABLE void deleteWordRight();

    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // 事件处理
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void rendererChanged();
    void documentChanged();

private:
    TextRenderer* m_renderer = nullptr;
    InputHandler* m_inputHandler = nullptr;
    InputManager* m_inputManager = nullptr;
    DocumentModel* m_document = nullptr;

    void setupConnections();
    void setupInputManager();
};

#endif // TEXT_EDITOR_CONTROLLER_H