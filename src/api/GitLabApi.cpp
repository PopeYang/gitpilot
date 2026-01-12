#include "GitLabApi.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDateTime>
#include <QMessageBox>

GitLabApi::GitLabApi(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &GitLabApi::onReplyFinished);
}

GitLabApi::~GitLabApi() {
}

void GitLabApi::setBaseUrl(const QString& url) {
    m_baseUrl = url;
    // 移除末尾的斜杠
    if (m_baseUrl.endsWith('/')) {
        m_baseUrl.chop(1);
    }
}

void GitLabApi::setApiToken(const QString& token) {
    m_apiToken = token;
}

void GitLabApi::setProjectId(const QString& projectId) {
    m_projectId = projectId;
}

// ========== 用户API ==========

void GitLabApi::getCurrentUser() {
    LOG_INFO("API调用: 获取当前用户信息");
    sendGetRequest("/api/v4/user", "getCurrentUser");
}

// ========== 项目API ==========

void GitLabApi::getProjects() {
    LOG_INFO("API调用: 获取项目列表");
    sendGetRequest("/api/v4/projects?membership=true&per_page=100", "getProjects");
}

void GitLabApi::getProject(const QString& projectId) {
    LOG_INFO(QString("API调用: 获取项目信息 ID=%1").arg(projectId));
    QString endpoint = QString("/api/v4/projects/%1").arg(projectId);
    sendGetRequest(endpoint, "getProject");
}

// ========== MR API ==========

void GitLabApi::createMergeRequest(const MrParams& params) {
    LOG_INFO(QString("API调用: 创建MR %1 -> %2")
             .arg(params.sourceBranch, params.targetBranch));
    
    if (m_projectId.isEmpty()) {
        LOG_ERROR("项目ID未设置，无法创建MR");
        emit apiError("createMergeRequest", QString::fromUtf8("项目ID未设置"));
        return;
    }
    
    QJsonObject json;
    json["source_branch"] = params.sourceBranch;
    json["target_branch"] = params.targetBranch;
    json["title"] = params.title;
    
    if (!params.description.isEmpty()) {
        json["description"] = params.description;
    }
    
    json["remove_source_branch"] = params.removeSourceBranch;
    json["squash"] = params.squash;
    
    LOG_INFO(QString("MR JSON: %1").arg(QString(QJsonDocument(json).toJson())));
    
    // URL编码项目ID（如果是路径格式 yanghaozhe/test -> yanghaozhe%2Ftest）
    QString encodedProjectId = QString(QUrl::toPercentEncoding(m_projectId));
    QString endpoint = QString("/api/v4/projects/%1/merge_requests").arg(encodedProjectId);
    sendPostRequest(endpoint, json, "createMergeRequest");
}

void GitLabApi::getMergeRequest(int mrIid) {
    QString endpoint = QString("/api/v4/projects/%1/merge_requests/%2")
                      .arg(m_projectId).arg(mrIid);
    sendGetRequest(endpoint, "getMergeRequest");
}

void GitLabApi::listMergeRequests(int page, int perPage, const QString& state, const QString& targetBranch) {
    QString endpoint = QString("/api/v4/projects/%1/merge_requests?page=%2&per_page=%3")
                      .arg(m_projectId).arg(page).arg(perPage);
    
    if (!state.isEmpty()) {
        endpoint += QString("&state=%1").arg(state);
    }
    
    if (!targetBranch.isEmpty()) {
        endpoint += QString("&target_branch=%1").arg(targetBranch);
    }
                      
    sendGetRequest(endpoint, "listMergeRequests");
}

// ========== Pipeline API ==========

void GitLabApi::triggerPipeline(const QString& ref) {
    LOG_INFO(QString("API调用: 触发Pipeline ref=%1").arg(ref));
    
    QJsonObject json;
    json["ref"] = ref;
    
    QString endpoint = QString("/api/v4/projects/%1/pipeline").arg(m_projectId);
    sendPostRequest(endpoint, json, "triggerPipeline");
}

void GitLabApi::getPipelineStatus(int pipelineId) {
    QString endpoint = QString("/api/v4/projects/%1/pipelines/%2")
                      .arg(m_projectId).arg(pipelineId);
    sendGetRequest(endpoint, "getPipelineStatus");
}

void GitLabApi::listPipelines(const QString& ref) {
    QString endpoint = QString("/api/v4/projects/%1/pipelines").arg(m_projectId);
    if (!ref.isEmpty()) {
        endpoint += QString("?ref=%1").arg(ref);
    }
    sendGetRequest(endpoint, "listPipelines");
}

// ========== Job API ==========

void GitLabApi::getJobLog(int jobId) {
    QString endpoint = QString("/api/v4/projects/%1/jobs/%2/trace")
                      .arg(m_projectId).arg(jobId);
    sendGetRequest(endpoint, QString("getJobLog:%1").arg(jobId));
}

void GitLabApi::getJobArtifacts(int jobId) {
    QString endpoint = QString("/api/v4/projects/%1/jobs/%2/artifacts")
                      .arg(m_projectId).arg(jobId);
    sendGetRequest(endpoint, QString("getJobArtifacts:%1").arg(jobId));
}

// ========== HTTP请求方法 ==========

void GitLabApi::sendGetRequest(const QString& endpoint, const QString& callbackId) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = m_networkManager->get(request);
    reply->setProperty("callbackId", callbackId);
}

void GitLabApi::sendPostRequest(const QString& endpoint, const QJsonObject& data, const QString& callbackId) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(data).toJson());
    reply->setProperty("callbackId", callbackId);
    
    // 如果是创建MR的请求，设置标记
    if (callbackId == "createMergeRequest") {
        reply->setProperty("isCreate", true);
    }
}

void GitLabApi::sendPutRequest(const QString& endpoint, const QJsonObject& data, const QString& callbackId) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = m_networkManager->put(request, QJsonDocument(data).toJson());
    reply->setProperty("callbackId", callbackId);
}

QNetworkRequest GitLabApi::createRequest(const QString& endpoint) {
    QNetworkRequest request;
    request.setUrl(QUrl(buildApiUrl(endpoint)));
    request.setRawHeader("PRIVATE-TOKEN", m_apiToken.toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    return request;
}

QString GitLabApi::buildApiUrl(const QString& endpoint) {
    return m_baseUrl + endpoint;
}

// ========== 响应处理 ==========

void GitLabApi::onReplyFinished(QNetworkReply* reply) {
    QString callbackId = reply->property("callbackId").toString();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    LOG_INFO(QString("API响应: %1, HTTP %2").arg(callbackId).arg(statusCode));
    
    // 关键修复：即使有网络错误，如果HTTP状态码有效（200-599），也应该处理响应
    // 因为像409这样的HTTP错误码是有效的业务逻辑错误，不是网络故障
    bool hasValidHttpStatus = (statusCode >= 200 && statusCode < 600);
    
    if (reply->error() != QNetworkReply::NoError && !hasValidHttpStatus) {
        // 真正的网络错误（连接失败、超时等）
        QString errorMsg = reply->errorString();
        QString detailedError = QString("API Error [%1]: %2\nHTTP Status: %3")
            .arg(callbackId)
            .arg(errorMsg)
            .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        
        LOG_ERROR(detailedError);
        
        // 读取响应体以获取更详细的错误信息
        QByteArray responseData = reply->readAll();
        if (!responseData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("message")) {
                    detailedError += "\nGitLab Message: " + obj["message"].toString();
                }
            }
        }
        
        emit apiError(callbackId, detailedError);
        emit networkError(errorMsg);
        reply->deleteLater();
        return;
    }
    
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    // 检查业务逻辑错误（HTTP 4xx/5xx）
    if (statusCode >= 400) {
        QString errorMsg = QString("HTTP %1").arg(statusCode);
        QString detailedError;
        
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            // GitLab错误信息通常在 "message" 或 "error" 字段
            if (obj.contains("message")) {
                if (obj["message"].isArray()) {
                    QJsonArray msgs = obj["message"].toArray();
                    QStringList msgList;
                    for (const auto& m : msgs) msgList << m.toString();
                    detailedError = msgList.join("; ");
                } else {
                    detailedError = obj["message"].toString();
                }
            } else if (obj.contains("error")) {
                detailedError = obj["error"].toString();
            }
        }
        
        if (detailedError.isEmpty()) {
            detailedError = errorMsg;
        } else {
            detailedError = QString("%1: %2").arg(errorMsg, detailedError);
        }
        
        LOG_ERROR(QString("API业务错误 [%1]: %2").arg(callbackId, detailedError));
        emit apiError(callbackId, detailedError);
        reply->deleteLater();
        return;
    }
    
    if (!doc.isNull()) {
        
        // 根据callbackId分发处理
        if (callbackId == "getCurrentUser") {
            handleUserInfoResponse(doc.object());
        }
        else if (callbackId == "getProjects") {
            handleProjectsResponse(doc.array());
        }
        else if (callbackId == "getProject") {
            handleProjectResponse(doc.object());
        }
        else if (callbackId == "createMergeRequest") {
            // 提取isCreate属性并传递给handler
            bool isCreate = reply->property("isCreate").toBool();
            handleMergeRequestResponse(doc.object(), isCreate);
        }
        else if (callbackId == "getMergeRequest") {
            handleMergeRequestResponse(doc.object(), false);  // 查询操作，不是创建
        }
        else if (callbackId == "listMergeRequests") {
            handleMergeRequestsResponse(doc.array());
        }
        else if (callbackId == "triggerPipeline" || callbackId == "getPipelineStatus") {
            handlePipelineResponse(doc.object(), (callbackId == "triggerPipeline"));
        }
        else if (callbackId == "listPipelines") {
            handlePipelinesResponse(doc.array());
        }
        else if (callbackId.startsWith("getJobLog:")) {
            int jobId = callbackId.split(':')[1].toInt();
            handleJobLogResponse(jobId, QString::fromUtf8(responseData));
        }
    }
    
    reply->deleteLater();
}

void GitLabApi::handleUserInfoResponse(const QJsonObject& json) {
    UserInfo user = parseUserInfo(json);
    emit userInfoReceived(user);
}

void GitLabApi::handleProjectsResponse(const QJsonArray& jsonArray) {
    QList<ProjectInfo> projects;
    for (const QJsonValue& val : jsonArray) {
        projects.append(parseProjectInfo(val.toObject()));
    }
    emit projectsReceived(projects);
}

void GitLabApi::handleProjectResponse(const QJsonObject& json) {
    ProjectInfo project = parseProjectInfo(json);
    emit projectReceived(project);
}

void GitLabApi::handleMergeRequestResponse(const QJsonObject& json, bool isCreate) {
    MrResponse mr = parseMergeRequest(json);
    
    LOG_INFO(QString("handleMergeRequestResponse: isCreate=%1, iid=%2").arg(isCreate).arg(mr.iid));
    
    if (isCreate) {
        LOG_INFO("Emitting mergeRequestCreated signal");
        emit mergeRequestCreated(mr);
    } else {
        LOG_INFO("Emitting mergeRequestReceived signal");
        emit mergeRequestReceived(mr);
    }
}

void GitLabApi::handleMergeRequestsResponse(const QJsonArray& jsonArray) {
    QList<MrResponse> mrs;
    for (const QJsonValue& val : jsonArray) {
        mrs.append(parseMergeRequest(val.toObject()));
    }
    emit mergeRequestsReceived(mrs);
}

void GitLabApi::handlePipelineResponse(const QJsonObject& json, bool isTrigger) {
    PipelineStatus pipeline = parsePipeline(json);
    
    if (isTrigger) {
        emit pipelineTriggered(pipeline);
    } else {
        emit pipelineStatusReceived(pipeline);
    }
}

void GitLabApi::handlePipelinesResponse(const QJsonArray& jsonArray) {
    QList<PipelineStatus> pipelines;
    for (const QJsonValue& val : jsonArray) {
        pipelines.append(parsePipeline(val.toObject()));
    }
    emit pipelinesReceived(pipelines);
}

void GitLabApi::handleJobLogResponse(int jobId, const QString& log) {
    emit jobLogReceived(jobId, log);
}

// ========== 数据解析 ==========

UserInfo GitLabApi::parseUserInfo(const QJsonObject& json) {
    UserInfo user;
    user.id = json["id"].toInt();
    user.username = json["username"].toString();
    user.name = json["name"].toString();
    user.email = json["email"].toString();
    return user;
}

ProjectInfo GitLabApi::parseProjectInfo(const QJsonObject& json) {
    ProjectInfo project;
    project.id = json["id"].toInt();
    project.name = json["name"].toString();
    project.pathWithNamespace = json["path_with_namespace"].toString();
    project.description = json["description"].toString();
    project.webUrl = json["web_url"].toString();
    return project;
}

MrResponse GitLabApi::parseMergeRequest(const QJsonObject& json) {
    MrResponse mr;
    mr.id = json["id"].toInt();
    mr.iid = json["iid"].toInt();
    mr.title = json["title"].toString();
    mr.webUrl = json["web_url"].toString();
    mr.state = json["state"].toString();
    mr.createdAt = json["created_at"].toString();
    return mr;
}

PipelineStatus GitLabApi::parsePipeline(const QJsonObject& json) {
    PipelineStatus pipeline;
    pipeline.id = json["id"].toInt();
    pipeline.status = json["status"].toString();
    pipeline.ref = json["ref"].toString();
    pipeline.webUrl = json["web_url"].toString();
    
    QString createdAt = json["created_at"].toString();
    pipeline.createdAt = QDateTime::fromString(createdAt, Qt::ISODate);
    
    QString updatedAt = json["updated_at"].toString();
    pipeline.updatedAt = QDateTime::fromString(updatedAt, Qt::ISODate);
    
    return pipeline;
}
