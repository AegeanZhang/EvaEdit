#include "FileController.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMimeDatabase>
#include <QStringConverter> // 【修改】Qt 6 中替代 QTextCodec
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QEventLoop>
#include <QTimer>

#include "../config/ConfigCenter.h"
#include "../logger/Logger.h"
#include "TabController.h"

FileController* FileController::m_instance = nullptr;

// 常量定义
const qint64 FileController::MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB 限制
const QStringList FileController::SUPPORTED_TEXT_EXTENSIONS = {
    "txt", "cpp", "h", "hpp", "c", "cc", "cxx", "hxx",
    "js", "qml", "json", "xml", "html", "htm", "css",
    "py", "java", "cs", "php", "rb", "go", "rs",
    "md", "log", "ini", "cfg", "conf"
};

FileController* FileController::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return instance();
}

FileController* FileController::instance()
{
    if (!m_instance) {
        m_instance = new FileController();
    }
    return m_instance;
}

FileController::FileController(QObject* parent)
    : QObject(parent)
    , m_configCenter(ConfigCenter::instance())
{
    LOG_DEBUG("FileController 初始化");

    // 初始化文件修改状态映射
    m_fileModifiedStatus.clear();

    // 【新增】初始化编辑器内容缓存和性能监控
    m_editorContents.clear();
    m_updateTimers.clear();
    m_updateCounts.clear();

    // 连接配置中心信号，监听最近文件变化
    connect(m_configCenter, &ConfigCenter::recentFilesChanged,
        this, &FileController::recentFilesChanged);
}

FileController::~FileController()
{
    LOG_DEBUG("FileController 析构");
}

bool FileController::openFile(const QString& filePath)
{
    LOG_DEBUG(QString("尝试打开文件: %1").arg(filePath));

    if (filePath.isEmpty()) {
        LOG_WARN("文件路径为空");
        return false;
    }

    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("文件不存在: %1").arg(filePath));
        return false;
    }

    // 检查文件是否可读
    if (!fileInfo.isReadable()) {
        LOG_ERROR(QString("文件无读取权限: %1").arg(filePath));
        return false;
    }

    // 检查文件大小
    if (fileInfo.size() > MAX_FILE_SIZE) {
        LOG_ERROR(QString("文件过大: %1 (%2 bytes)").arg(filePath).arg(fileInfo.size()));
        return false;
    }

    // 检查文件类型是否支持
    if (!isSupportedFileType(filePath)) {
        LOG_WARN(QString("不支持的文件类型: %1").arg(filePath));
        // 仍然尝试打开，但记录警告
    }

    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR(QString("无法打开文件: %1, 错误: %2").arg(filePath, file.errorString()));
        return false;
    }

    // 【修改】使用 Qt 6 的新方式检测和处理编码
    QString encoding = detectFileEncoding(filePath);
    QString content;

    // 【修改】Qt 6 中的文本流编码处理
    if (encoding == "UTF-8" || encoding == "UTF-8-BOM") {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        content = stream.readAll();
    }
    else if (encoding == "UTF-16LE") {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf16LE);
        content = stream.readAll();
    }
    else if (encoding == "UTF-16BE") {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf16BE);
        content = stream.readAll();
    }
    else {
        // 对于其他编码，使用字节读取后转换
        QByteArray data = file.readAll();
        content = convertFromEncoding(data, encoding);
    }

    file.close();

    // 标记文件为未修改状态
    m_fileModifiedStatus[filePath] = false;

    // 记录文件编码信息
    m_fileEncodings[filePath] = encoding;

    // 添加到最近文件列表
    addToRecentFiles(filePath);

    // 发出文件打开信号
    emit fileOpened(filePath, content);

    LOG_INFO(QString("成功打开文件: %1, 编码: %2, 大小: %3 字符")
        .arg(filePath, encoding)
        .arg(content.length()));

    return true;
}


// 【修改】saveFile 方法，保存前强制更新内容
bool FileController::saveFile(const QString& filePath)
{
    QString targetPath = filePath;

    if (targetPath.isEmpty()) {
        TabController* tabController = TabController::instance();
        targetPath = tabController->currentFilePath();

        if (targetPath.isEmpty()) {
            LOG_WARN("没有指定保存路径且当前没有活动文件");
            return false;
        }

        if (tabController->isNewFile(targetPath)) {
            LOG_DEBUG("新建文件需要另存为");
            return false;
        }
    }

    // 【新增】保存前强制更新内容
    emit forceContentUpdateRequested(targetPath);

    // 短暂延迟确保内容已更新
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    return saveFileContent(targetPath, getCurrentFileContent(targetPath));
}

bool FileController::saveAsFile(const QString& filePath)
{
    if (filePath.isEmpty()) {
        LOG_WARN("另存为路径为空");
        return false;
    }

    // 获取当前文件内容
    TabController* tabController = TabController::instance();
    QString currentPath = tabController->currentFilePath();

    // 【新增】保存前强制更新内容
    emit forceContentUpdateRequested(currentPath);

    // 短暂延迟确保内容已更新
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    QString content = getCurrentFileContent(currentPath);

    // 保存到新路径
    if (saveFileContent(filePath, content)) {
        if (tabController->isNewFile(currentPath)) {
            // 【情况1】如果原文件是新建文件，更新当前标签页路径
            int currentIndex = tabController->currentTabIndex();
            tabController->saveFileAs(currentIndex, filePath);
        }
        else {
            // 【情况2】如果原文件是已存在的文件，创建新标签页
            int newTabIndex = tabController->addNewTab(filePath);
            if (newTabIndex >= 0) {
                // 初始化新标签页的内容缓存
                m_editorContents[filePath] = content;
                m_fileModifiedStatus[filePath] = false;

                // 发出文件打开信号，让编辑器显示内容
                emit fileOpened(filePath, content);

                LOG_INFO(QString("另存为创建新标签页: %1").arg(filePath));
            }
        }

        return true;
    }

    return false;
}

bool FileController::newFile()
{
    LOG_DEBUG("创建新文件");

    TabController* tabController = TabController::instance();
    int newTabIndex = tabController->addNewTab();

    if (newTabIndex >= 0) {
        QString newFilePath = tabController->getTabFilePath(newTabIndex);

        // 标记为未修改状态
        m_fileModifiedStatus[newFilePath] = false;

        // 发出文件打开信号（空内容）
        emit fileOpened(newFilePath, QString());

        LOG_INFO(QString("创建新文件: %1").arg(newFilePath));
        return true;
    }

    return false;
}

bool FileController::isFileModified(const QString& filePath) const
{
    return m_fileModifiedStatus.value(filePath, false);
}

QString FileController::getFileEncoding(const QString& filePath) const
{
    return m_fileEncodings.value(filePath, "UTF-8");
}

qint64 FileController::getFileSize(const QString& filePath) const
{
    if (filePath.isEmpty()) {
        return 0;
    }

    // 如果是新建文件URL，返回0
    TabController* tabController = TabController::instance();
    if (tabController->isNewFile(filePath)) {
        return 0;
    }

    QFileInfo fileInfo(filePath);
    return fileInfo.exists() ? fileInfo.size() : 0;
}

QStringList FileController::getRecentFiles() const
{
    return m_configCenter->recentFiles();
}

void FileController::addToRecentFiles(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    // 不要添加新建文件URL到最近文件列表
    TabController* tabController = TabController::instance();
    if (tabController->isNewFile(filePath)) {
        return;
    }

    m_configCenter->addRecentFile(filePath);
    LOG_DEBUG(QString("添加到最近文件: %1").arg(filePath));
}

void FileController::markFileModified(const QString& filePath, bool modified)
{
    if (m_fileModifiedStatus.value(filePath) != modified) {
        m_fileModifiedStatus[filePath] = modified;
        emit fileModifiedChanged(filePath, modified);

        LOG_DEBUG(QString("文件修改状态变更: %1 -> %2")
            .arg(filePath)
            .arg(modified ? "已修改" : "未修改"));
    }
}

void FileController::clearRecentFiles()
{
    m_configCenter->clearRecentFiles();
    LOG_INFO("清空最近文件列表");
}

bool FileController::saveFileContent(const QString& filePath, const QString& content)
{
    LOG_DEBUG(QString("保存文件内容到: %1").arg(filePath));

    // 确保目录存在
    QFileInfo fileInfo(filePath);
    QDir parentDir = fileInfo.absoluteDir();
    if (!parentDir.exists() && !parentDir.mkpath(parentDir.absolutePath())) {
        LOG_ERROR(QString("无法创建目录: %1").arg(parentDir.absolutePath()));
        return false;
    }

    // 备份原文件（如果存在且大于一定大小）
    if (fileInfo.exists() && fileInfo.size() > 1024) { // 1KB以上才备份
        createBackup(filePath);
    }

    // 保存文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_ERROR(QString("无法写入文件: %1, 错误: %2").arg(filePath, file.errorString()));
        return false;
    }

    // 【修改】使用 Qt 6 的文本流编码
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8); // 使用UTF-8编码保存
    stream << content;

    file.close();

    // 更新文件状态
    m_fileModifiedStatus[filePath] = false;
    m_fileEncodings[filePath] = "UTF-8";

    // 添加到最近文件
    addToRecentFiles(filePath);

    // 发出保存信号
    emit fileSaved(filePath);
    emit fileModifiedChanged(filePath, false);

    LOG_INFO(QString("成功保存文件: %1, 大小: %2 字符").arg(filePath).arg(content.length()));

    return true;
}

// 【修改】Qt 6 兼容的编码检测函数
QString FileController::detectFileEncoding(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "UTF-8";
    }

    // 读取文件开头的一些字节来检测编码
    QByteArray header = file.read(1024);
    file.close();

    // 检测UTF-8 BOM
    if (header.startsWith("\xEF\xBB\xBF")) {
        return "UTF-8-BOM";
    }

    // 检测UTF-16 LE BOM
    if (header.startsWith("\xFF\xFE")) {
        return "UTF-16LE";
    }

    // 检测UTF-16 BE BOM
    if (header.startsWith("\xFE\xFF")) {
        return "UTF-16BE";
    }

    // 【修改】使用 Qt 6 的正确 API 验证UTF-8
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    QString result = utf8Decoder.decode(header);

    // 【修改】检查是否有错误的正确方式
    if (!utf8Decoder.hasError()) {
        // 进一步检查是否包含替换字符，这可能表示编码错误
        if (!result.contains(QChar::ReplacementCharacter)) {
            return "UTF-8";
        }
    }

    // 检查是否可能是 Latin1 (ASCII 兼容)
    bool isAsciiCompatible = true;
    for (char byte : header) {
        unsigned char ubyte = static_cast<unsigned char>(byte);
        if (ubyte > 127) {
            isAsciiCompatible = false;
            break;
        }
    }

    if (isAsciiCompatible) {
        return "UTF-8"; // ASCII 文本，当作 UTF-8 处理
    }

    // 对于中文系统，默认假设为系统本地编码
    return "System"; // 使用系统默认编码
}

// 【新增】Qt 6 兼容的编码转换函数
QString FileController::convertFromEncoding(const QByteArray& data, const QString& encoding)
{
    if (encoding == "UTF-8" || encoding == "UTF-8-BOM") {
        // 【修改】处理 UTF-8 BOM
        QByteArray processedData = data;
        if (encoding == "UTF-8-BOM" && data.startsWith("\xEF\xBB\xBF")) {
            processedData = data.mid(3); // 跳过 BOM
        }
        return QString::fromUtf8(processedData);
    }
    else if (encoding == "UTF-16LE") {
        // 【修改】处理可能的 BOM
        QByteArray processedData = data;
        if (data.startsWith("\xFF\xFE")) {
            processedData = data.mid(2); // 跳过 BOM
        }
        return QString::fromUtf16(reinterpret_cast<const char16_t*>(processedData.constData()),
            processedData.size() / 2);
    }
    else if (encoding == "UTF-16BE") {
        // 【修改】处理可能的 BOM 和字节序转换
        QByteArray processedData = data;
        if (data.startsWith("\xFE\xFF")) {
            processedData = data.mid(2); // 跳过 BOM
        }

        // 需要字节序转换
        QByteArray swapped = processedData;
        for (int i = 0; i < swapped.size() - 1; i += 2) {
            std::swap(swapped[i], swapped[i + 1]);
        }
        return QString::fromUtf16(reinterpret_cast<const char16_t*>(swapped.constData()),
            swapped.size() / 2);
    }
    else if (encoding == "System") {
        // 使用本地编码
        return QString::fromLocal8Bit(data);
    }
    else {
        // 默认使用UTF-8，如果失败则使用Latin1
        QString result = QString::fromUtf8(data);
        if (result.contains(QChar::ReplacementCharacter)) {
            result = QString::fromLatin1(data);
        }
        return result;
    }
}

bool FileController::isSupportedFileType(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();

    // 检查扩展名是否在支持列表中
    if (SUPPORTED_TEXT_EXTENSIONS.contains(extension)) {
        return true;
    }

    // 使用MIME类型检测
    static QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fileInfo);

    // 检查是否为文本类型
    if (mimeType.name().startsWith("text/") ||
        mimeType.inherits("text/plain")) {
        return true;
    }

    // 检查一些特殊的文本MIME类型
    QStringList textMimeTypes = {
        "application/javascript",
        "application/json",
        "application/xml",
        "application/x-shellscript"
    };

    return textMimeTypes.contains(mimeType.name());
}

// 【修改】getCurrentFileContent 实现
QString FileController::getCurrentFileContent(const QString& filePath)
{
    LOG_DEBUG(QString("获取文件内容: %1").arg(filePath));

    // 直接从缓存中获取内容
    return m_editorContents.value(filePath, QString());
}

void FileController::createBackup(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return;
    }

    // 生成备份文件名：原文件名.backup.时间戳
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString backupPath = QString("%1.backup.%2")
        .arg(filePath)
        .arg(timestamp);

    if (QFile::copy(filePath, backupPath)) {
        LOG_DEBUG(QString("创建备份文件: %1").arg(backupPath));

        // 清理旧的备份文件（保留最近5个）
        cleanupBackups(filePath);
    }
    else {
        LOG_WARN(QString("备份文件创建失败: %1").arg(backupPath));
    }
}

void FileController::cleanupBackups(const QString& originalFilePath)
{
    QFileInfo fileInfo(originalFilePath);
    QDir dir = fileInfo.absoluteDir();

    QString baseName = fileInfo.fileName();
    QStringList filters;
    filters << QString("%1.backup.*").arg(baseName);

    QFileInfoList backupFiles = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    // 保留最近的5个备份文件，删除其余的
    const int maxBackups = 5;
    if (backupFiles.size() > maxBackups) {
        for (int i = maxBackups; i < backupFiles.size(); ++i) {
            QFile::remove(backupFiles[i].absoluteFilePath());
            LOG_DEBUG(QString("删除旧备份文件: %1").arg(backupFiles[i].absoluteFilePath()));
        }
    }
}

// 公开的工具方法

bool FileController::fileExists(const QString& filePath)
{
    return !filePath.isEmpty() && QFileInfo::exists(filePath);
}

QString FileController::getFileName(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QString();
    }

    // 处理新建文件URL
    TabController* tabController = TabController::instance();
    if (tabController->isNewFile(filePath)) {
        return tabController->getDisplayName(filePath);
    }

    return QFileInfo(filePath).fileName();
}

QString FileController::getFileDirectory(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QString();
    }

    // 新建文件URL没有目录
    TabController* tabController = TabController::instance();
    if (tabController->isNewFile(filePath)) {
        return QString();
    }

    return QFileInfo(filePath).absolutePath();
}

QDateTime FileController::getFileLastModified(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return QDateTime();
    }

    // 新建文件URL返回当前时间
    TabController* tabController = TabController::instance();
    if (tabController->isNewFile(filePath)) {
        return QDateTime::currentDateTime();
    }

    QFileInfo fileInfo(filePath);
    return fileInfo.exists() ? fileInfo.lastModified() : QDateTime();
}

// 【新增】更新编辑器内容方法
void FileController::updateEditorContent(const QString& filePath, const QString& content)
{
    if (filePath.isEmpty()) {
        return;
    }

    // 【新增】性能监控
    if (!m_updateTimers.contains(filePath)) {
        m_updateTimers[filePath].start();
        m_updateCounts[filePath] = 0;
    }

    m_updateCounts[filePath]++;

    // 每50次更新报告一次性能数据
    if (m_updateCounts[filePath] % 50 == 0) {
        qint64 elapsed = m_updateTimers[filePath].elapsed();
        double avgInterval = double(elapsed) / m_updateCounts[filePath];

        LOG_DEBUG(QString("文件 %1 内容更新统计: %2 次更新，平均间隔 %3 ms")
            .arg(QFileInfo(filePath).fileName())
            .arg(m_updateCounts[filePath])
            .arg(avgInterval, 0, 'f', 1));
    }

    // 只有内容真正变化时才更新
    QString currentContent = m_editorContents.value(filePath);
    if (currentContent == content) {
        return; // 内容没有变化，跳过更新
    }

    m_editorContents[filePath] = content;

    // 简单的修改状态检测 - 这里可以根据需要实现更复杂的逻辑
    bool wasModified = m_fileModifiedStatus.value(filePath, false);
    bool isModified = !content.isEmpty(); // 简化逻辑：有内容就认为已修改

    if (wasModified != isModified) {
        markFileModified(filePath, isModified);
    }

    LOG_DEBUG(QString("更新编辑器内容缓存: %1, 长度: %2")
        .arg(QFileInfo(filePath).fileName())
        .arg(content.length()));
}

