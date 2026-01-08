#ifndef GITSERVICE_H
#define GITSERVICE_H

#include <QString>
#include <QStringList>
#include <QObject>
#include <QPair>

/**
 * @brief 文件状态结构
 */
struct FileStatus {
    QString filename;
    QString status;  // "M"=Modified, "A"=Added, "D"=Deleted, "??"=Untracked
    QString displayText;
};

/**
 * @brief Cherry-pick冲突检测结果
 */
struct CherryPickConflictResult {
    bool hasConflict;           // 是否存在冲突
    QStringList conflictFiles;  // 冲突文件列表
    QString errorMessage;       // 错误信息（如果检测失败）
};

/**
 * @brief Git操作服务层
 * 封装所有Git命令，使用QProcess执行
 */
class GitService : public QObject {
    Q_OBJECT
    
public:
    explicit GitService(QObject* parent = nullptr);
    
    // 仓库管理
    void setRepoPath(const QString& path);
    QString getRepoPath() const { return m_repoPath; }
    bool isValidRepo();
    
    // 分支操作
    QString getCurrentBranch();
    QStringList getAllBranches();
    bool createBranch(const QString& newBranch, const QString& baseBranch = QString());
    bool switchBranch(const QString& branchName);
    bool deleteBranch(const QString& branchName, bool force = false);
    
    // 代码状态
    bool hasUncommittedChanges();
    bool hasUnpushedCommits();
    QStringList getModifiedFiles();
    QList<FileStatus> getFileStatus();  // 新增：获取详细文件状态
    
    // 提交操作
    bool stageAll();
    bool stageFiles(const QStringList& files);
    bool commit(const QString& message);
    QStringList getRecentCommits(int count = 10);
    QString getLastCommitMessage();
    
    // 远程操作
    bool pushBranch(const QString& branchName, bool setUpstream = false);
    bool pullLatest();
    bool fetch();
    bool checkMergeConflict(const QString& targetBranch, QString& conflictInfo);  // 检查合并冲突
    CherryPickConflictResult checkCherryPickConflict(const QString& sourceBranch, const QString& targetBranch);  // Cherry-pick冲突检测
    
    // Tags操作
    QStringList getTags(int limit = 10);  // 获取最近的Tags
    QStringList getGraphLog(int limit = 20); // 获取图形化日志
    
    // 远程仓库信息
    QString getRemoteUrl();
    bool hasRemote();
    
    // 仓库管理（静态方法）
    static bool cloneRepository(const QString& url, const QString& targetPath, QString& error);
    
signals:
    void operationStarted(const QString& operation);
    void operationFinished(const QString& operation, bool success);
    void outputReceived(const QString& output);
    void errorReceived(const QString& error);
    
private:
    QString m_repoPath;
    
    // 执行Git命令
    bool executeGitCommand(const QStringList& args, QString& output, QString& error);
    QString executeGitCommandSimple(const QStringList& args);
    
    // 辅助方法
    bool isGitInstalled();
};

#endif // GITSERVICE_H
