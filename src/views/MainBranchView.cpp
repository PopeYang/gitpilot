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

MainBranchView::MainBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
{
    setupUi();
    connectSignals();
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
    
    // Tags列表区域
    QGroupBox* tagsGroup = new QGroupBox(QString::fromUtf8("📋 最近发布"), this);
    tagsGroup->setStyleSheet(
        "QGroupBox {"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
    );
    
    QVBoxLayout* tagsLayout = new QVBoxLayout(tagsGroup);
    m_tagsListWidget = new QListWidget(this);
    m_tagsListWidget->setMaximumHeight(150);
    m_tagsListWidget->setAlternatingRowColors(true);
    m_tagsListWidget->setStyleSheet(
        "QListWidget {"
        "   border: 1px solid #ddd;"
        "   border-radius: 4px;"
        "   background-color: white;"
        "   font-size: 12px;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #E3F2FD;"
        "   color: black;"
        "}"
    );
    tagsLayout->addWidget(m_tagsListWidget);
    
    mainLayout->addWidget(tagsGroup);
    
    // 操作按钮区域
    QGroupBox* actionGroup = new QGroupBox(QString::fromUtf8("🔄 操作区"), this);
    actionGroup->setStyleSheet("QGroupBox { font-size: 13px; font-weight: bold; padding: 10px; }");
    
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    
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
    mainLayout->addStretch();
}

void MainBranchView::connectSignals() {
    connect(m_pullButton, &QPushButton::clicked, this, &MainBranchView::onPullClicked);
    connect(m_triggerBuildButton, &QPushButton::clicked, this, &MainBranchView::onTriggerBuildClicked);
    connect(m_switchBranchButton, &QPushButton::clicked, this, &MainBranchView::onSwitchBranchClicked);
}

void MainBranchView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    refreshTags();
}

void MainBranchView::refreshTags() {
    m_tagsListWidget->clear();
    
    QStringList tags = m_gitService->getTags(10);
    
    if (tags.isEmpty()) {
        m_tagsListWidget->addItem(QString::fromUtf8("📝 暂无发布标签"));
    } else {
        for (const QString& tag : tags) {
            m_tagsListWidget->addItem(QString::fromUtf8("🏷️  ") + tag);
        }
    }
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
            refreshTags();  // 刷新Tags列表
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
    
    // 显示进度对话框
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
    
    // 连接成功信号
    connect(m_gitLabApi, &GitLabApi::pipelineTriggered, this,
        [this, progress](const PipelineStatus& pipeline) {
            progress->close();
            progress->deleteLater();
            
            QMessageBox::information(this, QString::fromUtf8("构建已触发"),
                QString::fromUtf8("✅ Pipeline已成功触发\n\n"
                                 "Pipeline ID: %1\n"
                                 "状态: %2").arg(pipeline.id).arg(pipeline.status));
            
            disconnect(m_gitLabApi, &GitLabApi::pipelineTriggered, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    // 连接错误信号
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this, progress](const QString& endpoint, const QString& errorMessage) {
            if (endpoint.contains("pipeline")) {
                progress->close();
                progress->deleteLater();
                
                QMessageBox::warning(this, QString::fromUtf8("触发失败"),
                    QString::fromUtf8("Pipeline触发失败：\n\n%1").arg(errorMessage));
                
                disconnect(m_gitLabApi, &GitLabApi::pipelineTriggered, this, nullptr);
                disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
            }
        });
    
    // 触发API调用
    m_gitLabApi->triggerPipeline(currentBranch);
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