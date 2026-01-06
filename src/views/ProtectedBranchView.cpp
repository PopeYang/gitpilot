#include "ProtectedBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QGroupBox>

ProtectedBranchView::ProtectedBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent) 
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
{
    setupUi();
    connectSignals();
}

void ProtectedBranchView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // 标题
    QLabel* titleLabel = new QLabel(QString::fromUtf8("🔒 保护分支同步模式"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // 说明文字
    QLabel* descLabel = new QLabel(
        QString::fromUtf8("当前分支受保护，禁止直接推送。\n请切出新分支进行开发。"),
        this
    );
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("color: #666; font-size: 12px;");
    mainLayout->addWidget(descLabel);
    
    mainLayout->addSpacing(20);
    
    // 操作按钮组
    QGroupBox* actionsGroup = new QGroupBox(QString::fromUtf8("快速操作"), this);
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsGroup);
    actionsLayout->setSpacing(15);
    
    // 拉取最新代码按钮
    m_pullButton = new QPushButton(QString::fromUtf8("⬇ 拉取最新代码"), this);
    m_pullButton->setMinimumHeight(50);
    m_pullButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );
    actionsLayout->addWidget(m_pullButton);
    
    // 切出新分支按钮
    m_newBranchButton = new QPushButton(QString::fromUtf8("➕ 切出新分支"), this);
    m_newBranchButton->setMinimumHeight(50);
    m_newBranchButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0a6bc2;"
        "}"
    );
    actionsLayout->addWidget(m_newBranchButton);
    
    mainLayout->addWidget(actionsGroup);
    
    // 状态标签
    m_statusLabel = new QLabel(QString::fromUtf8("就绪"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px; padding: 10px;");
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addStretch();
    
    // 设置背景色
    setStyleSheet("ProtectedBranchView { background-color: #F5F5F5; }");
}

void ProtectedBranchView::connectSignals() {
    connect(m_pullButton, &QPushButton::clicked, this, &ProtectedBranchView::onPullClicked);
    connect(m_newBranchButton, &QPushButton::clicked, this, &ProtectedBranchView::onNewBranchClicked);
    
    connect(m_gitService, &GitService::operationStarted, this, &ProtectedBranchView::onOperationStarted);
    connect(m_gitService, &GitService::operationFinished, this, &ProtectedBranchView::onOperationFinished);
}

void ProtectedBranchView::onPullClicked() {
    m_pullButton->setEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("正在拉取最新代码..."));
    
    bool success = m_gitService->pullLatest();
    
    m_pullButton->setEnabled(true);
    
    if (success) {
        QMessageBox::information(this, QString::fromUtf8("成功"), 
            QString::fromUtf8("已成功拉取最新代码！"));
        m_statusLabel->setText(QString::fromUtf8("拉取成功"));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
            QString::fromUtf8("拉取失败，请检查网络连接或仓库状态。"));
        m_statusLabel->setText(QString::fromUtf8("拉取失败"));
    }
}

void ProtectedBranchView::onNewBranchClicked() {
    // 获取当前分支作为基础分支
    QString baseBranch = m_gitService->getCurrentBranch();
    
    // 生成建议的分支名: feature/username-YYYYMMDD
    QString userName = qgetenv("USERNAME");  // Windows环境变量
    if (userName.isEmpty()) {
        userName = "user";
    }
    QString dateStr = QDateTime::currentDateTime().toString("yyyyMMdd");
    QString suggestedName = QString("feature/%1-%2").arg(userName.toLower(), dateStr);
    
    // 弹出输入对话框
    bool ok;
    QString newBranchName = QInputDialog::getText(
        this,
        QString::fromUtf8("创建新分支"),
        QString::fromUtf8("请输入新分支名称：\n\n建议格式：feature/功能名称 或 hotfix/问题描述"),
        QLineEdit::Normal,
        suggestedName,
        &ok
    );
    
    if (!ok || newBranchName.trimmed().isEmpty()) {
        return;
    }
    
    newBranchName = newBranchName.trimmed();
    
    // 创建并切换到新分支
    m_newBranchButton->setEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("正在创建分支..."));
    
    bool success = m_gitService->createBranch(newBranchName, baseBranch);
    
    m_newBranchButton->setEnabled(true);
    
    if (success) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("已创建并切换到新分支：%1\n\n现在可以开始开发了！").arg(newBranchName));
        m_statusLabel->setText(QString::fromUtf8("分支创建成功"));
        
        // 通知主窗口刷新视图
        emit branchChanged();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
            QString::fromUtf8("创建分支失败，可能是分支名已存在或格式不正确。"));
        m_statusLabel->setText(QString::fromUtf8("分支创建失败"));
    }
}

void ProtectedBranchView::onOperationStarted(const QString& operation) {
    m_statusLabel->setText(QString::fromUtf8("正在执行: %1").arg(operation));
}

void ProtectedBranchView::onOperationFinished(const QString& operation, bool success) {
    if (success) {
        m_statusLabel->setText(QString::fromUtf8("完成: %1").arg(operation));
    } else {
        m_statusLabel->setText(QString::fromUtf8("失败: %1").arg(operation));
    }
}