#ifndef SYNTAX_HIGHLIGHTER_H
#define SYNTAX_HIGHLIGHTER_H

#include <QObject>
#include <QTextCharFormat>
#include <QHash>
#include <QRegularExpression>
#include <QList>
#include <QColor>
#include <QStringList>
#include <QPair>
#include "TokenTypes.h"

struct HighlightRule {
    QRegularExpression pattern;
    TokenType tokenType = TokenType::None;
    QTextCharFormat format;
};

// 语言定义
class LanguageDefinition {
public:
    QString name;
    QStringList fileExtensions;
    QList<HighlightRule> rules;
    QHash<TokenType, QTextCharFormat> defaultFormats;

    // 语言特定的词汇
    QStringList keywords;
    QStringList types;
    QStringList functions;

    // 注释规则
    QString singleLineComment;
    QString multiLineCommentStart;
    QString multiLineCommentEnd;

    // 字符串规则
    QStringList stringDelimiters;
    QString escapeCharacter;
};

// 增量语法高亮器
class SyntaxHighlighter : public QObject {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QObject* parent = nullptr);

    // 语言管理
    void registerLanguage(const LanguageDefinition& language);
    void setLanguage(const QString& languageName);
    void setLanguageByFileExtension(const QString& extension);
    LanguageDefinition currentLanguage() const;
    QStringList availableLanguages() const;

    // 高亮处理
    QList<Token> tokenize(const QString& text) const;
    QList<Token> tokenizeLine(const QString& line, int lineNumber,
        const QList<Token>& previousLineTokens = QList<Token>()) const;

    // 增量更新
    void updateHighlighting(const QString& text, int changedPosition,
        int removedLength, int addedLength);

    // 格式获取
    QTextCharFormat getFormat(TokenType type) const;
    void setFormat(TokenType type, const QTextCharFormat& format);

    // 主题支持
    void applyTheme(const QString& themeName);
    void setCustomTheme(const QHash<TokenType, QTextCharFormat>& theme);

    // 高级功能
    Token getTokenAtPosition(const QString& text, int position) const;
    QPair<int, int> findMatchingBracket(const QString& text, int position) const;
    QList<QPair<int, int>> getCodeFoldingRanges(const QString& text) const;
    QString getDebugInfo() const;

signals:
    void highlightingUpdated(int startLine, int endLine, const QList<Token>& tokens);
    void languageChanged(const QString& languageName);

private:
    LanguageDefinition m_currentLanguage;
    QHash<QString, LanguageDefinition> m_languages;
    QHash<int, QList<Token>> m_lineTokens; // 缓存每行的tokens

    // 私有辅助方法
    void loadBuiltinLanguages();
    bool isInsideMultiLineComment(int position, const QString& text) const;
    bool isInsideString(int position, const QString& text) const;
    TokenType classifyToken(const QString& token) const;
    bool isOperator(const QString& op) const;
};

#endif // SYNTAX_HIGHLIGHTER_H