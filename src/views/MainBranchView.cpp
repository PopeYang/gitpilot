#include "MainBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QFutureWatcher>
#include <QTimer>
#include <QTreeWidget>
#include <QHeaderView>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QTimeZone>

MainBranchView::MainBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
{
    setupUi();
    connectSignals();
    
    // Auto refresh timer
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000); // 30s auto refresh
    connect(m_refreshTimer, &QTimer::timeout, this, &MainBranchView::refreshPipelines);
    m_refreshTimer->start();
}

void MainBranchView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 顶部警告区域
    QGroupBox* warningGroup = new QGroupBox(QString::fromUtf8("🔴 主分支保护区"), this);
    warningGroup->setStyleSheet(
        "QGroupBox {"
        "   background-color: #FFF5F5;"
        "   border: 2px solid #FF6B6B;"
        "   border-radius: 8px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
        "QGroupBox::title {"
        "   color: #C92A2A;"
        "}"
    );
    
    QVBoxLayout* warningLayout = new QVBoxLayout(warningGroup);
    m_warningLabel = new QLabel(
        QString::fromUtf8("⚠️ 这是生产环境分支，仅供查看\n\n"
                         "• 不允许本地修改\n"
                         "• 仅可拉取最新代码\n"
                         "• 可触发构建Pipeline"), 
        this);
    m_warningLabel->setStyleSheet("color: #C92A2A; font-size: 13px; background: transparent; border: none;");
    m_warningLabel->setWordWrap(true);
    warningLayout->addWidget(m_warningLabel);
    
    mainLayout->addWidget(warningGroup);
    
    // 操作按钮区域
    QGroupBox* actionGroup = new QGroupBox(QString::fromUtf8("🔄 操作区"), this);
    actionGroup->setStyleSheet("QGroupBox { font-size: 13px; font-weight: bold; padding: 10px; }");
    
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    actionLayout->setSpacing(15);
    
    m_pullButton = new QPushButton(QString::fromUtf8("🔄 拉取最新代码"), this);
    m_pullButton->setMinimumHeight(40);
    m_pullButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0a6bc5;"
        "}"
    );
    
    m_triggerBuildButton = new QPushButton(QString::fromUtf8("🚀 触发构建Pipeline"), this);
    m_triggerBuildButton->setMinimumHeight(40);
    m_triggerBuildButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #E65100;"
        "}"
    );
    
    m_switchBranchButton = new QPushButton(QString::fromUtf8("🔀 切换到其他分支"), this);
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
    
    actionLayout->addWidget(m_pullButton);
    actionLayout->addWidget(m_triggerBuildButton);
    actionLayout->addWidget(m_switchBranchButton);
    
    mainLayout->addWidget(actionGroup);
    mainLayout->addWidget(actionGroup);

    // Pipeline 列表区域
    m_pipelineGroup = new QGroupBox(QString::fromUtf8("🚀 CI/CD Pipelines"), this);
    QVBoxLayout* pipelineLayout = new QVBoxLayout(m_pipelineGroup);
    
    // Header
    QHBoxLayout* plHeaderLayout = new QHBoxLayout();
    QLabel* plHint = new QLabel(QString::fromUtf8("双击在浏览器中查看详情"), this);
    plHint->setStyleSheet("color: #666; font-size: 11px;");
    plHeaderLayout->addWidget(plHint);
    plHeaderLayout->addStretch();
    
    m_refreshPipelinesButton = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_refreshPipelinesButton->setMaximumWidth(80);
    plHeaderLayout->addWidget(m_refreshPipelinesButton);
    pipelineLayout->addLayout(plHeaderLayout);
    
    // Tree Widget
    m_pipelineTreeWidget = new QTreeWidget(this);
    m_pipelineTreeWidget->setAlternatingRowColors(true);
    m_pipelineTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pipelineTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pipelineTreeWidget->setRootIsDecorated(false);
    m_pipelineTreeWidget->setMinimumHeight(200);
    
    QStringList headerLabels;
    headerLabels << "ID" << "状态" << "分支" << "时间";
    m_pipelineTreeWidget->setHeaderLabels(headerLabels);
    
    // Column resizing
    QHeaderView* header = m_pipelineTreeWidget->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    pipelineLayout->addWidget(m_pipelineTreeWidget);
    mainLayout->addWidget(m_pipelineGroup);

    mainLayout->addStretch();
}

void MainBranchView::connectSignals() {
    connect(m_pullButton, &QPushButton::clicked, this, &MainBranchView::onPullClicked);
    connect(m_triggerBuildButton, &QPushButton::clicked, this, &MainBranchView::onTriggerBuildClicked);
    connect(m_switchBranchButton, &QPushButton::clicked, this, &MainBranchView::onSwitchBranchClicked);
    
    connect(m_refreshPipelinesButton, &QPushButton::clicked, this, &MainBranchView::refreshPipelines);
    connect(m_gitLabApi, &GitLabApi::pipelinesReceived, this, &MainBranchView::onPipelinesReceived);
    connect(m_gitLabApi, &GitLabApi::pipelineTriggered, this, &MainBranchView::refreshPipelines); // Refresh after trigger
    connect(m_gitLabApi, &GitLabApi::pipelineRetried, this, &MainBranchView::onPipelineOperationCompleted);
    connect(m_gitLabApi, &GitLabApi::pipelineCanceled, this, &MainBranchView::onPipelineOperationCompleted);
    
    connect(m_pipelineTreeWidget, &QTreeWidget::customContextMenuRequested, this, &MainBranchView::onPipelineContextMenuRequested);
    connect(m_pipelineTreeWidget, &QTreeWidget::itemDoubleClicked, [](QTreeWidgetItem* item, int column) {
        QString url = item->data(0, Qt::UserRole).toString();
        if(!url.isEmpty()) QDesktopServices::openUrl(QUrl(url));
    });
}

void MainBranchView::onPullClicked() {
    int ret = QMessageBox::question(
        this,
        QString::fromUtf8("确认拉取"),
        QString::fromUtf8("确认要拉取远程主分支的最新代码？"),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 显示进度对话框
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在拉取最新代码..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("拉取中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress]() {
        bool success = watcher->result();
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8("拉取成功"),
                QString::fromUtf8("✅ 已成功拉取最新代码"));
        } else {
            QMessageBox::warning(this, QString::fromUtf8("拉取失败"),
                QString::fromUtf8("拉取失败，请检查网络连接"));
        }
    });
    
    QFuture<bool> future = QtConcurrent::run([this]() {
        return m_gitService->pullLatest();
    });
    
    watcher->setFuture(future);
}

void MainBranchView::onTriggerBuildClicked() {
    QString currentBranch = m_gitService->getCurrentBranch();
    
    int ret = QMessageBox::question(
        this,
        QString::fromUtf8("确认触发构建"),
        QString::fromUtf8("确认要触发 %1 分支的Pipeline构建？\n\n"
                         "这将启动CI/CD流程。").arg(currentBranch),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 显示简短的进度提示
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在触发Pipeline..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("触发中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    QApplication::processEvents();
    
    // 连接错误信号 - 只在失败时提示
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this](const QString& endpoint, const QString& errorMessage) {
            if (endpoint.contains("pipeline")) {
                QMessageBox::warning(this, QString::fromUtf8("触发失败"),
                    QString::fromUtf8("Pipeline触发失败：\n\n%1").arg(errorMessage));
                
                disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
            }
        });
    
    // 触发API调用
    m_gitLabApi->triggerPipeline(currentBranch);
    
    // 立即关闭进度条，因为下方列表会自动刷新显示状态
    progress->close();
    progress->deleteLater();
    
    // 立即刷新Pipeline列表以显示新触发的Pipeline
    QTimer::singleShot(1000, this, &MainBranchView::refreshPipelines);
}

void MainBranchView::onSwitchBranchClicked() {
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
    
    // 设置对话框最小宽度
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    if (!dialogs.isEmpty()) {
        dialogs.last()->setMinimumWidth(255);
    }
    
    if (!ok || selectedBranch.isEmpty()) {
        return;
    }
    
    // 检查是否有未提交的修改
    if (m_gitService->hasUncommittedChanges()) {
        int ret = QMessageBox::warning(
            this,
            QString::fromUtf8("发现未提交的修改"),
            QString::fromUtf8("当前工作区有未提交的修改，切换分支可能会丢失修改。\n\n"
                             "是否继续切换？\n\n"
                             "建议：先暂存或提交修改后再切换。"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    
    // 显示进度对话框
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在切换分支..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("切换中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress, selectedBranch]() {
        bool success = watcher->result();
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            // 切换成功，直接通知主窗口刷新，不弹窗干扰用户
            emit branchSwitched();
        } else {
            QMessageBox::warning(this, QString::fromUtf8("切换失败"),
                QString::fromUtf8("切换分支失败，请检查工作区状态"));
        }
    });
    
    QFuture<bool> future = QtConcurrent::run([this, selectedBranch]() {
        return m_gitService->switchBranch(selectedBranch);
    });
    
    watcher->setFuture(future);
}

void MainBranchView::refreshPipelines() {
    QString currentBranch = m_gitService->getCurrentBranch();
    m_gitLabApi->listPipelines(currentBranch);
}

void MainBranchView::onPipelinesReceived(const QList<PipelineStatus>& pipelines) {
    m_pipelineTreeWidget->clear();
    
    if (pipelines.isEmpty()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_pipelineTreeWidget);
        item->setText(2, QString::fromUtf8("无Pipeline记录"));
    } else {
        for (const PipelineStatus& p : pipelines) {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_pipelineTreeWidget);
            item->setText(0, QString::number(p.id));
            item->setText(1, p.status);
            item->setText(2, p.ref);
            
             // 强制转换为UTC+8 (28800秒)
            QTimeZone zone = QTimeZone::fromSecondsAheadOfUtc(28800);
            QDateTime dt = p.createdAt.toTimeZone(zone);
            item->setText(3, dt.toString("MM-dd HH:mm"));
            
            item->setData(0, Qt::UserRole, p.webUrl);
            item->setData(0, Qt::UserRole + 1, p.id);
            item->setData(0, Qt::UserRole + 2, p.status); // Store status for context menu logic
            
            // Status color
            if (p.isSuccess()) item->setForeground(1, QBrush(QColor("#4CAF50"))); // Green
            else if (p.isFailed()) item->setForeground(1, QBrush(QColor("#F44336"))); // Red
            else if (p.isRunning()) item->setForeground(1, QBrush(QColor("#2196F3"))); // Blue
            else if (p.isPending()) item->setForeground(1, QBrush(QColor("#FF9800"))); // Orange
        }
    }
}

void MainBranchView::onPipelineContextMenuRequested(const QPoint& pos) {
    QTreeWidgetItem* item = m_pipelineTreeWidget->itemAt(pos);
    if (!item) return;
    
    m_selectedPipelineId = item->data(0, Qt::UserRole + 1).toInt();
    QString status = item->data(0, Qt::UserRole + 2).toString();
    QString url = item->data(0, Qt::UserRole).toString();
    
    if (m_selectedPipelineId == 0) return;
    
    QMenu contextMenu(this);
    
    QAction* browserAction = contextMenu.addAction(QString::fromUtf8("🌐 在浏览器中打开"));
    contextMenu.addSeparator();
    
    // Actions based on status
    if (status == "failed" || status == "canceled" || status == "success") {
        QAction* retryAction = contextMenu.addAction(QString::fromUtf8("🔄 重试 (Retry)"));
        retryAction->setData("retry");
        connect(retryAction, &QAction::triggered, this, &MainBranchView::onPipelineActionClicked);
    }
    
    if (status == "running" || status == "pending") {
        QAction* cancelAction = contextMenu.addAction(QString::fromUtf8("⏹️ 取消 (Cancel)"));
        cancelAction->setData("cancel");
        connect(cancelAction, &QAction::triggered, this, &MainBranchView::onPipelineActionClicked);
    }
    
    connect(browserAction, &QAction::triggered, [url]() {
        QDesktopServices::openUrl(QUrl(url));
    });
    
    contextMenu.exec(m_pipelineTreeWidget->mapToGlobal(pos));
}

void MainBranchView::onPipelineActionClicked() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) return;
    
    QString type = action->data().toString();
    
    if (type == "retry") {
        int ret = QMessageBox::question(this, QString::fromUtf8("确认重试"),
            QString::fromUtf8("确定要重试 Pipeline #%1 吗？").arg(m_selectedPipelineId),
            QMessageBox::Yes | QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            m_gitLabApi->retryPipeline(m_selectedPipelineId);
        }
    } else if (type == "cancel") {
        int ret = QMessageBox::question(this, QString::fromUtf8("确认取消"),
            QString::fromUtf8("确定要取消 Pipeline #%1 吗？").arg(m_selectedPipelineId),
            QMessageBox::Yes | QMessageBox::No);
            
        if (ret == QMessageBox::Yes) {
            m_gitLabApi->cancelPipeline(m_selectedPipelineId);
        }
    }
}

void MainBranchView::onPipelineOperationCompleted(const PipelineStatus& pipeline) {
    QString msg;
    // Detect operation type by status or just generic success
    if (pipeline.status == "pending" || pipeline.status == "running") msg = QString::fromUtf8("已重试 Pipeline");
    else if (pipeline.status == "canceled") msg = QString::fromUtf8("已取消 Pipeline");
    else msg = QString::fromUtf8("操作成功");
    
    QMessageBox::information(this, QString::fromUtf8("成功"),
        QString("%1 #%2\n状态: %3").arg(msg).arg(pipeline.id).arg(pipeline.status));
        
    refreshPipelines();
}