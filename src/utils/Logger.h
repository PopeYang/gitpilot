#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

/**
 * @brief 日志记录器 - 单例模式
 * 记录所有Git操作、API调用和重要事件
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };
    
    static Logger& instance();
    
    // 日志记录方法
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    
    // 通用日志方法
    void log(Level level, const QString& message);
    
    // 日志管理
    QString getLogFilePath() const;
    void clearLogs();
    
private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void writeToFile(const QString& formattedMessage);
    QString getLevelString(Level level);
    
    QFile m_logFile;
    QTextStream m_stream;
    QMutex m_mutex; // 线程安全
};

// 便捷宏定义
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)

#endif // LOGGER_H
