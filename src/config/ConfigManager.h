#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QStringList>
#include <QSettings>

/**
 * @brief 配置管理器 - 单例模式
 * 使用QSettings持久化配置到用户目录
 * Windows: C:\Users\<用户名>\AppData\Roaming\GitGuiTeam\GitGuiClient.ini
 */
class ConfigManager {
public:
    static ConfigManager& instance();
    
    // 首次运行检测
    bool isFirstRun();
    void setFirstRunCompleted();
    
    // GitLab连接配置
    QString getGitLabUrl();
    void setGitLabUrl(const QString& url);
    
    QString getGitLabToken();
    void setGitLabToken(const QString& token);
    
    // 项目配置
    QString getCurrentProjectId();
    void setCurrentProjectId(const QString& id);
    
    QString getCurrentProjectName();
    void setCurrentProjectName(const QString& name);
    
    QString getRepoPath();
    void setRepoPath(const QString& path);
    
    // 分支保护规则
    QStringList getProtectedBranches();
    void setProtectedBranches(const QStringList& branches);
    
    QString getDatabaseBranchName();
    void setDatabaseBranchName(const QString& name);
    
    // 构建配置
    QString getArtifactPattern();
    void setArtifactPattern(const QString& pattern);
    
    int getPipelinePollInterval();
    void setPipelinePollInterval(int seconds);
    
    // 日志配置
    bool isLoggingEnabled();
    void setLoggingEnabled(bool enabled);
    
    // 默认值
    static constexpr const char* DEFAULT_GITLAB_URL = "https://gitlab.example.com";
    static constexpr const char* DEFAULT_DATABASE_BRANCH = "develop-database";
    static constexpr const char* DEFAULT_ARTIFACT_PATTERN = R"(https?://[^\s]+\.(apk|exe|zip|tar\.gz))";
    static constexpr int DEFAULT_POLL_INTERVAL = 10; // 秒
    
private:
    ConfigManager();
    ~ConfigManager();
    
    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    QSettings* m_settings;
    
    // Token加密/解密（简单的XOR + Base64）
    QString encryptToken(const QString& token);
    QString decryptToken(const QString& encrypted);
};

#endif // CONFIGMANAGER_H
