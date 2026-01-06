#include "GitService.h"
#include "utils/Logger.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

GitService::GitService(QObject* parent)
    : QObject(parent)
{
}

void GitService::setRepoPath(const QString& path) {
    m_repoPath = path;
    LOG_INFO(QString("设置仓库路径: %1").arg(path));
}

bool GitService::isValidRepo() {
    if (m_repoPath.isEmpty()) {
        return false;
    }
    
    // 检查.git目录是否存在
    QFileInfo gitDir(m_repoPath + "/.git");
    return gitDir.exists() && gitDir.isDir();
}

// ========== 分支操作 ==========

QString GitService::getCurrentBranch() {
    QString output = executeGitCommandSimple({"branch", "--show-current"});
    LOG_INFO(QString("当前分支: %1").arg(output));
    return output;
}

QStringList GitService::getAllBranches() {
    QString output = executeGitCommandSimple({"branch", "-a"});
    QStringList branches;
    
    for (const QString& line : output.split('\n')) {
        QString branch = line.trimmed();
        if (branch.isEmpty()) continue;
        
        // 移除*标记和remotes/前缀
        branch.remove(QRegularExpression("^\\* "));
        branch.remove("remotes/origin/");
        
        if (!branch.contains("HEAD ->")) {
            branches.append(branch);
        }
    }
    
    branches.removeDuplicates();
    return branches;
}

bool GitService::createBranch(const QString& newBranch, const QString& baseBranch) {
    QStringList args = {"checkout", "-b", newBranch};
    if (!baseBranch.isEmpty()) {
        args.append(baseBranch);
    }
    
    QString output, error;
    bool success = executeGitCommand(args, output, error);
    
    if (success) {
        LOG_INFO(QString("创建分支成功: %1").arg(newBranch));
    } else {
        LOG_ERROR(QString("创建分支失败: %1, 错误: %2").arg(newBranch, error));
    }
    
    return success;
}

bool GitService::switchBranch(const QString& branchName) {
    QString output, error;
    bool success = executeGitCommand({"checkout", branchName}, output, error);
    
    if (success) {
        LOG_INFO(QString("切换分支成功: %1").arg(branchName));
    } else {
        LOG_ERROR(QString("切换分支失败: %1, 错误: %2").arg(branchName, error));
    }
    
    return success;
}

bool GitService::deleteBranch(const QString& branchName, bool force) {
    QString flag = force ? "-D" : "-d";
    QString output, error;
    bool success = executeGitCommand({"branch", flag, branchName}, output, error);
    
    if (success) {
        LOG_INFO(QString("删除分支成功: %1").arg(branchName));
    } else {
        LOG_ERROR(QString("删除分支失败: %1, 错误: %2").arg(branchName, error));
    }
    
    return success;
}

// ========== 代码状态 ==========

bool GitService::hasUncommittedChanges() {
    QString output = executeGitCommandSimple({"status", "--porcelain"});
    return !output.trimmed().isEmpty();
}

bool GitService::hasUnpushedCommits() {
    QString output = executeGitCommandSimple({"log", "@{u}..", "--oneline"});
    return !output.trimmed().isEmpty();
}

QStringList GitService::getModifiedFiles() {
    QString output = executeGitCommandSimple({"status", "--porcelain"});
    QStringList files;
    
    for (const QString& line : output.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        
        // 格式: "M  file.cpp" 或 "?? newfile.txt"
        QString file = line.mid(3); // 跳过状态标记
        files.append(file);
    }
    
    return files;
}

// ========== 提交操作 ==========

bool GitService::stageAll() {
    QString output, error;
    bool success = executeGitCommand({"add", "-A"}, output, error);
    
    if (success) {
        LOG_INFO("暂存所有文件成功");
    }
    
    return success;
}

bool GitService::stageFiles(const QStringList& files) {
    QStringList args = {"add"};
    args.append(files);
    
    QString output, error;
    bool success = executeGitCommand(args, output, error);
    
    if (success) {
        LOG_INFO(QString("暂存文件成功: 共%1个").arg(files.size()));
    }
    
    return success;
}

bool GitService::commit(const QString& message) {
    QString output, error;
    bool success = executeGitCommand({"commit", "-m", message}, output, error);
    
    if (success) {
        LOG_INFO(QString("提交成功: %1").arg(message));
    } else {
        LOG_ERROR(QString("提交失败: %1").arg(error));
    }
    
    return success;
}

QStringList GitService::getRecentCommits(int count) {
    QString output = executeGitCommandSimple({"log", QString("-%1").arg(count), "--pretty=format:%s"});
    return output.split('\n', Qt::SkipEmptyParts);
}

QString GitService::getLastCommitMessage() {
    return executeGitCommandSimple({"log", "-1", "--pretty=format:%s"});
}

// ========== 远程操作 ==========

bool GitService::pushBranch(const QString& branchName, bool setUpstream) {
    QStringList args = {"push"};
    
    if (setUpstream) {
        args << "-u" << "origin" << branchName;
    } else {
        args << "origin" << branchName;
    }
    
    QString output, error;
    bool success = executeGitCommand(args, output, error);
    
    if (success) {
        LOG_INFO(QString("推送分支成功: %1").arg(branchName));
    } else {
        LOG_ERROR(QString("推送分支失败: %1, 错误: %2").arg(branchName, error));
    }
    
    return success;
}

bool GitService::pullLatest() {
    QString output, error;
    bool success = executeGitCommand({"pull"}, output, error);
    
    if (success) {
        LOG_INFO("拉取最新代码成功");
    } else {
        LOG_ERROR(QString("拉取失败: %1").arg(error));
    }
    
    return success;
}

bool GitService::fetch() {
    QString output, error;
    return executeGitCommand({"fetch"}, output, error);
}

QString GitService::getRemoteUrl() {
    return executeGitCommandSimple({"remote", "get-url", "origin"});
}

bool GitService::hasRemote() {
    QString output = executeGitCommandSimple({"remote"});
    return !output.trimmed().isEmpty();
}

// ========== 私有方法 ==========

bool GitService::executeGitCommand(const QStringList& args, QString& output, QString& error) {
    if (!isGitInstalled()) {
        error = "Git未安装或不在PATH中";
        LOG_ERROR(error);
        return false;
    }
    
    QProcess process;
    process.setWorkingDirectory(m_repoPath);
    
    emit operationStarted(args.join(' '));
    
    process.start("git", args);
    process.waitForFinished(30000); // 30秒超时
    
    output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    error = QString::fromUtf8(process.readAllStandardError()).trimmed();
    
    bool success = (process.exitCode() == 0);
    
    if (!output.isEmpty()) {
        emit outputReceived(output);
    }
    if (!error.isEmpty()) {
        emit errorReceived(error);
    }
    
    emit operationFinished(args.join(' '), success);
    
    return success;
}

QString GitService::executeGitCommandSimple(const QStringList& args) {
    QString output, error;
    executeGitCommand(args, output, error);
    return output;
}

bool GitService::isGitInstalled() {
    static bool checked = false;
    static bool installed = false;
    
    if (!checked) {
        QProcess process;
        process.start("git", {"--version"});
        process.waitForFinished(3000);
        installed = (process.exitCode() == 0);
        checked = true;
    }
    
    return installed;
}
