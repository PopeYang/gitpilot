#include "ConfigManager.h"
#include <QCoreApplication>
#include <QByteArray>

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    // QSettings会自动使用组织名和应用名创建配置文件
    // 需要在main.cpp中设置QCoreApplication::setOrganizationName/setApplicationName
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                               "GitPilot", "GitPilot");
}

ConfigManager::~ConfigManager() {
    delete m_settings;
}

// ========== 首次运行检测 ==========

bool ConfigManager::isFirstRun() {
    return !m_settings->contains("Setup/Completed");
}

void ConfigManager::setFirstRunCompleted() {
    m_settings->setValue("Setup/Completed", true);
    m_settings->sync();
}

// ========== GitLab连接配置 ==========

QString ConfigManager::getGitLabUrl() {
    return m_settings->value("GitLab/Url", DEFAULT_GITLAB_URL).toString();
}

void ConfigManager::setGitLabUrl(const QString& url) {
    m_settings->setValue("GitLab/Url", url);
    m_settings->sync();
}

QString ConfigManager::getGitLabToken() {
    QString encrypted = m_settings->value("GitLab/Token").toString();
    if (encrypted.isEmpty()) {
        return QString();
    }
    return decryptToken(encrypted);
}

void ConfigManager::setGitLabToken(const QString& token) {
    if (token.isEmpty()) {
        m_settings->remove("GitLab/Token");
    } else {
        QString encrypted = encryptToken(token);
        m_settings->setValue("GitLab/Token", encrypted);
    }
    m_settings->sync();
}

// ========== 项目配置 ==========

QString ConfigManager::getCurrentProjectId() {
    return m_settings->value("Project/CurrentId").toString();
}

void ConfigManager::setCurrentProjectId(const QString& id) {
    m_settings->setValue("Project/CurrentId", id);
    m_settings->sync();
}

QString ConfigManager::getCurrentProjectName() {
    return m_settings->value("Project/CurrentName").toString();
}

void ConfigManager::setCurrentProjectName(const QString& name) {
    m_settings->setValue("Project/CurrentName", name);
    m_settings->sync();
}

QString ConfigManager::getRepoPath() {
    return m_settings->value("Project/RepoPath").toString();
}

void ConfigManager::setRepoPath(const QString& path) {
    m_settings->setValue("Project/RepoPath", path);
    m_settings->sync();
}

// ========== 分支保护规则 ==========

QStringList ConfigManager::getProtectedBranches() {
    QStringList defaultBranches = {"main", "master", "develop", "internal"};
    return m_settings->value("Branches/Protected", defaultBranches).toStringList();
}

void ConfigManager::setProtectedBranches(const QStringList& branches) {
    m_settings->setValue("Branches/Protected", branches);
    m_settings->sync();
}

QString ConfigManager::getDatabaseBranchName() {
    return m_settings->value("Branches/Database", DEFAULT_DATABASE_BRANCH).toString();
}

void ConfigManager::setDatabaseBranchName(const QString& name) {
    m_settings->setValue("Branches/Database", name);
    m_settings->sync();
}

// ========== 构建配置 ==========

QString ConfigManager::getArtifactPattern() {
    return m_settings->value("Build/ArtifactPattern", DEFAULT_ARTIFACT_PATTERN).toString();
}

void ConfigManager::setArtifactPattern(const QString& pattern) {
    m_settings->setValue("Build/ArtifactPattern", pattern);
    m_settings->sync();
}

int ConfigManager::getPipelinePollInterval() {
    return m_settings->value("Build/PollInterval", DEFAULT_POLL_INTERVAL).toInt();
}

void ConfigManager::setPipelinePollInterval(int seconds) {
    m_settings->setValue("Build/PollInterval", seconds);
    m_settings->sync();
}

// ========== 日志配置 ==========

bool ConfigManager::isLoggingEnabled() {
    return m_settings->value("Logging/Enabled", true).toBool();
}

void ConfigManager::setLoggingEnabled(bool enabled) {
    m_settings->setValue("Logging/Enabled", enabled);
    m_settings->sync();
}

// ========== Token加密/解密 ==========

QString ConfigManager::encryptToken(const QString& token) {
    // 简单的XOR加密（仅用于防止明文存储，不是强加密）
    const QByteArray key = "GitPilotEncKey"; // 密钥
    QByteArray data = token.toUtf8();
    QByteArray encrypted;
    
    for (int i = 0; i < data.size(); ++i) {
        encrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    // Base64编码存储
    return encrypted.toBase64();
}

QString ConfigManager::decryptToken(const QString& encrypted) {
    // Base64解码
    QByteArray data = QByteArray::fromBase64(encrypted.toUtf8());
    
    // XOR解密
    const QByteArray key = "GitPilotEncKey";
    QByteArray decrypted;
    
    for (int i = 0; i < data.size(); ++i) {
        decrypted.append(data[i] ^ key[i % key.size()]);
    }
    
    return QString::fromUtf8(decrypted);
}
