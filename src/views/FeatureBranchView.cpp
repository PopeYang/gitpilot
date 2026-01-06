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
    m_filesListWidget->setMaximumHeight(150);
    filesLayout->addWidget(m_filesListWidget);
    
    QHBoxLayout* filesButtonLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_stageAllButton = new QPushButton(QString::fromUtf8("✅ 暂存所有"), this);
    filesButtonLayout->addWidget(m_refreshButton);
    filesButtonLayout->addWidget(m_stageAllButton);
    filesButtonLayout->addStretch();
    filesLayout->addLayout(filesButtonLayout);
    
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
    
    QStringList modifiedFiles = m_gitService->getModifiedFiles();
    
    if (modifiedFiles.isEmpty()) {
        m_filesListWidget->addItem(QString::fromUtf8("✓ 没有待提交的修改"));
        m_stageAllButton->setEnabled(false);
    } else {
        for (const QString& file : modifiedFiles) {
            m_filesListWidget->addItem(file);
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
    QMessageBox::information(this, QString::fromUtf8("刷新"), 
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

void FeatureBranchView::onMrSubmitted(const QString& targetBranch, const QString& title, const QString& description) {
    // TODO: 实现完整的自动化工作流
    // 1. 检查本地状态
    // 2. 暂存并提交
    // 3. 推送到远程
    // 4. 创建MR
    // 5. 触发Pipeline
    // 6. 监控构建
    // 7. 获取下载链接
    
    QMessageBox::information(this, QString::fromUtf8("开发中"),
        QString::fromUtf8("MR自动化工作流开发中...\n\n"
                         "将要创建：\n"
                         "标题: %1\n"
                         "目标: %2\n"
                         "描述: %3").arg(title, targetBranch, description));
}