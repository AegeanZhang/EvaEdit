#include "SyntaxHighlighter.h"
#include <QDebug>
#include <QStringList>
#include <QFileInfo>
#include <algorithm>

SyntaxHighlighter::SyntaxHighlighter(QObject* parent)
    : QObject(parent)
{
    // 加载内置语言定义
    loadBuiltinLanguages();

    // 设置默认语言为纯文本
    if (m_languages.contains("text")) {
        setLanguage("text");
    }
}

// ==============================================================================
// 语言管理
// ==============================================================================

void SyntaxHighlighter::registerLanguage(const LanguageDefinition& language)
{
    m_languages[language.name] = language;
}

void SyntaxHighlighter::setLanguage(const QString& languageName)
{
    if (!m_languages.contains(languageName)) {
        qWarning() << "Language not found:" << languageName;
        return;
    }

    if (m_currentLanguage.name == languageName)
        return;

    m_currentLanguage = m_languages[languageName];

    // 清除缓存的tokens
    m_lineTokens.clear();

    emit languageChanged(languageName);
}

void SyntaxHighlighter::setLanguageByFileExtension(const QString& extension)
{
    QString lowercaseExt = extension.toLower();

    // 查找匹配的语言
    for (auto it = m_languages.begin(); it != m_languages.end(); ++it) {
        const LanguageDefinition& lang = it.value();
        if (lang.fileExtensions.contains(lowercaseExt)) {
            setLanguage(lang.name);
            return;
        }
    }

    // 如果没有找到匹配的语言，设置为纯文本
    setLanguage("text");
}

LanguageDefinition SyntaxHighlighter::currentLanguage() const
{
    return m_currentLanguage;
}

QStringList SyntaxHighlighter::availableLanguages() const
{
    return m_languages.keys();
}

// ==============================================================================
// 高亮处理
// ==============================================================================

QList<Token> SyntaxHighlighter::tokenize(const QString& text) const
{
    QList<Token> tokens;

    if (text.isEmpty() || m_currentLanguage.name.isEmpty())
        return tokens;

    // 分行处理
    QStringList lines = text.split('\n');
    int currentPosition = 0;

    for (int lineNumber = 0; lineNumber < lines.size(); ++lineNumber) {
        const QString& line = lines[lineNumber];
        QList<Token> lineTokens = tokenizeLine(line, lineNumber);

        // 调整token位置到全文坐标
        for (Token& token : lineTokens) {
            token.position += currentPosition;
            tokens.append(token);
        }

        currentPosition += line.length() + 1; // +1 for newline
    }

    return tokens;
}

QList<Token> SyntaxHighlighter::tokenizeLine(const QString& line, int lineNumber,
    const QList<Token>& previousLineTokens) const
{
    QList<Token> tokens;

    if (line.isEmpty())
        return tokens;

    // 检查是否在多行注释中
    bool inMultiLineComment = false;
    bool inString = false;
    QChar stringDelimiter;

    // 从上一行的状态继续
    if (!previousLineTokens.isEmpty()) {
        const Token& lastToken = previousLineTokens.last();
        if (lastToken.type == TokenType::Comment) {
            inMultiLineComment = true;
        }
        else if (lastToken.type == TokenType::String) {
            inString = true;
            // 需要确定字符串分隔符，这里简化处理
            stringDelimiter = '"';
        }
    }

    int i = 0;
    while (i < line.length()) {
        QChar ch = line.at(i);

        // 处理多行注释
        if (!inString && !m_currentLanguage.multiLineCommentStart.isEmpty()) {
            if (!inMultiLineComment &&
                line.mid(i).startsWith(m_currentLanguage.multiLineCommentStart)) {
                inMultiLineComment = true;
                int commentStart = i;
                i += m_currentLanguage.multiLineCommentStart.length();

                // 查找注释结束
                int commentEnd = line.indexOf(m_currentLanguage.multiLineCommentEnd, i);
                if (commentEnd != -1) {
                    // 注释在同一行结束
                    Token token;
                    token.position = commentStart;
                    token.length = commentEnd + m_currentLanguage.multiLineCommentEnd.length() - commentStart;
                    token.type = TokenType::Comment;
                    tokens.append(token);

                    i = commentEnd + m_currentLanguage.multiLineCommentEnd.length();
                    inMultiLineComment = false;
                }
                else {
                    // 注释延续到行尾
                    Token token;
                    token.position = commentStart;
                    token.length = line.length() - commentStart;
                    token.type = TokenType::Comment;
                    tokens.append(token);
                    break;
                }
                continue;
            }
            else if (inMultiLineComment) {
                int commentEnd = line.indexOf(m_currentLanguage.multiLineCommentEnd, i);
                if (commentEnd != -1) {
                    // 注释结束
                    Token token;
                    token.position = 0;
                    token.length = commentEnd + m_currentLanguage.multiLineCommentEnd.length();
                    token.type = TokenType::Comment;
                    tokens.append(token);

                    i = commentEnd + m_currentLanguage.multiLineCommentEnd.length();
                    inMultiLineComment = false;
                }
                else {
                    // 整行都是注释
                    Token token;
                    token.position = 0;
                    token.length = line.length();
                    token.type = TokenType::Comment;
                    tokens.append(token);
                    break;
                }
                continue;
            }
        }

        // 处理单行注释
        if (!inString && !inMultiLineComment &&
            !m_currentLanguage.singleLineComment.isEmpty() &&
            line.mid(i).startsWith(m_currentLanguage.singleLineComment)) {
            Token token;
            token.position = i;
            token.length = line.length() - i;
            token.type = TokenType::Comment;
            tokens.append(token);
            break;
        }

        // 处理字符串
        if (!inMultiLineComment) {
            for (const QString& delimiter : m_currentLanguage.stringDelimiters) {
                if (delimiter.length() == 1 && ch == delimiter.at(0)) {
                    if (!inString) {
                        // 字符串开始
                        inString = true;
                        stringDelimiter = ch;
                        int stringStart = i;
                        i++;

                        // 查找字符串结束
                        bool escaped = false;
                        while (i < line.length()) {
                            QChar currentChar = line.at(i);

                            if (escaped) {
                                escaped = false;
                            }
                            else if (!m_currentLanguage.escapeCharacter.isEmpty() &&
                                currentChar == m_currentLanguage.escapeCharacter.at(0)) {
                                escaped = true;
                            }
                            else if (currentChar == stringDelimiter) {
                                // 字符串结束
                                Token token;
                                token.position = stringStart;
                                token.length = i - stringStart + 1;
                                token.type = TokenType::String;
                                tokens.append(token);

                                inString = false;
                                i++;
                                goto next_char;
                            }
                            i++;
                        }

                        // 字符串延续到行尾
                        if (inString) {
                            Token token;
                            token.position = stringStart;
                            token.length = line.length() - stringStart;
                            token.type = TokenType::String;
                            tokens.append(token);
                            goto line_end;
                        }
                    }
                    else if (ch == stringDelimiter) {
                        // 字符串结束（这种情况在上面已经处理了）
                        inString = false;
                        i++;
                        continue;
                    }
                }
            }
        }

        // 处理数字
        if (!inString && !inMultiLineComment && ch.isDigit()) {
            int numberStart = i;
            i++;

            // 继续读取数字
            while (i < line.length()) {
                QChar nextChar = line.at(i);
                if (nextChar.isDigit() || nextChar == '.' ||
                    nextChar.toLower() == 'x' || nextChar.toLower() == 'e' ||
                    (nextChar >= 'a' && nextChar <= 'f') || (nextChar >= 'A' && nextChar <= 'F')) {
                    i++;
                }
                else {
                    break;
                }
            }

            Token token;
            token.position = numberStart;
            token.length = i - numberStart;
            token.type = TokenType::Number;
            tokens.append(token);
            continue;
        }

        // 处理标识符和关键字
        if (!inString && !inMultiLineComment && (ch.isLetter() || ch == '_')) {
            int identifierStart = i;
            i++;

            // 继续读取标识符
            while (i < line.length()) {
                QChar nextChar = line.at(i);
                if (nextChar.isLetterOrNumber() || nextChar == '_') {
                    i++;
                }
                else {
                    break;
                }
            }

            QString identifier = line.mid(identifierStart, i - identifierStart);
            TokenType tokenType = classifyToken(identifier);

            Token token;
            token.position = identifierStart;
            token.length = i - identifierStart;
            token.type = tokenType;
            tokens.append(token);
            continue;
        }

        // 处理运算符
        if (!inString && !inMultiLineComment && !ch.isLetterOrNumber() && !ch.isSpace()) {
            // 检查多字符运算符
            bool foundOperator = false;
            for (int len = 3; len >= 1; len--) {
                if (i + len <= line.length()) {
                    QString op = line.mid(i, len);
                    if (isOperator(op)) {
                        Token token;
                        token.position = i;
                        token.length = len;
                        token.type = TokenType::Operator;
                        tokens.append(token);

                        i += len;
                        foundOperator = true;
                        break;
                    }
                }
            }

            if (!foundOperator) {
                i++;
            }
            continue;
        }

    next_char:
        if (i < line.length()) {
            i++;
        }
    }

line_end:
    Q_UNUSED(lineNumber) // 避免未使用参数警告
        return tokens;
}

// ==============================================================================
// 增量更新
// ==============================================================================

void SyntaxHighlighter::updateHighlighting(const QString& text, int changedPosition,
    int removedLength, int addedLength)
{
    Q_UNUSED(text)
        Q_UNUSED(changedPosition)
        Q_UNUSED(removedLength)
        Q_UNUSED(addedLength)

        // 简化的增量更新：清除所有缓存
        // 在实际实现中，可以只更新受影响的行
        m_lineTokens.clear();

    // 发送更新信号
    emit highlightingUpdated(0, -1, QList<Token>());
}

// ==============================================================================
// 格式获取
// ==============================================================================

QTextCharFormat SyntaxHighlighter::getFormat(TokenType type) const
{
    if (m_currentLanguage.defaultFormats.contains(type)) {
        return m_currentLanguage.defaultFormats[type];
    }

    // 返回默认格式
    QTextCharFormat format;

    switch (type) {
    case TokenType::Keyword:
        format.setForeground(QColor(86, 156, 214)); // VS Code 蓝色
        format.setFontWeight(QFont::Bold);
        break;
    case TokenType::String:
        format.setForeground(QColor(206, 145, 120)); // VS Code 橙色
        break;
    case TokenType::Comment:
        format.setForeground(QColor(106, 153, 85)); // VS Code 绿色
        format.setFontItalic(true);
        break;
    case TokenType::Number:
        format.setForeground(QColor(181, 206, 168)); // VS Code 浅绿色
        break;
    case TokenType::Operator:
        format.setForeground(QColor(212, 212, 212)); // VS Code 浅灰色
        break;
    case TokenType::Function:
        format.setForeground(QColor(220, 220, 170)); // VS Code 黄色
        break;
    case TokenType::Type:
        format.setForeground(QColor(78, 201, 176)); // VS Code 青色
        break;
    case TokenType::Preprocessor:
        format.setForeground(QColor(155, 155, 155)); // VS Code 灰色
        break;
    default:
        format.setForeground(QColor(212, 212, 212)); // 默认浅灰色
        break;
    }

    return format;
}

void SyntaxHighlighter::setFormat(TokenType type, const QTextCharFormat& format)
{
    m_currentLanguage.defaultFormats[type] = format;
}

// ==============================================================================
// 主题支持
// ==============================================================================

void SyntaxHighlighter::applyTheme(const QString& themeName)
{
    QHash<TokenType, QTextCharFormat> theme;

    if (themeName == "dark") {
        // 暗色主题
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(86, 156, 214));
        keywordFormat.setFontWeight(QFont::Bold);
        theme[TokenType::Keyword] = keywordFormat;

        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(206, 145, 120));
        theme[TokenType::String] = stringFormat;

        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(106, 153, 85));
        commentFormat.setFontItalic(true);
        theme[TokenType::Comment] = commentFormat;

        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(181, 206, 168));
        theme[TokenType::Number] = numberFormat;

        QTextCharFormat operatorFormat;
        operatorFormat.setForeground(QColor(212, 212, 212));
        theme[TokenType::Operator] = operatorFormat;

        QTextCharFormat functionFormat;
        functionFormat.setForeground(QColor(220, 220, 170));
        theme[TokenType::Function] = functionFormat;

        QTextCharFormat typeFormat;
        typeFormat.setForeground(QColor(78, 201, 176));
        theme[TokenType::Type] = typeFormat;

        QTextCharFormat preprocessorFormat;
        preprocessorFormat.setForeground(QColor(155, 155, 155));
        theme[TokenType::Preprocessor] = preprocessorFormat;

    }
    else if (themeName == "light") {
        // 亮色主题
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(0, 0, 255));
        keywordFormat.setFontWeight(QFont::Bold);
        theme[TokenType::Keyword] = keywordFormat;

        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(163, 21, 21));
        theme[TokenType::String] = stringFormat;

        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(0, 128, 0));
        commentFormat.setFontItalic(true);
        theme[TokenType::Comment] = commentFormat;

        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(9, 134, 88));
        theme[TokenType::Number] = numberFormat;

        QTextCharFormat operatorFormat;
        operatorFormat.setForeground(QColor(0, 0, 0));
        theme[TokenType::Operator] = operatorFormat;

        QTextCharFormat functionFormat;
        functionFormat.setForeground(QColor(121, 94, 38));
        theme[TokenType::Function] = functionFormat;

        QTextCharFormat typeFormat;
        typeFormat.setForeground(QColor(43, 145, 175));
        theme[TokenType::Type] = typeFormat;

        QTextCharFormat preprocessorFormat;
        preprocessorFormat.setForeground(QColor(128, 128, 128));
        theme[TokenType::Preprocessor] = preprocessorFormat;
    }

    setCustomTheme(theme);
}

void SyntaxHighlighter::setCustomTheme(const QHash<TokenType, QTextCharFormat>& theme)
{
    for (auto it = theme.begin(); it != theme.end(); ++it) {
        m_currentLanguage.defaultFormats[it.key()] = it.value();
    }

    // 清除缓存，强制重新高亮
    m_lineTokens.clear();
}

// ==============================================================================
// 高级功能
// ==============================================================================

Token SyntaxHighlighter::getTokenAtPosition(const QString& text, int position) const
{
    QList<Token> tokens = tokenize(text);

    for (const Token& token : tokens) {
        if (position >= token.position && position < token.position + token.length) {
            return token;
        }
    }

    // 返回空token
    Token emptyToken;
    emptyToken.position = position;
    emptyToken.length = 0;
    emptyToken.type = TokenType::None;
    return emptyToken;
}

QPair<int, int> SyntaxHighlighter::findMatchingBracket(const QString& text, int position) const
{
    if (position < 0 || position >= text.length())
        return QPair<int, int>(-1, -1);

    QChar ch = text.at(position);
    QChar openBracket, closeBracket;
    bool searchForward = true;

    // 确定括号类型和搜索方向
    if (ch == '(') {
        openBracket = '(';
        closeBracket = ')';
    }
    else if (ch == ')') {
        openBracket = '(';
        closeBracket = ')';
        searchForward = false;
    }
    else if (ch == '[') {
        openBracket = '[';
        closeBracket = ']';
    }
    else if (ch == ']') {
        openBracket = '[';
        closeBracket = ']';
        searchForward = false;
    }
    else if (ch == '{') {
        openBracket = '{';
        closeBracket = '}';
    }
    else if (ch == '}') {
        openBracket = '{';
        closeBracket = '}';
        searchForward = false;
    }
    else {
        return QPair<int, int>(-1, -1);
    }

    // 搜索匹配的括号
    int depth = 1;
    int searchPos = searchForward ? position + 1 : position - 1;

    while (searchPos >= 0 && searchPos < text.length()) {
        QChar currentChar = text.at(searchPos);

        // 跳过字符串和注释中的括号
        if (isInsideString(searchPos, text) || isInsideMultiLineComment(searchPos, text)) {
            searchPos += searchForward ? 1 : -1;
            continue;
        }

        if (currentChar == openBracket) {
            if (searchForward) {
                depth++;
            }
            else {
                depth--;
            }
        }
        else if (currentChar == closeBracket) {
            if (searchForward) {
                depth--;
            }
            else {
                depth++;
            }
        }

        if (depth == 0) {
            return QPair<int, int>(position, searchPos);
        }

        searchPos += searchForward ? 1 : -1;
    }

    return QPair<int, int>(-1, -1);
}

QList<QPair<int, int>> SyntaxHighlighter::getCodeFoldingRanges(const QString& text) const
{
    QList<QPair<int, int>> ranges;

    if (m_currentLanguage.name == "cpp" || m_currentLanguage.name == "javascript") {
        // 基于大括号的代码折叠
        int braceDepth = 0;
        int foldStart = -1;

        for (int i = 0; i < text.length(); ++i) {
            QChar ch = text.at(i);

            if (ch == '{') {
                if (braceDepth == 0) {
                    foldStart = i;
                }
                braceDepth++;
            }
            else if (ch == '}') {
                braceDepth--;
                if (braceDepth == 0 && foldStart != -1) {
                    ranges.append(QPair<int, int>(foldStart, i));
                    foldStart = -1;
                }
            }
        }
    }
    else if (m_currentLanguage.name == "python") {
        // 基于缩进的代码折叠
        QStringList lines = text.split('\n');
        QList<int> indentStack;

        for (int i = 0; i < lines.size(); ++i) {
            const QString& line = lines[i];
            if (line.trimmed().isEmpty())
                continue;

            int indent = 0;
            for (QChar ch : line) {
                if (ch == ' ') {
                    indent++;
                }
                else if (ch == '\t') {
                    indent += 4; // 假设tab等于4个空格
                }
                else {
                    break;
                }
            }

            // 简化的缩进折叠逻辑
            // 在实际实现中需要更复杂的处理
            Q_UNUSED(indent)
                Q_UNUSED(indentStack)
        }
    }

    return ranges;
}

QString SyntaxHighlighter::getDebugInfo() const
{
    QStringList info;

    info << "SyntaxHighlighter Debug Info:";
    info << QString("  Current language: %1").arg(m_currentLanguage.name);
    info << QString("  Available languages: %1").arg(availableLanguages().join(", "));
    info << QString("  Keywords count: %1").arg(m_currentLanguage.keywords.size());
    info << QString("  Types count: %1").arg(m_currentLanguage.types.size());
    info << QString("  Functions count: %1").arg(m_currentLanguage.functions.size());
    info << QString("  Single line comment: '%1'").arg(m_currentLanguage.singleLineComment);
    info << QString("  Multi line comment: '%1' ... '%2'")
        .arg(m_currentLanguage.multiLineCommentStart)
        .arg(m_currentLanguage.multiLineCommentEnd);
    info << QString("  String delimiters: %1").arg(m_currentLanguage.stringDelimiters.join(", "));
    info << QString("  Cached line tokens: %1").arg(m_lineTokens.size());

    return info.join("\n");
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

void SyntaxHighlighter::loadBuiltinLanguages()
{
    // C++ 语言定义
    LanguageDefinition cpp;
    cpp.name = "cpp";
    cpp.fileExtensions = QStringList{ "cpp", "cxx", "cc", "c", "h", "hpp", "hxx" };
    cpp.keywords = QStringList{
        "auto", "break", "case", "catch", "class", "const", "continue", "default",
        "delete", "do", "else", "enum", "explicit", "extern", "false", "for",
        "friend", "goto", "if", "inline", "namespace", "new", "nullptr", "operator",
        "private", "protected", "public", "return", "sizeof", "static", "struct",
        "switch", "template", "this", "throw", "true", "try", "typedef", "typename",
        "union", "using", "virtual", "void", "volatile", "while"
    };
    cpp.types = QStringList{
        "bool", "char", "double", "float", "int", "long", "short", "signed",
        "unsigned", "wchar_t", "char16_t", "char32_t", "size_t", "ptrdiff_t",
        "string", "vector", "map", "set", "list", "queue", "stack", "pair"
    };
    cpp.functions = QStringList{
        "printf", "scanf", "malloc", "free", "sizeof", "strlen", "strcpy", "strcmp",
        "std", "cout", "cin", "endl", "cerr", "clog"
    };
    cpp.singleLineComment = "//";
    cpp.multiLineCommentStart = "/*";
    cpp.multiLineCommentEnd = "*/";
    cpp.stringDelimiters = QStringList{ "\"", "'" };
    cpp.escapeCharacter = "\\";
    registerLanguage(cpp);

    // JavaScript 语言定义
    LanguageDefinition js;
    js.name = "javascript";
    js.fileExtensions = QStringList{ "js", "jsx", "ts", "tsx" };
    js.keywords = QStringList{
        "abstract", "arguments", "boolean", "break", "byte", "case", "catch", "char",
        "class", "const", "continue", "debugger", "default", "delete", "do", "double",
        "else", "enum", "eval", "export", "extends", "false", "final", "finally",
        "float", "for", "function", "goto", "if", "implements", "import", "in",
        "instanceof", "int", "interface", "let", "long", "native", "new", "null",
        "package", "private", "protected", "public", "return", "short", "static",
        "super", "switch", "synchronized", "this", "throw", "throws", "transient",
        "true", "try", "typeof", "var", "void", "volatile", "while", "with", "yield"
    };
    js.types = QStringList{
        "Array", "Boolean", "Date", "Error", "Function", "Number", "Object",
        "RegExp", "String", "undefined", "null"
    };
    js.functions = QStringList{
        "console", "log", "alert", "confirm", "prompt", "setTimeout", "setInterval",
        "parseInt", "parseFloat", "isNaN", "isFinite"
    };
    js.singleLineComment = "//";
    js.multiLineCommentStart = "/*";
    js.multiLineCommentEnd = "*/";
    js.stringDelimiters = QStringList{ "\"", "'", "`" };
    js.escapeCharacter = "\\";
    registerLanguage(js);

    // Python 语言定义
    LanguageDefinition python;
    python.name = "python";
    python.fileExtensions = QStringList{ "py", "pyw", "pyx" };
    python.keywords = QStringList{
        "False", "None", "True", "and", "as", "assert", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "finally", "for", "from", "global",
        "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
        "raise", "return", "try", "while", "with", "yield"
    };
    python.types = QStringList{
        "int", "float", "str", "bool", "list", "tuple", "dict", "set", "frozenset",
        "bytes", "bytearray", "memoryview", "complex"
    };
    python.functions = QStringList{
        "print", "input", "len", "range", "enumerate", "zip", "map", "filter",
        "sorted", "reversed", "sum", "min", "max", "abs", "round", "type", "isinstance"
    };
    python.singleLineComment = "#";
    python.multiLineCommentStart = "\"\"\"";
    python.multiLineCommentEnd = "\"\"\"";
    python.stringDelimiters = QStringList{ "\"", "'", "\"\"\"", "'''" };
    python.escapeCharacter = "\\";
    registerLanguage(python);

    // HTML 语言定义
    LanguageDefinition html;
    html.name = "html";
    html.fileExtensions = QStringList{ "html", "htm", "xhtml" };
    html.keywords = QStringList{
        "a", "abbr", "address", "area", "article", "aside", "audio", "b", "base",
        "bdi", "bdo", "blockquote", "body", "br", "button", "canvas", "caption",
        "cite", "code", "col", "colgroup", "data", "datalist", "dd", "del", "details",
        "dfn", "dialog", "div", "dl", "dt", "em", "embed", "fieldset", "figcaption",
        "figure", "footer", "form", "h1", "h2", "h3", "h4", "h5", "h6", "head",
        "header", "hr", "html", "i", "iframe", "img", "input", "ins", "kbd", "label",
        "legend", "li", "link", "main", "map", "mark", "meta", "meter", "nav",
        "noscript", "object", "ol", "optgroup", "option", "output", "p", "param",
        "picture", "pre", "progress", "q", "rp", "rt", "ruby", "s", "samp", "script",
        "section", "select", "small", "source", "span", "strong", "style", "sub",
        "summary", "sup", "table", "tbody", "td", "template", "textarea", "tfoot",
        "th", "thead", "time", "title", "tr", "track", "u", "ul", "var", "video", "wbr"
    };
    html.singleLineComment = "";
    html.multiLineCommentStart = "<!--";
    html.multiLineCommentEnd = "-->";
    html.stringDelimiters = QStringList{ "\"", "'" };
    html.escapeCharacter = "&";
    registerLanguage(html);

    // CSS 语言定义
    LanguageDefinition css;
    css.name = "css";
    css.fileExtensions = QStringList{ "css", "scss", "sass", "less" };
    css.keywords = QStringList{
        "color", "background", "border", "margin", "padding", "width", "height",
        "font", "text", "display", "position", "float", "clear", "overflow",
        "visibility", "z-index", "top", "bottom", "left", "right", "line-height",
        "letter-spacing", "word-spacing", "text-align", "text-decoration",
        "text-transform", "white-space", "vertical-align", "list-style"
    };
    css.singleLineComment = "//";
    css.multiLineCommentStart = "/*";
    css.multiLineCommentEnd = "*/";
    css.stringDelimiters = QStringList{ "\"", "'" };
    css.escapeCharacter = "\\";
    registerLanguage(css);

    // JSON 语言定义
    LanguageDefinition json;
    json.name = "json";
    json.fileExtensions = QStringList{ "json" };
    json.keywords = QStringList{ "true", "false", "null" };
    json.singleLineComment = "";
    json.multiLineCommentStart = "";
    json.multiLineCommentEnd = "";
    json.stringDelimiters = QStringList{ "\"" };
    json.escapeCharacter = "\\";
    registerLanguage(json);

    // Markdown 语言定义
    LanguageDefinition markdown;
    markdown.name = "markdown";
    markdown.fileExtensions = QStringList{ "md", "markdown", "mdown", "mkd" };
    markdown.keywords = QStringList{}; // Markdown 没有传统意义的关键字
    markdown.singleLineComment = "";
    markdown.multiLineCommentStart = "";
    markdown.multiLineCommentEnd = "";
    markdown.stringDelimiters = QStringList{ "`", "```" };
    markdown.escapeCharacter = "\\";
    registerLanguage(markdown);

    // 纯文本语言定义
    LanguageDefinition text;
    text.name = "text";
    text.fileExtensions = QStringList{ "txt", "text", "log" };
    text.keywords = QStringList{};
    text.singleLineComment = "";
    text.multiLineCommentStart = "";
    text.multiLineCommentEnd = "";
    text.stringDelimiters = QStringList{};
    text.escapeCharacter = "";
    registerLanguage(text);
}

bool SyntaxHighlighter::isInsideMultiLineComment(int position, const QString& text) const
{
    Q_UNUSED(position)
        Q_UNUSED(text)
        // 简化实现，在实际应用中需要更复杂的状态跟踪
        return false;
}

bool SyntaxHighlighter::isInsideString(int position, const QString& text) const
{
    Q_UNUSED(position)
        Q_UNUSED(text)
        // 简化实现，在实际应用中需要更复杂的状态跟踪
        return false;
}

TokenType SyntaxHighlighter::classifyToken(const QString& token) const
{
    if (m_currentLanguage.keywords.contains(token)) {
        return TokenType::Keyword;
    }

    if (m_currentLanguage.types.contains(token)) {
        return TokenType::Type;
    }

    if (m_currentLanguage.functions.contains(token)) {
        return TokenType::Function;
    }

    // 检查是否为预处理指令
    if (token.startsWith('#')) {
        return TokenType::Preprocessor;
    }

    return TokenType::Identifier;
}

bool SyntaxHighlighter::isOperator(const QString& op) const
{
    static const QStringList operators = {
        // 三字符运算符
        "<<<", ">>>", "<<=", ">>=", "...",

        // 双字符运算符
        "++", "--", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>",
        "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "::",

        // 单字符运算符
        "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^",
        "~", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}"
    };

    return operators.contains(op);
}