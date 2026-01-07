#ifndef GITLABAPI_H
#define GITLABAPI_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "ApiModels.h"

/**
 * @brief GitLab API客户端
 * 使用Qt Network模块实现RESTful API调用
 */
class GitLabApi : public QObject {
    Q_OBJECT
    
public:
    explicit GitLabApi(QObject* parent = nullptr);
    ~GitLabApi();
    
    // 配置
    void setBaseUrl(const QString& url);
    void setApiToken(const QString& token);
    void setProjectId(const QString& projectId);
    
    // 用户API
    void getCurrentUser(); // 获取当前用户信息（用于测试连接）
    
    // 项目API
    void getProjects();    // 获取用户有权限的项目列表
    void getProject(const QString& projectId);
    
    // MR API
    void createMergeRequest(const MrParams& params);
    void getMergeRequest(int mrIid);
    void listMergeRequests(int page = 1, int perPage = 20);
    
    // Pipeline API
    void triggerPipeline(const QString& ref);
    void getPipelineStatus(int pipelineId);
    void listPipelines(const QString& ref = QString());
    
    // Job API
    void getJobLog(int jobId);
    void getJobArtifacts(int jobId);
    
signals:
    // 成功信号
    void userInfoReceived(const UserInfo& user);
    void projectsReceived(const QList<ProjectInfo>& projects);
    void projectReceived(const ProjectInfo& project);
    void mergeRequestCreated(const MrResponse& mr);
    void mergeRequestReceived(const MrResponse& mr);
    void mergeRequestsReceived(const QList<MrResponse>& mrs);
    void pipelineTriggered(const PipelineStatus& pipeline);
    void pipelineStatusReceived(const PipelineStatus& status);
    void pipelinesReceived(const QList<PipelineStatus>& pipelines);
    void jobLogReceived(int jobId, const QString& log);
    void jobArtifactsReceived(int jobId, const QList<BuildArtifact>& artifacts);
    
    // 错误信号
    void apiError(const QString& endpoint, const QString& errorMessage);
    void networkError(const QString& errorMessage);
    
private slots:
    void onReplyFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* m_networkManager;
    QString m_baseUrl;      // 如: https://gitlab.example.com
    QString m_apiToken;
    QString m_projectId;
    
    // HTTP请求方法
    void sendGetRequest(const QString& endpoint, const QString& callbackId);
    void sendPostRequest(const QString& endpoint, const QJsonObject& data, const QString& callbackId);
    void sendPutRequest(const QString& endpoint, const QJsonObject& data, const QString& callbackId);
    
    // 响应处理
    void handleUserInfoResponse(const QJsonObject& json);
    void handleProjectsResponse(const QJsonArray& jsonArray);
    void handleProjectResponse(const QJsonObject& json);
    void handleMergeRequestResponse(const QJsonObject& json, bool isCreate);
    void handleMergeRequestsResponse(const QJsonArray& jsonArray);
    void handlePipelineResponse(const QJsonObject& json);
    void handlePipelinesResponse(const QJsonArray& jsonArray);
    void handleJobLogResponse(int jobId, const QString& log);
    
    // 数据解析
    UserInfo parseUserInfo(const QJsonObject& json);
    ProjectInfo parseProjectInfo(const QJsonObject& json);
    MrResponse parseMergeRequest(const QJsonObject& json);
    PipelineStatus parsePipeline(const QJsonObject& json);
    
    // 辅助方法
    QNetworkRequest createRequest(const QString& endpoint);
    QString buildApiUrl(const QString& endpoint);
};

#endif // GITLABAPI_H
