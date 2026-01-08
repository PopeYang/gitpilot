#include "FeatureBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include "api/ApiModels.h"
#include "widgets/MrZone.h"
#include "widgets/ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QTimer>
#include <QFrame>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

FeatureBranchView::FeatureBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
{
    setupUi();
    connectSignals();
}

void FeatureBranchView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 顶部友好提示区域 - 绿色主题
    QGroupBox* welcomeGroup = new QGroupBox(QString::fromUtf8("🟢 开发分支 - 活跃工作区"), this);
    welcomeGroup->setStyleSheet(
        "QGroupBox {"
        "   background-color: #E8F5E9;"  // 浅绿色背景
        "   border: 1px solid #4CAF50;"  // 绿色边框
        "   border-radius: 5px;"
        "   margin-top: 10px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   color: #2E7D32;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 5px;"
        "   left: 10px;"
        "}"
    );
    
    QVBoxLayout* welcomeLayout = new QVBoxLayout(welcomeGroup);
    QLabel* welcomeLabel = new QLabel(
        QString::fromUtf8("• 尽情挥洒创意，代码改动无负担\n"
                         "• 随时本地提交，保护灵感的火花\n"
                         "• 困难及时求助，团队就在你身边"), 
        this);
    welcomeLabel->setStyleSheet("color: #2E7D32; font-size: 13px; background: transparent; border: none;");
    welcomeLabel->setWordWrap(true);
    welcomeLayout->addWidget(welcomeLabel);
    
    mainLayout->addWidget(welcomeGroup);
    
    // 修改文件列表
    QGroupBox* filesGroup = new QGroupBox(QString::fromUtf8("📝 待提交的修改"), this);
    QVBoxLayout* filesLayout = new QVBoxLayout(filesGroup);
    
    m_filesListWidget = new QListWidget(this);
    m_filesListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_filesListWidget->setMaximumHeight(200);
    filesLayout->addWidget(m_filesListWidget);
    
    // 按钮区域 - 采用Main/Database一致的布局
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(QString::fromUtf8("🔄 刷新状态"), this);
    m_stageAllButton = new QPushButton(QString::fromUtf8("✅ 暂存全部"), this);
    
    // 刷新和暂存采用白色样式
    m_refreshButton->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333;"
        "   border: 1px solid #ccc;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #f5f5f5;"
        "}"
    );
    
    m_stageAllButton->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333;"
        "   border: 1px solid #ccc;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #f5f5f5;"
        "}"
    );
    
    buttonsLayout->addWidget(m_refreshButton);
    buttonsLayout->addWidget(m_stageAllButton);
    filesLayout->addLayout(buttonsLayout);
    
    mainLayout->addWidget(filesGroup);

    // 提交操作区域
    QGroupBox* commitGroup = new QGroupBox(QString::fromUtf8("📝 提交操作"), this);
    commitGroup->setStyleSheet("QGroupBox { font-size: 13px; font-weight: bold; padding: 10px; }");
    
    QVBoxLayout* commitLayout = new QVBoxLayout(commitGroup);
    
    QHBoxLayout* commitButtonsLayout = new QHBoxLayout();
    
    m_commitButton = new QPushButton(QString::fromUtf8("📝 本地提交"), this);
    m_pushButton = new QPushButton(QString::fromUtf8("🚀 上传推送"), this);
    
    // 提交按钮 - 蓝色
    m_commitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0D47A1;"
        "}"
    );
    
    // 推送按钮 - 橙色
    m_pushButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #E65100;"
        "}"
    );
    
    commitButtonsLayout->addWidget(m_commitButton);
    commitButtonsLayout->addWidget(m_pushButton);
    commitLayout->addLayout(commitButtonsLayout);
    
    mainLayout->addWidget(commitGroup);
    
    // MR提交专区
    m_mrZone = new MrZone(m_gitService, m_gitLabApi, this);
    mainLayout->addWidget(m_mrZone);
    
    mainLayout->addStretch();
    
    // 设置背景色
    setStyleSheet("FeatureBranchView { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F0FFF0, stop:1 #E0FFE0); }");
}

void FeatureBranchView::connectSignals() {
    connect(m_refreshButton, &QPushButton::clicked, this, &FeatureBranchView::onRefreshClicked);
    connect(m_stageAllButton, &QPushButton::clicked, this, &FeatureBranchView::onStageAllClicked);
    connect(m_commitButton, &QPushButton::clicked, this, &FeatureBranchView::onCommitClicked);
    connect(m_pushButton, &QPushButton::clicked, this, &FeatureBranchView::onPushClicked);
    connect(m_mrZone, &MrZone::conflictCheckRequested, this, &FeatureBranchView::onConflictCheckRequested);
    connect(m_mrZone, &MrZone::mrSubmitted, this, &FeatureBranchView::onMrSubmitted);
}

void FeatureBranchView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // 视图显示时自动刷新
    updateFileList();
    updateMrZone();
}

void FeatureBranchView::updateFileList() {
    m_filesListWidget->clear();
    
    QList<FileStatus> fileStatuses = m_gitService->getFileStatus();
    
    if (fileStatuses.isEmpty()) {
        m_filesListWidget->addItem(QString::fromUtf8("✓ 没有待提交的修改"));
        m_stageAllButton->setEnabled(false);
    } else {
        for (const FileStatus& status : fileStatuses) {
            QListWidgetItem* item = new QListWidgetItem(status.displayText);
            item->setData(Qt::UserRole, status.filename);  // 存储原始文件名
            m_filesListWidget->addItem(item);
        }
        m_stageAllButton->setEnabled(true);
    }
}

void FeatureBranchView::updateMrZone() {
    QString currentBranch = m_gitService->getCurrentBranch();
    m_mrZone->updateForBranch(currentBranch);
}

void FeatureBranchView::onRefreshClicked() {
    updateFileList();
    QMessageBox::information(this, QString::fromUtf8("刷新状态"), 
        QString::fromUtf8("已刷新文件列表"));
}

void FeatureBranchView::onStageAllClicked() {
    bool success = m_gitService->stageAll();
    
    if (success) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("已暂存所有修改"));
        updateFileList();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
            QString::fromUtf8("暂存失败，请检查Git状态"));
    }
}

void FeatureBranchView::onCommitClicked() {
    bool ok;
    QString commitMsg = QInputDialog::getText(
        this,
        QString::fromUtf8("提交修改"),
        QString::fromUtf8("请输入提交消息："),
        QLineEdit::Normal,
        "",
        &ok
    );
    
    if (!ok || commitMsg.trimmed().isEmpty()) {
        return;
    }
    
    // 静默执行commit，不显示进度对话框
    bool success = m_gitService->commit(commitMsg);
    
    if (success) {
        // 成功后刷新列表，不弹窗
        updateFileList();
    } else {
        // 只在失败时弹窗
        QMessageBox::warning(this, QString::fromUtf8("提交失败"),
            QString::fromUtf8("提交失败，请检查Git状态"));
    }
}

void FeatureBranchView::onPushClicked() {
    QString currentBranch = m_gitService->getCurrentBranch();
    
    int ret = QMessageBox::question(
        this,
        QString::fromUtf8("确认推送"),
        QString::fromUtf8("确认要推送 %1 分支到远程仓库？").arg(currentBranch),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 显示进度对话框，宽度固定255
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在推送到远程仓库..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("推送中"));
    progress->setMinimumWidth(255);  // 设置最小宽度，避免太窄，但允许自适应
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    // 使用FutureWatcher监听异步任务
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress]() {
        bool success = watcher->result();
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("推送成功"));
            msgBox.setText(QString::fromUtf8("✅ 代码已成功推送到远程仓库"));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setMinimumWidth(255); // 设置最小宽度
            msgBox.exec();
            msgBox.exec();
        } else {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("推送失败"));
            msgBox.setText(QString::fromUtf8("推送失败，请检查网络连接和权限"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setMinimumWidth(255); // 设置最小宽度
            msgBox.exec();
            msgBox.exec();
        }
    });
    
    // 在后台线程执行Git操作
    QFuture<bool> future = QtConcurrent::run([this, currentBranch]() {
        return m_gitService->pushBranch(currentBranch, true);
    });
    
    watcher->setFuture(future);
}

void FeatureBranchView::onConflictCheckRequested(const QString& targetBranch) {
    // 显示进度对话框，宽度固定255
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在检查冲突..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("检查中"));
    progress->setMinimumWidth(255);  // 设置最小宽度
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    // 定义结果类型
    typedef QPair<bool, QString> CheckResult;
    QFutureWatcher<CheckResult>* watcher = new QFutureWatcher<CheckResult>(this);
    
    connect(watcher, &QFutureWatcher<CheckResult>::finished, this, [this, watcher, progress, targetBranch]() {
        CheckResult result = watcher->result();
        bool hasNoConflict = result.first;
        QString conflictInfo = result.second;
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (hasNoConflict) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("检查完成"));
            msgBox.setText(QString::fromUtf8("✅ ") + conflictInfo + QString::fromUtf8("\n\n可以继续发起合并请求。"));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setMinimumWidth(255);
            
            // 添加"发起合并"按钮
            QPushButton* mergeBtn = msgBox.addButton(QString::fromUtf8("发起合并"), QMessageBox::AcceptRole);
            msgBox.addButton(QMessageBox::Close);
            
            msgBox.exec();
            
            if (msgBox.clickedButton() == mergeBtn) {
                // 触发MR提交
                if (m_mrZone) {
                    m_mrZone->triggerSubmit();
                }
            }
        } else {
            QString helpText = QString::fromUtf8(
                "\n\n🛠️ 如何解决冲突：\n"
                "1. 在本地终端运行：\n   git pull origin %1\n"
                "2. 打开IDE解决冲突文件\n"
                "3. 提交修改并再次推送"
            ).arg(targetBranch);
            
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("发现冲突"));
            msgBox.setText(conflictInfo + helpText);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setMinimumWidth(255);
            msgBox.exec();
        }
    });
    
    // 在后台线程执行Git操作
    QFuture<CheckResult> future = QtConcurrent::run([this, targetBranch]() {
        QString info;
        // 注意：GitService必须是线程安全的，或者checkMergeConflict不应该访问任何GUI元素
        bool result = m_gitService->checkMergeConflict(targetBranch, info);
        return qMakePair(result, info);
    });
    
    watcher->setFuture(future);
}

void FeatureBranchView::onMrSubmitted(const QString& targetBranch, const QString& title, const QString& description) {
    QString sourceBranch = m_gitService->getCurrentBranch();
    
    // 创建MR参数
    MrParams params;
    params.sourceBranch = sourceBranch;
    params.targetBranch = targetBranch;
    params.title = title;
    params.description = description;
    params.removeSourceBranch = false;
    params.squash = false;
    
    // 显示等待动画
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在推送到远程仓库..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("提交中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);  // 不可取消
    progress->setValue(0);
    progress->show();
    QApplication::processEvents();
    
    // 异步执行Push操作
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress, params, sourceBranch]() {
        bool pushSuccess = watcher->result();
        watcher->deleteLater();
        
        if (!pushSuccess) {
            progress->close();
            progress->deleteLater();
            QMessageBox::warning(this, QString::fromUtf8("推送失败"), 
                QString::fromUtf8("无法推送到远程仓库，请检查网络连接或权限。"));
            return;
        }
        
        // Push成功，开始创建MR
        progress->setLabelText(QString::fromUtf8("正在创建合并请求..."));
        
        // 连接API信号（一次性连接）
        connect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, 
            [this, progress, params, sourceBranch](const MrResponse& mr) {
                progress->close();
                progress->deleteLater();
                
                // 显示MR创建成功
                showMrSuccessDialog(mr);
                
                // 检测是否需要同步（仅针对bugfix分支）
                if (isBugfixBranch(sourceBranch)) {
                    QString syncTarget = (params.targetBranch == "develop") 
                                         ? "internal" : "develop";
                    
                    // 启动异步冲突检测并提示同步
                    checkAndPromptSync(sourceBranch, syncTarget, params.title);
                }
                
                disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
                disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
            });
        
        connect(m_gitLabApi, &GitLabApi::apiError, this,
            [this, progress](const QString& endpoint, const QString& errorMessage) {
                progress->close();
                progress->deleteLater();
                
                QString userMessage;
                
                if (errorMessage.contains("409")) {
                    userMessage = QString::fromUtf8("⚠️ MR已存在\n该分支的MR可能已经创建过了。");
                } else if (errorMessage.contains("401") || errorMessage.contains("403")) {
                    userMessage = QString::fromUtf8("🔒 权限错误\nToken无效或权限不足。");
                } else if (errorMessage.contains("404")) {
                    userMessage = QString::fromUtf8("❓ 未找到资源\n项目ID不正确或远程分支不存在。");
                } else {
                    userMessage = QString::fromUtf8("❌ 创建MR失败\n%1").arg(errorMessage);
                }
                
                QMessageBox::warning(this, QString::fromUtf8("失败"), userMessage);
                disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
                disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
            });
        
        // 发起API调用
        m_gitLabApi->createMergeRequest(params);
    });
    
    // 开始后台Push
    QFuture<bool> future = QtConcurrent::run([this, sourceBranch]() {
        // pushBranch(branch, setUpstream=true)
        return m_gitService->pushBranch(sourceBranch, true);
    });
    
    watcher->setFuture(future);
}// Bugfix 分支同步工作流的辅助函数实现
// 这些函数将被追加到 FeatureBranchView.cpp 的末尾

// 检测是否为 bugfix 分支（支持 bugfix/xxx 命名）
bool FeatureBranchView::isBugfixBranch(const QString& branchName) {
    return branchName.startsWith("bugfix/", Qt::CaseInsensitive) ||
           branchName.startsWith("bugfix-", Qt::CaseInsensitive) ||
           branchName.startsWith("fix/", Qt::CaseInsensitive) ||
           branchName.startsWith("fix-", Qt::CaseInsensitive);
}

// 显示MR创建成功对话框（提取为独立方法）
void FeatureBranchView::showMrSuccessDialog(const MrResponse& mr) {
    QString message = QString(
        "<h3 style='color: green;'>✅ 合并请求创建成功！</h3>"
        "<p><b>编号:</b> %1</p>"
        "<p><b>标题:</b> %2</p>"
        "<p><b>状态:</b> %3</p>"
        "<p><b>链接:</b> ⬇️⬇️⬇️ <br>"
        "<a href='%4'>%4</a></p>"
        "<p style='color: #666; font-size: 11px;'>💡 点击链接在浏览器中查看合并请求详情</p>"
    ).arg(mr.iid).arg(mr.title, mr.state, mr.webUrl);
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("合并请求创建成功"));
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::NoIcon);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setMinimumWidth(255);
    msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
    msgBox.exec();
}

// 异步检测冲突并提示用户同步
void FeatureBranchView::checkAndPromptSync(
    const QString& sourceBranch,
    const QString& targetBranch,
    const QString& originalTitle) {
    
    // 创建进度对话框
    QProgressDialog* checkProgress = new QProgressDialog(
        QString::fromUtf8("正在为同步修改检测冲突 ..."), 
        QString(), 0, 0, this);
    checkProgress->setWindowTitle(QString::fromUtf8("冲突检测"));
    checkProgress->setMinimumWidth(255);
    checkProgress->setWindowModality(Qt::WindowModal);
    checkProgress->setCancelButton(nullptr);
    checkProgress->show();
    QApplication::processEvents();
    
    // 异步执行冲突检测
    QFutureWatcher<CherryPickConflictResult>* watcher = 
        new QFutureWatcher<CherryPickConflictResult>(this);
    
    connect(watcher, &QFutureWatcher<CherryPickConflictResult>::finished, 
        this, [this, watcher, checkProgress, sourceBranch, targetBranch, originalTitle]() {
        
        CherryPickConflictResult result = watcher->result();
        watcher->deleteLater();
        checkProgress->close();
        checkProgress->deleteLater();
        
        // 检测失败
        if (!result.errorMessage.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("检测失败"),
                QString::fromUtf8("无法检测冲突：\n%1\n\n"
                                  "是否仍要创建同步MR？")
                    .arg(result.errorMessage));
            // 用户可选择继续或放弃
            return;
        }
        
        // 根据检测结果提示用户
        if (result.hasConflict) {
            promptSyncWithConflict(sourceBranch, targetBranch, 
                                  originalTitle, result.conflictFiles);
        } else {
            promptSyncNoConflict(sourceBranch, targetBranch, originalTitle);
        }
    });
    
    // 启动后台检测
    QFuture<CherryPickConflictResult> future = QtConcurrent::run(
        [this, sourceBranch, targetBranch]() {
            return m_gitService->checkCherryPickConflict(sourceBranch, targetBranch);
        });
    
    watcher->setFuture(future);
}

// 提示无冲突同步
void FeatureBranchView::promptSyncNoConflict(
    const QString& sourceBranch,
    const QString& targetBranch,
    const QString& originalTitle) {
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("同步到另一分支？"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(QString::fromUtf8(
        "✅ <b>未检测到冲突</b><br><br>"
        "是否需要创建同步 MR 到 <b>%1</b> 分支？<br><br>"
        "这可以确保两条主线的修复一致性。")
        .arg(targetBranch));
    
    QPushButton* yesBtn = msgBox.addButton(
        QString::fromUtf8("立即创建"), QMessageBox::YesRole);
    msgBox.addButton(QString::fromUtf8("稍后手动"), QMessageBox::NoRole);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == yesBtn) {
        createSyncMergeRequest(sourceBranch, targetBranch, originalTitle, false);
    }
}

// 提示有冲突同步
void FeatureBranchView::promptSyncWithConflict(
    const QString& sourceBranch,
    const QString& targetBranch,
    const QString& originalTitle,
    const QStringList& conflictFiles) {
    
    QString fileList = conflictFiles.join("\n• ");
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("检测到潜在冲突"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(QString::fromUtf8(
        "⚠️ <b>检测到 Cherry-pick 冲突</b><br><br>"
        "同步到 <b>%1</b> 时可能存在以下冲突文件：<br><br>"
        "<span style='font-family:monospace;color:#d32f2f;'>• %2</span><br><br>"
        "是否仍要创建同步 MR？<br>"
        "<span style='color:#666;font-size:11px;'>"
        "（合并时需要手动解决冲突）</span>")
        .arg(targetBranch, fileList));
    
    QPushButton* yesBtn = msgBox.addButton(
        QString::fromUtf8("仍要创建"), QMessageBox::YesRole);
    msgBox.addButton(QString::fromUtf8("取消"), QMessageBox::NoRole);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == yesBtn) {
        createSyncMergeRequest(sourceBranch, targetBranch, originalTitle, true);
    }
}

// 创建同步MR（带冲突标记）
void FeatureBranchView::createSyncMergeRequest(
    const QString& sourceBranch,
    const QString& targetBranch,
    const QString& originalTitle,
    bool hasConflict) {
    
    MrParams syncParams;
    syncParams.sourceBranch = sourceBranch;
    syncParams.targetBranch = targetBranch;
    
    // 标题带标记
    if (hasConflict) {
        syncParams.title = QString("[同步⚠️冲突] %1").arg(originalTitle);
    } else {
        syncParams.title = QString("[同步] %1").arg(originalTitle);
    }
    
    // 描述包含冲突信息
    QString oppositeBranch = (targetBranch == "develop") ? "internal" : "develop";
    syncParams.description = QString::fromUtf8(
        "🔄 这是从另一分支同步的 bugfix\n\n"
        "原始 MR 已合并到 %1\n"
        "此 MR 用于保持分支一致性\n\n")
        .arg(oppositeBranch);
    
    if (hasConflict) {
        syncParams.description += QString::fromUtf8(
            "⚠️ **注意**：检测到潜在冲突\n"
            "合并时可能需要手动解决\n");
    }
    
    syncParams.removeSourceBranch = false;
    syncParams.squash = false;
    
    // 创建进度条
    QProgressDialog* syncProgress = new QProgressDialog(
        QString::fromUtf8("正在创建同步合并请求..."), 
        QString(), 0, 0, this);
    syncProgress->setWindowTitle(QString::fromUtf8("提交中"));
    syncProgress->setMinimumWidth(255);
    syncProgress->setWindowModality(Qt::WindowModal);
    syncProgress->setCancelButton(nullptr);
    syncProgress->show();
    QApplication::processEvents();
    
    // 连接API信号
    connect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, 
        [this, syncProgress](const MrResponse& mr) {
            syncProgress->close();
            syncProgress->deleteLater();
            
            QMessageBox::information(this, QString::fromUtf8("同步MR创建成功"),
                QString::fromUtf8("同步合并请求已创建！\n\n"
                                  "编号: #%1\n"
                                  "链接: %2")
                    .arg(mr.iid).arg(mr.webUrl));
            
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this, syncProgress](const QString& endpoint, const QString& errorMessage) {
            syncProgress->close();
            syncProgress->deleteLater();
            
            QMessageBox::warning(this, QString::fromUtf8("创建失败"),
                QString::fromUtf8("同步MR创建失败：\n%1").arg(errorMessage));
            
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    // 发起API调用
    m_gitLabApi->createMergeRequest(syncParams);
}
