#include "SearchService.h"
#include <QRegularExpression>
#include <QStringList>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>

SearchService::SearchService(QObject* parent)
    : QObject(parent)
{
}

// ==============================================================================
// 基础搜索方法
// ==============================================================================

QList<SearchResult> SearchService::findAll(const QString& text, const QString& pattern,
    const SearchOptions& options) const
{
    QList<SearchResult> results;

    if (pattern.isEmpty() || text.isEmpty())
        return results;

    QElapsedTimer timer;
    timer.start();

    if (options.useRegex) {
        // 正则表达式搜索
        QRegularExpression regex = createRegex(pattern, options);
        if (!regex.isValid()) {
            qWarning() << "Invalid regex pattern:" << pattern;
            return results;
        }

        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            SearchResult result = createSearchResult(text, match.capturedStart(),
                match.capturedLength(), options);
            results.append(result);

            // 每100个结果发送一次进度信号
            if (results.size() % 100 == 0) {
                int progress = static_cast<int>((match.capturedStart() * 100.0) / text.length());
                // 移除const_cast，改为通过其他方式处理进度通知
                QMetaObject::invokeMethod(const_cast<SearchService*>(this),
                    "searchProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            }
        }
    }
    else {
        // 普通字符串搜索
        QString searchText = text;
        QString searchPattern = pattern;

        if (!options.caseSensitive) {
            searchText = text.toLower();
            searchPattern = pattern.toLower();
        }

        int startPos = 0;
        while (true) {
            int foundPos = searchText.indexOf(searchPattern, startPos);
            if (foundPos == -1)
                break;

            // 检查整词匹配
            if (options.wholeWords && !isWholeWordMatch(text, foundPos, pattern.length())) {
                startPos = foundPos + 1;
                continue;
            }

            SearchResult result = createSearchResult(text, foundPos, pattern.length(), options);
            results.append(result);

            startPos = foundPos + pattern.length();

            // 发送进度信号
            if (results.size() % 100 == 0) {
                int progress = static_cast<int>((foundPos * 100.0) / text.length());
                QMetaObject::invokeMethod(const_cast<SearchService*>(this),
                    "searchProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            }
        }
    }

    qDebug() << "Search completed in" << timer.elapsed() << "ms, found" << results.size() << "results";

    QMetaObject::invokeMethod(const_cast<SearchService*>(this),
        "searchProgress", Qt::QueuedConnection, Q_ARG(int, 100));
    QMetaObject::invokeMethod(const_cast<SearchService*>(this),
        "searchCompleted", Qt::QueuedConnection, Q_ARG(QList<SearchResult>, results));

    return results;
}

SearchResult SearchService::findNext(const QString& text, const QString& pattern,
    int startPosition, const SearchOptions& options) const
{
    SearchResult result;
    result.position = -1;

    if (pattern.isEmpty() || text.isEmpty() || startPosition >= text.length())
        return result;

    if (options.useRegex) {
        // 正则表达式搜索
        QRegularExpression regex = createRegex(pattern, options);
        if (!regex.isValid())
            return result;

        QRegularExpressionMatch match = regex.match(text, startPosition);
        if (match.hasMatch()) {
            result = createSearchResult(text, match.capturedStart(),
                match.capturedLength(), options);
        }
    }
    else {
        // 普通字符串搜索
        QString searchText = text;
        QString searchPattern = pattern;

        if (!options.caseSensitive) {
            searchText = text.toLower();
            searchPattern = pattern.toLower();
        }

        int foundPos = searchText.indexOf(searchPattern, startPosition);

        // 检查整词匹配
        while (foundPos != -1 && options.wholeWords &&
            !isWholeWordMatch(text, foundPos, pattern.length())) {
            foundPos = searchText.indexOf(searchPattern, foundPos + 1);
        }

        if (foundPos != -1) {
            result = createSearchResult(text, foundPos, pattern.length(), options);
        }
    }

    return result;
}

SearchResult SearchService::findPrevious(const QString& text, const QString& pattern,
    int startPosition, const SearchOptions& options) const
{
    SearchResult result;
    result.position = -1;

    if (pattern.isEmpty() || text.isEmpty() || startPosition <= 0)
        return result;

    if (options.useRegex) {
        // 正则表达式向后搜索
        QRegularExpression regex = createRegex(pattern, options);
        if (!regex.isValid())
            return result;

        // 从开始到当前位置搜索所有匹配，取最后一个
        QString searchText = text.left(startPosition);
        QRegularExpressionMatchIterator iterator = regex.globalMatch(searchText);

        QRegularExpressionMatch lastMatch;
        while (iterator.hasNext()) {
            lastMatch = iterator.next();
        }

        if (lastMatch.hasMatch()) {
            result = createSearchResult(text, lastMatch.capturedStart(),
                lastMatch.capturedLength(), options);
        }
    }
    else {
        // 普通字符串向后搜索
        QString searchText = text;
        QString searchPattern = pattern;

        if (!options.caseSensitive) {
            searchText = text.toLower();
            searchPattern = pattern.toLower();
        }

        int foundPos = searchText.lastIndexOf(searchPattern, startPosition - pattern.length());

        // 检查整词匹配
        while (foundPos != -1 && options.wholeWords &&
            !isWholeWordMatch(text, foundPos, pattern.length())) {
            foundPos = searchText.lastIndexOf(searchPattern, foundPos - 1);
        }

        if (foundPos != -1) {
            result = createSearchResult(text, foundPos, pattern.length(), options);
        }
    }

    return result;
}

// ==============================================================================
// 异步搜索
// ==============================================================================

QFuture<QList<SearchResult>> SearchService::findAllAsync(const QString& text, const QString& pattern,
    const SearchOptions& options) const
{
    return QtConcurrent::run([this, text, pattern, options]() {
        return findAll(text, pattern, options);
        });
}

// ==============================================================================
// 替换方法
// ==============================================================================

QString SearchService::replaceAll(const QString& text, const QString& pattern,
    const QString& replacement, const SearchOptions& options) const
{
    if (pattern.isEmpty())
        return text;

    QString result = text;

    if (options.useRegex) {
        // 正则表达式替换
        QRegularExpression regex = createRegex(pattern, options);
        if (!regex.isValid()) {
            qWarning() << "Invalid regex pattern for replacement:" << pattern;
            return text;
        }

        result = text;
        result.replace(regex, replacement);
    }
    else {
        // 普通字符串替换
        if (options.wholeWords) {
            // 整词替换需要特殊处理
            QList<SearchResult> matches = findAll(text, pattern, options);

            // 从后往前替换，避免位置偏移
            for (int i = matches.size() - 1; i >= 0; --i) {
                const SearchResult& match = matches[i];
                result.replace(match.position, match.length, replacement);
            }
        }
        else {
            // 简单替换
            Qt::CaseSensitivity caseSensitivity = options.caseSensitive ?
                Qt::CaseSensitive : Qt::CaseInsensitive;
            result.replace(pattern, replacement, caseSensitivity);
        }
    }

    return result;
}

QString SearchService::replaceFirst(const QString& text, const QString& pattern,
    const QString& replacement, int startPosition,
    const SearchOptions& options) const
{
    SearchResult match = findNext(text, pattern, startPosition, options);

    if (match.position == -1)
        return text;

    QString result = text;
    result.replace(match.position, match.length, replacement);

    return result;
}

// ==============================================================================
// 增量搜索
// ==============================================================================

void SearchService::startIncrementalSearch(const QString& text, const SearchOptions& options)
{
    m_incrementalText = text;
    m_incrementalOptions = options;
    m_incrementalActive = true;
}

QList<SearchResult> SearchService::updateIncrementalSearch(const QString& pattern)
{
    QList<SearchResult> results;

    if (!m_incrementalActive || pattern.isEmpty())
        return results;

    // 增量搜索通常限制结果数量以提高性能
    results = findAll(m_incrementalText, pattern, m_incrementalOptions);

    // 限制结果数量
    if (results.size() > 1000) {
        results = results.mid(0, 1000);
    }

    emit incrementalSearchUpdated(results);
    return results;
}

void SearchService::stopIncrementalSearch()
{
    m_incrementalActive = false;
    m_incrementalText.clear();
}

// ==============================================================================
// 高级搜索功能
// ==============================================================================

QList<SearchResult> SearchService::fuzzySearch(const QString& text, const QString& pattern,
    int maxDistance, const SearchOptions& options) const
{
    QList<SearchResult> results;

    if (pattern.isEmpty() || text.isEmpty())
        return results;

    QStringList words = text.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);

    for (int i = 0; i < words.size(); ++i) {
        QString word = words[i];
        if (!options.caseSensitive) {
            word = word.toLower();
        }

        QString searchPattern = pattern;
        if (!options.caseSensitive) {
            searchPattern = searchPattern.toLower();
        }

        int distance = calculateEditDistance(word, searchPattern);
        if (distance <= maxDistance) {
            // 找到模糊匹配，需要在原文中定位
            int wordPos = findWordPosition(text, words[i], i);
            if (wordPos != -1) {
                SearchResult result = createSearchResult(text, wordPos, words[i].length(), options);
                results.append(result);
            }
        }
    }

    return results;
}

// ==============================================================================
// 搜索统计
// ==============================================================================

SearchStatistics SearchService::getSearchStatistics(const QString& text, const QString& pattern,
    const SearchOptions& options) const
{
    SearchStatistics stats;

    QElapsedTimer timer;
    timer.start();

    QList<SearchResult> results = findAll(text, pattern, options);

    stats.totalMatches = results.size();
    stats.searchTime = timer.elapsed();

    // 统计每行的匹配数
    for (const SearchResult& result : results) {
        stats.matchesPerLine[result.line]++;
    }

    // 计算匹配密度
    if (!text.isEmpty()) {
        stats.matchDensity = static_cast<double>(stats.totalMatches) / text.length() * 1000;
    }

    return stats;
}

// ==============================================================================
// 搜索历史管理
// ==============================================================================

void SearchService::addToHistory(const QString& pattern, const SearchOptions& options)
{
    SearchHistoryItem item;
    item.pattern = pattern;
    item.options = options;
    item.timestamp = QDateTime::currentDateTime();

    // 移除重复项
    m_searchHistory.removeAll(item);

    // 添加到开头
    m_searchHistory.prepend(item);

    // 限制历史数量
    if (m_searchHistory.size() > 100) {
        m_searchHistory.removeLast();
    }
}

QStringList SearchService::getSearchHistory() const
{
    QStringList history;
    for (const SearchHistoryItem& item : m_searchHistory) {
        history.append(item.pattern);
    }
    return history;
}

void SearchService::clearSearchHistory()
{
    m_searchHistory.clear();
}

// ==============================================================================
// 搜索预设
// ==============================================================================

void SearchService::saveSearchPreset(const QString& name, const QString& pattern,
    const SearchOptions& options)
{
    SearchPreset preset;
    preset.name = name;
    preset.pattern = pattern;
    preset.options = options;

    m_searchPresets[name] = preset;
}

SearchOptions SearchService::loadSearchPreset(const QString& name) const
{
    if (m_searchPresets.contains(name)) {
        return m_searchPresets[name].options;
    }
    return SearchOptions();
}

QStringList SearchService::getSearchPresets() const
{
    return m_searchPresets.keys();
}

// ==============================================================================
// 性能优化：搜索索引
// ==============================================================================

void SearchService::buildSearchIndex(const QString& text)
{
    m_searchIndex.clear();
    m_indexedText = text;

    // 构建简单的单词索引
    QRegularExpression wordRegex("\\b\\w+\\b");
    QRegularExpressionMatchIterator iterator = wordRegex.globalMatch(text);

    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString word = match.captured().toLower();
        m_searchIndex[word].append(match.capturedStart());
    }
}

QList<SearchResult> SearchService::searchWithIndex(const QString& pattern,
    const SearchOptions& options) const
{
    QList<SearchResult> results;

    if (!options.useRegex && !options.wholeWords && !m_indexedText.isEmpty()) {
        QString searchPattern = pattern.toLower();
        if (m_searchIndex.contains(searchPattern)) {
            const QList<int>& positions = m_searchIndex[searchPattern];
            for (int position : positions) {
                SearchResult result = createSearchResult(m_indexedText, position,
                    pattern.length(), options);
                results.append(result);
            }
        }
    }

    return results;
}

// ==============================================================================
// 调试和统计
// ==============================================================================

QString SearchService::getDebugInfo() const
{
    QStringList info;

    info << "SearchService Debug Info:";
    info << QString("  Incremental search active: %1").arg(m_incrementalActive);
    info << QString("  Search history size: %1").arg(m_searchHistory.size());
    info << QString("  Search presets: %1").arg(m_searchPresets.size());
    info << QString("  Search index entries: %1").arg(m_searchIndex.size());

    if (m_incrementalActive) {
        info << QString("  Incremental text length: %1").arg(m_incrementalText.length());
        info << QString("  Incremental options: caseSensitive=%1, wholeWords=%2, useRegex=%3")
            .arg(m_incrementalOptions.caseSensitive)
            .arg(m_incrementalOptions.wholeWords)
            .arg(m_incrementalOptions.useRegex);
    }

    return info.join("\n");
}

// ==============================================================================
// 私有辅助方法
// ==============================================================================

QRegularExpression SearchService::createRegex(const QString& pattern, const SearchOptions& options) const
{
    QString regexPattern = pattern;

    if (!options.useRegex) {
        // 转义特殊字符
        regexPattern = escapeRegexPattern(pattern);
    }

    if (options.wholeWords) {
        regexPattern = QString("\\b%1\\b").arg(regexPattern);
    }

    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;

    if (!options.caseSensitive) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }

    // Qt 6中移除了OptimizeOnFirstUsageOption，直接使用NoPatternOption或其他有效选项

    QRegularExpression regex(regexPattern, patternOptions);

    if (!regex.isValid()) {
        qWarning() << "Invalid regex pattern:" << regexPattern << "Error:" << regex.errorString();
    }

    return regex;
}

QString SearchService::escapeRegexPattern(const QString& pattern) const
{
    return QRegularExpression::escape(pattern);
}

QString SearchService::extractContext(const QString& text, int position, int contextLines) const
{
    if (contextLines <= 0)
        return QString();

    // 找到匹配位置所在的行
    int matchLine = text.left(position).count('\n');

    // 计算上下文范围
    QStringList lines = text.split('\n');
    int startLine = qMax(0, matchLine - contextLines);
    int endLine = qMin(lines.size() - 1, matchLine + contextLines);

    QStringList contextLines_;
    for (int i = startLine; i <= endLine; ++i) {
        QString line = lines[i];
        if (i == matchLine) {
            // 标记匹配行
            line = QString(">>> %1").arg(line);
        }
        else {
            line = QString("    %1").arg(line);
        }
        contextLines_.append(line);
    }

    return contextLines_.join('\n');
}

SearchResult SearchService::createSearchResult(const QString& text, int position,
    int length, const SearchOptions& options) const
{
    SearchResult result;
    result.position = position;
    result.length = length;

    // 计算行号和列号
    QString textBeforeMatch = text.left(position);
    result.line = textBeforeMatch.count('\n');

    int lastNewlinePos = textBeforeMatch.lastIndexOf('\n');
    if (lastNewlinePos == -1) {
        result.column = position;
    }
    else {
        result.column = position - lastNewlinePos - 1;
    }

    // 提取上下文
    result.context = extractContext(text, position, options.contextLines);

    return result;
}

bool SearchService::isWholeWordMatch(const QString& text, int position, int length) const
{
    // 检查匹配前的字符
    if (position > 0) {
        QChar prevChar = text.at(position - 1);
        if (prevChar.isLetterOrNumber() || prevChar == '_') {
            return false;
        }
    }

    // 检查匹配后的字符
    int endPos = position + length;
    if (endPos < text.length()) {
        QChar nextChar = text.at(endPos);
        if (nextChar.isLetterOrNumber() || nextChar == '_') {
            return false;
        }
    }

    return true;
}

int SearchService::calculateEditDistance(const QString& str1, const QString& str2) const
{
    int m = str1.length();
    int n = str2.length();

    // 创建动态规划表
    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1));

    // 初始化
    for (int i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }

    // 填充表格
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (str1[i - 1] == str2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else {
                dp[i][j] = 1 + qMin(qMin(dp[i - 1][j], dp[i][j - 1]), dp[i - 1][j - 1]);
            }
        }
    }

    return dp[m][n];
}

int SearchService::findWordPosition(const QString& text, const QString& word, int wordIndex) const
{
    // 简化实现：在文本中找到第 wordIndex 个单词的位置
    QRegularExpression wordRegex("\\b" + QRegularExpression::escape(word) + "\\b");
    QRegularExpressionMatchIterator iterator = wordRegex.globalMatch(text);

    int currentIndex = 0;
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        if (currentIndex == wordIndex) {
            return match.capturedStart();
        }
        currentIndex++;
    }

    return -1;
}