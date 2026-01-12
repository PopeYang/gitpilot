#ifndef APIMODELS_H
#define APIMODELS_H

#include <QString>
#include <QDateTime>
#include <QList>

/**
 * @brief 项目成员信息
 */
struct ProjectMember {
    int id;                  // 用户 ID
    QString username;        // 用户名
    QString name;            // 显示名称
    
    ProjectMember() : id(0) {}
};

/**
 * @brief MR创建参数
 */
struct MrParams {
    QString sourceBranch;       // 源分支
    QString targetBranch;       // 目标分支
    QString title;              // MR标题
    QString description;        // MR描述
    bool removeSourceBranch;    // 合并后删除源分支
    bool squash;                // 是否压缩提交
    QList<int> assigneeIds;     // 指派的审核人ID列表
    
    MrParams()
        : removeSourceBranch(true)
        , squash(false)
    {}
};

/**
 * @brief MR响应数据
 */
struct MrResponse {
    int id;                     // MR ID
    int iid;                    // 项目内的MR编号
    QString title;              // MR标题
    QString webUrl;             // MR网页链接
    QString state;              // 状态: opened/merged/closed
    QString createdAt;
    QString description;        // MR描述
    QString authorName;         // 提交人名称
    
    MrResponse() : id(0), iid(0) {}
};

/**
 * @brief Pipeline状态
 */
struct PipelineStatus {
    int id;                     // Pipeline ID
    QString status;             // running/pending/success/failed/canceled
    QString ref;                // 分支名
    QString webUrl;             // Pipeline网页链接
    QDateTime createdAt;
    QDateTime updatedAt;
    
    bool isPending() const { return status == "pending"; }
    bool isRunning() const { return status == "running"; }
    bool isSuccess() const { return status == "success"; }
    bool isFailed() const { return status == "failed"; }
    
    PipelineStatus() : id(0) {}
};

/**
 * @brief 构建产物信息
 */
struct BuildArtifact {
    QString filename;           // 文件名
    QString downloadUrl;        // 下载链接
    qint64 size;                // 文件大小(字节)
    
    BuildArtifact() : size(0) {}
};

/**
 * @brief 项目信息
 */
struct ProjectInfo {
    int id;                     // 项目ID
    QString name;               // 项目名
    QString pathWithNamespace; // 完整路径(group/project)
    QString description;        // 描述
    QString webUrl;             // 网页链接
    
    ProjectInfo() : id(0) {}
};

/**
 * @brief 用户信息
 */
struct UserInfo {
    int id;
    QString username;
    QString name;
    QString email;
    
    UserInfo() : id(0) {}
};

#endif // APIMODELS_H
