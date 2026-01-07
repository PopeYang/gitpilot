#include "ProtectedBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include "widgets/BranchCreatorDialog.h"
#include "widgets/ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QGroupBox>
#include <QListWidget>
#include <QTimer>
#include <QProgressDialog>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QListWidget>
#include <QTimer>

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
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 顶部说明区域
    QGroupBox* infoGroup = new QGroupBox(QString::fromUtf8("🔒 保护分支开发模式"), this);
    infoGroup->setStyleSheet(
        "QGroupBox {"
        "   background-color: #F0F8FF;"
        "   border: 2px solid #4A90E2;"
        "   border-radius: 8px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
        "QGroupBox::title {"
        "   color: #2B5278;"
        "}"
    );
    
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    QLabel* descLabel = new QLabel(
        QString::fromUtf8("⚠️ 当前分支受保护，禁止直接推送\n\n"
                         "• 仅可拉取最新代码\n"
                         "• 请切出新分支进行开发"),
        this);
    descLabel->setStyleSheet("color: #2B5278; font-size: 13px; background: transparent; border: none;");
    descLabel->setWordWrap(true);
    infoLayout->addWidget(descLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // 操作按钮组
    QGroupBox* actionsGroup = new QGroupBox(QString::fromUtf8("🔄 操作区"), this);
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
    
    // 新建分支按钮
    m_newBranchButton = new QPushButton(QString::fromUtf8("➕ 新建分支"), this);
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
    
    // 切换分支按钮
    m_switchBranchButton = new QPushButton(QString::fromUtf8("🔀 切换分支"), this);
    m_switchBranchButton->setMinimumHeight(50);
    m_switchBranchButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #9E9E9E;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #757575;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #616161;"
        "}"
    );
    actionsLayout->addWidget(m_switchBranchButton);
    
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
    connect(m_switchBranchButton, &QPushButton::clicked, this, &ProtectedBranchView::onSwitchBranchClicked);
    
    connect(m_gitService, &GitService::operationStarted, this, &ProtectedBranchView::onOperationStarted);
    connect(m_gitService, &GitService::operationFinished, this, &ProtectedBranchView::onOperationFinished);
}

void ProtectedBranchView::onPullClicked() {
    int ret = QMessageBox::question(this, QString::fromUtf8("确认拉取"),
        QString::fromUtf8("确定要从远程拉取最新代码吗？\n这将更新当前分支。"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 使用进度对话框
    ProgressDialog* progressDlg = new ProgressDialog(
        QString::fromUtf8("正在拉取最新代码"),
        QString("git pull"),
        this
    );
    
    bool success = false;
    connect(progressDlg, &ProgressDialog::commandFinished, [&success](bool result) {
        success = result;
    });
    
    progressDlg->executeCommand("git",
        QStringList() << "pull",
        m_gitService->getRepoPath());
    progressDlg->exec();
    
    if (success) {
        m_statusLabel->setText(QString::fromUtf8("拉取成功"));
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("已成功拉取最新代码！"));
    } else {
        m_statusLabel->setText(QString::fromUtf8("拉取失败"));
    }
    
    progressDlg->deleteLater();
}

void ProtectedBranchView::onNewBranchClicked() {
    QString baseBranch = m_gitService->getCurrentBranch();
    
    BranchCreatorDialog dialog(baseBranch, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QString branchName = dialog.getBranchName();
    m_newBranchButton->setEnabled(false);
    
    bool success = false;
    
    if (dialog.getSelectedType() == BranchCreatorDialog::Database) {
        // 数据库分支：直接checkout
        m_statusLabel->setText(QString::fromUtf8("正在切换到数据库分支..."));
        success = m_gitService->switchBranch("develop-database");
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8("成功"),
                QString::fromUtf8("已切换到 develop-database 分支\n\n"
                                 "此分支用于数据库变更，只能向develop合并。"));
        }
    } else {
        // 其他类型：创建新分支
        m_statusLabel->setText(QString::fromUtf8("正在创建分支..."));
        success = m_gitService->createBranch(branchName, baseBranch);
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8("成功"),
                QString::fromUtf8("已创建并切换到新分支：%1\n\n现在可以开始开发了！").arg(branchName));
        }
    }
    
    m_newBranchButton->setEnabled(true);
    
    if (success) {
        m_statusLabel->setText(QString::fromUtf8("分支操作成功"));
        emit branchChanged();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("失败"),
            QString::fromUtf8("分支操作失败，请检查分支名或Git状态。"));
        m_statusLabel->setText(QString::fromUtf8("分支操作失败"));
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

void ProtectedBranchView::onSwitchBranchClicked() {
    // 获取所有分支列表
    QStringList branches = m_gitService->getAllBranches();
    QString currentBranch = m_gitService->getCurrentBranch();
    
    if (branches.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("无可用分支"),
            QString::fromUtf8("未找到可切换的分支"));
        return;
    }
    
    // 从列表中移除当前分支
    branches.removeAll(currentBranch);
    
    if (branches.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("没有其他分支可供切换"));
        return;
    }
    
    // 创建选择对话框
    bool ok;
    QString selectedBranch = QInputDialog::getItem(
        this,
        QString::fromUtf8("切换分支"),
        QString::fromUtf8("选择要切换的分支：\n\n当前分支：%1").arg(currentBranch),
        branches,
        0,  // 默认选择第一个
        false,  // 不可编辑
        &ok
    );
    
    if (!ok || selectedBranch.isEmpty()) {
        return;
    }
    
    // 设置对话框最小宽度
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    if (!dialogs.isEmpty()) {
        dialogs.last()->setMinimumWidth(255);
    }
    
    // 检查是否有未提交的改动
    if (m_gitService->hasUncommittedChanges()) {
        int ret = QMessageBox::warning(this, 
            QString::fromUtf8("未提交的改动"),
            QString::fromUtf8("当前存在未提交的改动，切换分支可能会丢失这些改动。\n\n"
                                 "是否继续切换？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    
    // 执行切换
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在切换分支..."),
        QString(),
        0, 0,
        this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->show();
    
    QFuture<bool> future = QtConcurrent::run([this, selectedBranch]() {
        return m_gitService->switchBranch(selectedBranch);
    });
    
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    watcher->setFuture(future);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress, selectedBranch]() {
        bool success = watcher->result();
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            emit branchChanged();
        } else {
            QMessageBox::warning(this, QString::fromUtf8("切换失败"),
                QString::fromUtf8("切换到分支 %1 失败，请检查Git状态。").arg(selectedBranch));
        }
    });
}