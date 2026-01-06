#include "FeatureBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include "widgets/MrZone.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QFrame>

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
    
    bool success = m_gitService->commit(commitMsg);
    
    if (success) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("代码已提交到本地仓库"));
        updateFileList();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
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
    
    m_pushButton->setEnabled(false);
    m_pushButton->setText(QString::fromUtf8("推送中..."));
    
    bool success = m_gitService->pushBranch(currentBranch, true);
    
    m_pushButton->setEnabled(true);
    m_pushButton->setText(QString::fromUtf8("⬆️ 推送"));
    
    if (success) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("代码已推送到远程仓库"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
            QString::fromUtf8("推送失败，请检查网络连接和权限"));
    }
}

void FeatureBranchView::onConflictCheckRequested(const QString& targetBranch) {
    QString conflictInfo;
    bool hasNoConflict = m_gitService->checkMergeConflict(targetBranch, conflictInfo);
    
    if (hasNoConflict) {
        QMessageBox::information(this, QString::fromUtf8("检查结果"),
            conflictInfo + QString::fromUtf8("\n\n可以继续发起MR。"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("检查结果"), conflictInfo);
    }
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
    
    // 连接API信号（一次性连接）
    connect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, 
        [this](const MrResponse& mr) {
            QString message = QString::fromUtf8(
                "✅ MR创建成功！\n\n"
                "MR #%1: %2\n"
                "URL: %3\n"
                "状态: %4"
            ).arg(mr.iid).arg(mr.title, mr.webUrl, mr.state);
            
            QMessageBox::information(this, QString::fromUtf8("成功"), message);
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this](const QString& endpoint, const QString& errorMessage) {
            QMessageBox::warning(this, QString::fromUtf8("失败"),
                QString::fromUtf8("创建MR失败：\n\n%1\n\n请检查：\n"
                                 "1. GitLab Token权限\n"
                                 "2. 项目ID是否正确\n"
                                 "3. 网络连接").arg(errorMessage));
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    m_gitLabApi->createMergeRequest(params);
}