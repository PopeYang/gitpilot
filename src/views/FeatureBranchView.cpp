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
    
    // 标题
    QLabel* titleLabel = new QLabel(QString::fromUtf8("🟢 开发分支 - 活跃工作区"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // 分割线
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // 修改文件列表
    QGroupBox* filesGroup = new QGroupBox(QString::fromUtf8("📝 待提交的修改"), this);
    QVBoxLayout* filesLayout = new QVBoxLayout(filesGroup);
    
    m_filesListWidget = new QListWidget(this);
    m_filesListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_filesListWidget->setMaximumHeight(200);
    filesLayout->addWidget(m_filesListWidget);
    
    // 按钮区域 - 简化布局
    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(QString::fromUtf8("🔄 刷新状态"), this);
    m_stageAllButton = new QPushButton(QString::fromUtf8("✅ 暂存全部"), this);
    m_commitButton = new QPushButton(QString::fromUtf8("💾 本地提交"), this);
    m_pushButton = new QPushButton(QString::fromUtf8("⬆️ 推送远端"), this);
    
    // 设置按钮样式
    m_commitButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 5px 15px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    );
    
    m_pushButton->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 5px 15px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #e68900; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    );
    
    buttonsLayout->addWidget(m_refreshButton);
    buttonsLayout->addWidget(m_stageAllButton);
    buttonsLayout->addWidget(m_commitButton);
    buttonsLayout->addWidget(m_pushButton);
    buttonsLayout->addStretch();
    
    filesLayout->addLayout(buttonsLayout);
    
    mainLayout->addWidget(filesGroup);
    
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
    
    // 显示等待动画，宽度255
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在创建合并请求..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("提交中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);  // 不可取消
    progress->setValue(0);
    progress->show();
    QApplication::processEvents();  // 立即显示进度条
    
    // 连接API信号（一次性连接）
    connect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, 
        [this, progress](const MrResponse& mr) {
            // 关闭进度对话框
            progress->close();
            progress->deleteLater();
            
            // 创建富文本消息
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
            msgBox.setIcon(QMessageBox::NoIcon);  // 不使用默认图标，标题中已有emoji
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setMinimumWidth(255);
            
            // 让链接可以打开
            msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
            
            msgBox.exec();
            
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this, progress](const QString& endpoint, const QString& errorMessage) {
            // 关闭进度对话框
            progress->close();
            progress->deleteLater();
            
            QString userMessage;
            
            // 检查是否是409冲突错误
            if (errorMessage.contains("409")) {
                userMessage = QString::fromUtf8(
                    "⚠️ MR已存在\n\n"
                    "该分支的MR可能已经创建过了。\n\n"
                    "请前往GitLab检查是否已有相同的MR：\n"
                    "源分支 → 目标分支\n\n"
                    "详细错误：\n%1"
                ).arg(errorMessage);
            } else if (errorMessage.contains("401") || errorMessage.contains("403")) {
                userMessage = QString::fromUtf8(
                    "🔒 权限错误\n\n"
                    "GitLab Token可能无效或权限不足。\n\n"
                    "请检查：\n"
                    "1. Token是否已过期\n"
                    "2. Token是否有api和write_repository权限\n"
                    "3. 是否有项目的开发者权限\n\n"
                    "详细错误：\n%1"
                ).arg(errorMessage);
            } else if (errorMessage.contains("404")) {
                userMessage = QString::fromUtf8(
                    "❓ 未找到资源\n\n"
                    "项目ID可能不正确，或分支不存在。\n\n"
                    "请检查：\n"
                    "1. 设置中的项目ID是否正确\n"
                    "2. 代码是否已推送到远程\n\n"
                    "详细错误：\n%1"
                ).arg(errorMessage);
            } else {
                userMessage = QString::fromUtf8(
                    "❌ 创建MR失败\n\n"
                    "%1\n\n"
                    "请检查：\n"
                    "1. GitLab Token权限\n"
                    "2. 项目ID是否正确\n"
                    "3. 网络连接"
                ).arg(errorMessage);
            }
            
            QMessageBox::warning(this, QString::fromUtf8("失败"), userMessage);
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    m_gitLabApi->createMergeRequest(params);
}