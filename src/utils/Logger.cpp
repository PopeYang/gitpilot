#include "Logger.h"
#include "config/ConfigManager.h"
#include <QDir>
#include <QStandardPaths>
#include <QMutexLocker>

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // 日志文件路径：应用数据目录/logs/gitgui.log
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logDir);
    
    QString logPath = logDir + "/gitgui.log";
    m_logFile.setFileName(logPath);
    
    // 以追加模式打开
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        
        // 记录启动信息
        QString startMsg = QString("\n========== %1 ==========\n")
                          .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        m_stream << startMsg;
        m_stream.flush();
    }
}

Logger::~Logger() {
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::debug(const QString& message) {
    log(Level::Debug, message);
}

void Logger::info(const QString& message) {
    log(Level::Info, message);
}

void Logger::warning(const QString& message) {
    log(Level::Warning, message);
}

void Logger::error(const QString& message) {
    log(Level::Error, message);
}

void Logger::log(Level level, const QString& message) {
    // 检查是否启用日志
    if (!ConfigManager::instance().isLoggingEnabled()) {
        return;
    }
    
    QMutexLocker locker(&m_mutex); // 线程安全
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString levelStr = getLevelString(level);
    QString formattedMessage = QString("[%1] [%2] %3\n")
                              .arg(timestamp)
                              .arg(levelStr)
                              .arg(message);
    
    writeToFile(formattedMessage);
}

void Logger::writeToFile(const QString& formattedMessage) {
    if (m_logFile.isOpen()) {
        m_stream << formattedMessage;
        m_stream.flush(); // 立即写入文件
    }
}

QString Logger::getLevelString(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO ";
        case Level::Warning: return "WARN ";
        case Level::Error:   return "ERROR";
        default:             return "UNKNW";
    }
}

QString Logger::getLogFilePath() const {
    return m_logFile.fileName();
}

void Logger::clearLogs() {
    QMutexLocker locker(&m_mutex);
    
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
    
    // 清空文件
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    m_logFile.close();
    
    // 重新打开
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_stream.setDevice(&m_logFile);
}
