#ifndef SEARCH_SERVICE_H
#define SEARCH_SERVICE_H

#include <QObject>
#include <QRegularExpression>
#include <QList>
#include <QFuture>
#include <QStringList>
#include <QHash>
#include <QDateTime>
#include <QVector>

struct SearchResult {
    int position = -1;
    int length = 0;
    int line = 0;
    int column = 0;
    QString context; // 搜索结果上下文
};

struct SearchOptions {
    bool caseSensitive = false;
    bool wholeWords = false;
    bool useRegex = false;
    bool searchBackward = false;
    int contextLines = 1;
};

struct SearchStatistics {
    int totalMatches = 0;
    qint64 searchTime = 0;
    double matchDensity = 0.0;
    QHash<int, int> matchesPerLine;
};

struct SearchHistoryItem {
    QString pattern;
    SearchOptions options;
    QDateTime timestamp;

    bool operator==(const SearchHistoryItem& other) const {
        return pattern == other.pattern && timestamp == other.timestamp;
    }
};

struct SearchPreset {
    QString name;
    QString pattern;
    SearchOptions options;
};

// 搜索服务
class SearchService : public QObject {
    Q_OBJECT

public:
    explicit SearchService(QObject* parent = nullptr);

    // 基础搜索方法
    QList<SearchResult> findAll(const QString& text, const QString& pattern,
        const SearchOptions& options = SearchOptions()) const;

    SearchResult findNext(const QString& text, const QString& pattern,
        int startPosition, const SearchOptions& options = SearchOptions()) const;

    SearchResult findPrevious(const QString& text, const QString& pattern,
        int startPosition, const SearchOptions& options = SearchOptions()) const;

    // 异步搜索（大文件）
    QFuture<QList<SearchResult>> findAllAsync(const QString& text, const QString& pattern,
        const SearchOptions& options = SearchOptions()) const;

    // 替换方法
    QString replaceAll(const QString& text, const QString& pattern,
        const QString& replacement, const SearchOptions& options = SearchOptions()) const;

    QString replaceFirst(const QString& text, const QString& pattern,
        const QString& replacement, int startPosition,
        const SearchOptions& options = SearchOptions()) const;

    // 增量搜索
    void startIncrementalSearch(const QString& text, const SearchOptions& options = SearchOptions());
    QList<SearchResult> updateIncrementalSearch(const QString& pattern);
    void stopIncrementalSearch();

    // 高级搜索功能
    QList<SearchResult> fuzzySearch(const QString& text, const QString& pattern,
        int maxDistance = 2, const SearchOptions& options = SearchOptions()) const;

    // 搜索统计
    SearchStatistics getSearchStatistics(const QString& text, const QString& pattern,
        const SearchOptions& options = SearchOptions()) const;

    // 搜索历史管理
    void addToHistory(const QString& pattern, const SearchOptions& options);
    QStringList getSearchHistory() const;
    void clearSearchHistory();

    // 搜索预设
    void saveSearchPreset(const QString& name, const QString& pattern,
        const SearchOptions& options);
    SearchOptions loadSearchPreset(const QString& name) const;
    QStringList getSearchPresets() const;

    // 性能优化：搜索索引
    void buildSearchIndex(const QString& text);
    QList<SearchResult> searchWithIndex(const QString& pattern,
        const SearchOptions& options = SearchOptions()) const;

    // 调试和统计
    QString getDebugInfo() const;

signals:
    void searchProgress(int percentage);
    void searchCompleted(const QList<SearchResult>& results);
    void incrementalSearchUpdated(const QList<SearchResult>& results);

private:
    // 增量搜索状态
    QString m_incrementalText;
    SearchOptions m_incrementalOptions;
    bool m_incrementalActive = false;

    // 搜索历史
    QList<SearchHistoryItem> m_searchHistory;

    // 搜索预设
    QHash<QString, SearchPreset> m_searchPresets;

    // 搜索索引
    QHash<QString, QList<int>> m_searchIndex;
    QString m_indexedText;

    // 私有辅助方法
    QRegularExpression createRegex(const QString& pattern, const SearchOptions& options) const;
    QString escapeRegexPattern(const QString& pattern) const;
    QString extractContext(const QString& text, int position, int contextLines) const;
    SearchResult createSearchResult(const QString& text, int position,
        int length, const SearchOptions& options) const;
    bool isWholeWordMatch(const QString& text, int position, int length) const;

    // 高级搜索辅助方法
    int calculateEditDistance(const QString& str1, const QString& str2) const;
    int findWordPosition(const QString& text, const QString& word, int wordIndex) const;
};

#endif // SEARCH_SERVICE_H