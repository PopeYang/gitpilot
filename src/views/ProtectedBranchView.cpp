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
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>

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
                         "• 请新建分支进行新功能的开发"),
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
    m_pullButton->setMinimumHeight(40);
    m_pullButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 13px;"
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
    m_newBranchButton->setMinimumHeight(40);
    m_newBranchButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #1565C0;"
        "}"
    );
    actionsLayout->addWidget(m_newBranchButton);
    
    // 切换分支按钮
    m_switchBranchButton = new QPushButton(QString::fromUtf8("🔀 切换分支"), this);
    m_switchBranchButton->setMinimumHeight(40);
    m_switchBranchButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #9E9E9E;"
        "   color: white;"
        "   font-size: 13px;"
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
    
    // MR 列表区域
    m_mrGroup = new QGroupBox(QString::fromUtf8("📋 待合并的MR (Pending)"), this);
    QVBoxLayout* mrLayout = new QVBoxLayout(m_mrGroup);
    
    // 添加刷新按钮
    QHBoxLayout* mrHeaderLayout = new QHBoxLayout();
    QLabel* mrHint = new QLabel(QString::fromUtf8("双击MR条目可在浏览器中打开"), this);
    mrHint->setStyleSheet("color: #666; font-size: 11px;");
    mrHeaderLayout->addWidget(mrHint);
    mrHeaderLayout->addStretch();
    
    m_mrRefreshButton = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_mrRefreshButton->setMaximumWidth(80);
    m_mrRefreshButton->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333;"
        "   border: 1px solid #ccc;"
        "   font-size: 11px;"
        "   border-radius: 3px;"
        "   padding: 4px 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #f5f5f5;"
        "}"
    );
    mrHeaderLayout->addWidget(m_mrRefreshButton);
    mrLayout->addLayout(mrHeaderLayout);
    
    m_mrListWidget = new QListWidget(this);
    m_mrListWidget->setAlternatingRowColors(true);
    m_mrListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    mrLayout->addWidget(m_mrListWidget);
    
    mainLayout->addWidget(m_mrGroup);
    
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
    
    // MR Signal
    connect(m_gitLabApi, &GitLabApi::mergeRequestsReceived, this, &ProtectedBranchView::onMergeRequestsReceived);
    connect(m_gitLabApi, &GitLabApi::mergeRequestApproved, this, &ProtectedBranchView::onMrOperationCompleted);
    connect(m_gitLabApi, &GitLabApi::mergeRequestMerged, this, &ProtectedBranchView::onMrOperationCompleted);
    connect(m_gitLabApi, &GitLabApi::mergeRequestClosed, this, &ProtectedBranchView::onMrOperationCompleted);
    connect(m_gitLabApi, &GitLabApi::apiError, this, &ProtectedBranchView::onMrOperationFailed);
    connect(m_mrRefreshButton, &QPushButton::clicked, this, &ProtectedBranchView::refreshMrs);
    connect(m_mrListWidget, &QListWidget::itemDoubleClicked, this, &ProtectedBranchView::onMrItemDoubleClicked);
    connect(m_mrListWidget, &QListWidget::customContextMenuRequested, this, &ProtectedBranchView::onMrContextMenuRequested);
    
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
        m_statusLabel->setText(QString::fromUtf8("正在切换到数据库版本升级分支..."));
        success = m_gitService->switchBranch("develop-database");
        
        if (success) {
            // 切换成功后，自动执行pull
            m_statusLabel->setText(QString::fromUtf8("正在拉取最新代码..."));
            
            QProgressDialog* progress = new QProgressDialog(
                QString::fromUtf8("✅ 已切换到 develop-database\n正在拉取最新代码..."),
                QString(), 0, 0, this);
            progress->setWindowModality(Qt::WindowModal);
            progress->setMinimumDuration(0);
            progress->setCancelButton(nullptr);
            progress->show();
            
            // 异步执行pull
            QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
            QFuture<bool> future = QtConcurrent::run([this]() {
                return m_gitService->pullLatest();
            });
            watcher->setFuture(future);
            
            connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress]() {
                bool pullSuccess = watcher->result();
                progress->close();
                progress->deleteLater();
                watcher->deleteLater();
                
                if (pullSuccess) {
                    QMessageBox::information(this, QString::fromUtf8("成功"),
                        QString::fromUtf8("已切换到 develop-database 分支并拉取最新代码\n\n"
                                         "此分支用于数据库版本升级，只能向develop合并。"));
                } else {
                    QMessageBox::warning(this, QString::fromUtf8("拉取失败"),
                        QString::fromUtf8("已切换到 develop-database，但拉取最新代码失败。\n"
                                         "请手动执行拉取操作。"));
                }
                
                m_statusLabel->setText(QString::fromUtf8("分支操作完成"));
                emit branchChanged();
            });
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
        if (success) {
            emit branchChanged();
        } else {
            QMessageBox::warning(this, QString::fromUtf8("切换失败"),
                QString::fromUtf8("切换到分支 %1 失败，请检查Git状态。").arg(selectedBranch));
        }
    });
}

void ProtectedBranchView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refreshMrs();
}

void ProtectedBranchView::refreshMrs() {
    QString currentBranch = m_gitService->getCurrentBranch();
    setCursor(Qt::WaitCursor);
    m_gitLabApi->listMergeRequests(1, 20, "opened", currentBranch);
    setCursor(Qt::ArrowCursor);
}

void ProtectedBranchView::onMergeRequestsReceived(const QList<MrResponse>& mrs) {
    m_mrListWidget->clear();
    setCursor(Qt::ArrowCursor);
    
    if (mrs.isEmpty()) {
        m_mrListWidget->addItem(QString::fromUtf8("✓ 没有待处理的MR"));
    } else {
        for (const MrResponse& mr : mrs) {
            QString display = QString("!%1: %2").arg(mr.iid).arg(mr.title);
            QListWidgetItem* item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, mr.webUrl);
            item->setData(Qt::UserRole + 1, mr.iid);  // Store MR IID for context menu
            item->setToolTip(QString::fromUtf8("创建时间: %1\n状态: %2\n右键点击查看更多操作").arg(mr.createdAt, mr.state));
            m_mrListWidget->addItem(item);
        }
    }
}

void ProtectedBranchView::onMrItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    
    QString url = item->data(Qt::UserRole).toString();
    if (url.isEmpty()) {
        return; // 空条目(如 "没有待处理的MR")
    }
    
    // 使用系统默认浏览器打开URL
    if (!QDesktopServices::openUrl(QUrl(url))) {
        QMessageBox::warning(this, QString::fromUtf8("打开失败"),
            QString::fromUtf8("无法打开浏览器。\n\nMR链接: %1").arg(url));
    }
}

void ProtectedBranchView::onMrContextMenuRequested(const QPoint& pos) {
    QListWidgetItem* item = m_mrListWidget->itemAt(pos);
    if (!item) return;
    
    QString url = item->data(Qt::UserRole).toString();
    if (url.isEmpty()) return; // 空条目
    
    // 从UserRole+1获取MR IID
    m_selectedMrIid = item->data(Qt::UserRole + 1).toInt();
    if (m_selectedMrIid == 0) return;
    
    QMenu contextMenu(this);
    
    QAction* approveAction = contextMenu.addAction(QString::fromUtf8("✅ 批准 (Approve)"));
    QAction* mergeAction = contextMenu.addAction(QString::fromUtf8("🔀 合并 (Merge)"));
    QAction* closeAction = contextMenu.addAction(QString::fromUtf8("❌ 关闭 (Close)"));
    contextMenu.addSeparator();
    QAction* openAction = contextMenu.addAction(QString::fromUtf8("🌐 在浏览器中打开"));
    
    connect(approveAction, &QAction::triggered, this, &ProtectedBranchView::onMrApproveClicked);
    connect(mergeAction, &QAction::triggered, this, &ProtectedBranchView::onMrMergeClicked);
    connect(closeAction, &QAction::triggered, this, &ProtectedBranchView::onMrCloseClicked);
    connect(openAction, &QAction::triggered, [this, url]() {
        QDesktopServices::openUrl(QUrl(url));
    });
    
    contextMenu.exec(m_mrListWidget->mapToGlobal(pos));
}

void ProtectedBranchView::onMrApproveClicked() {
    if (m_selectedMrIid == 0) return;
    
    m_gitLabApi->approveMergeRequest(m_selectedMrIid);
    QMessageBox::information(this, QString::fromUtf8("批准MR"),
        QString::fromUtf8("正在批准 MR !%1，请稍候...").arg(m_selectedMrIid));
}

void ProtectedBranchView::onMrMergeClicked() {
    if (m_selectedMrIid == 0) return;
    
    int ret = QMessageBox::question(this, QString::fromUtf8("确认合并"),
        QString::fromUtf8("确定要合并 MR !%1 吗？\n\n此操作将：\n"
                         "• 将代码合并到目标分支\n"
                         "• 自动删除源分支\n\n"
                         "此操作不可撤销！").arg(m_selectedMrIid),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_gitLabApi->mergeMergeRequest(m_selectedMrIid, true);
        QMessageBox::information(this, QString::fromUtf8("合并MR"),
            QString::fromUtf8("正在合并 MR !%1，请稍候...").arg(m_selectedMrIid));
    }
}

void ProtectedBranchView::onMrCloseClicked() {
    if (m_selectedMrIid == 0) return;
    
    int ret = QMessageBox::question(this, QString::fromUtf8("确认关闭"),
        QString::fromUtf8("确定要关闭 MR !%1 而不合并吗？\n\n"
                         "此操作将关闭MR，不会合并代码。\n\n"
                         "是否继续？").arg(m_selectedMrIid),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_gitLabApi->closeMergeRequest(m_selectedMrIid);
        QMessageBox::information(this, QString::fromUtf8("关闭MR"),
            QString::fromUtf8("正在关闭 MR !%1，请稍候...").arg(m_selectedMrIid));
    }
}

void ProtectedBranchView::onMrOperationCompleted(const MrResponse& mr) {
    QMessageBox::information(this, QString::fromUtf8("操作成功"),
        QString::fromUtf8("MR !%1 操作完成！\n\n刷新列表以查看最新状态。").arg(mr.iid));
    
    // 自动刷新MR列表
    refreshMrs();
}

void ProtectedBranchView::onMrOperationFailed(const QString& endpoint, const QString& error) {
    // 只处理MR相关的错误
    if (!endpoint.contains("MergeRequest") && !endpoint.contains("merge_request")) {
        return;
    }
    
    QMessageBox::warning(this, QString::fromUtf8("操作失败"),
        QString::fromUtf8("MR操作失败：\n\n%1").arg(error));
}