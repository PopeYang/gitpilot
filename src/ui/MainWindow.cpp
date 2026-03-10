#include "MainWindow.h"
#include "config/ConfigManager.h"
#include "utils/Logger.h"
#include "utils/Logger.h"
#include "SettingsDialog.h"
#include "widgets/BranchSwitchDialog.h"
#include "views/MainBranchView.h"
#include "views/ProtectedBranchView.h"
#include "views/FeatureBranchView.h"
#include "views/DatabaseBranchView.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QInputDialog>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gitService(new GitService(this))
    , m_gitLabApi(new GitLabApi(this))
    , m_branchWatcher(new QFileSystemWatcher(this))
{
    setWindowTitle("Easy Git");
    resize(600, 700);
    
    setupUi();
    createMenuBar();
    connectServices();
    
    // 加载配置
    ConfigManager& config = ConfigManager::instance();
    m_gitService->setRepoPath(config.getRepoPath());
    m_gitLabApi->setBaseUrl(config.getGitLabUrl());
    m_gitLabApi->setApiToken(config.getGitLabToken());
    m_gitLabApi->setProjectId(config.getCurrentProjectId());
    
    // 延迟启动分支监控，避免阻塞窗口显示
    QTimer::singleShot(100, this, [this]() {
        loadCurrentBranch();
        setupBranchWatcher();
    });
    
    LOG_INFO("主窗口初始化完成");
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUi() {
    // 创建中央堆叠widget
    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);
    
    // 创建各视图（目前是占位实现）
    m_mainBranchView = new MainBranchView(m_gitService, m_gitLabApi, this);
    m_protectedBranchView = new ProtectedBranchView(m_gitService, m_gitLabApi, this);
    m_featureBranchView = new FeatureBranchView(m_gitService, m_gitLabApi, this);
    m_databaseBranchView = new DatabaseBranchView(m_gitService, m_gitLabApi, this);
    
    m_stackedWidget->addWidget(m_mainBranchView);
    m_stackedWidget->addWidget(m_protectedBranchView);
    m_stackedWidget->addWidget(m_featureBranchView);
    m_stackedWidget->addWidget(m_databaseBranchView);
    
    // 连接分支变化信号
    connect(m_protectedBranchView, &ProtectedBranchView::branchChanged, 
            this, &MainWindow::loadCurrentBranch);
    
    connect(m_mainBranchView, &MainBranchView::branchSwitched,
            this, &MainWindow::loadCurrentBranch);
    
    // 状态栏 - 双标签
    m_operationLabel = new QLabel(QString::fromUtf8("就绪"), this);
    
    // 右侧分支切换按钮
    m_branchButton = new QPushButton("", this);
    m_branchButton->setFlat(true);
    m_branchButton->setCursor(Qt::PointingHandCursor);
    m_branchButton->setStyleSheet(
        "QPushButton { "
        "   border: none; "
        "   padding: 0 10px; "
        "   text-align: right; "
        "   color: #333; "
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { "
        "   background-color: #f0f0f0; "
        "   color: #000; "
        "}"
    );
    connect(m_branchButton, &QPushButton::clicked, this, &MainWindow::onBranchSwitchClicked);
    
    statusBar()->addWidget(m_operationLabel, 1);  // 伸缩
    statusBar()->addPermanentWidget(m_branchButton);  // 固定宽度
}

void MainWindow::createMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");
    
    QAction* refreshAction = fileMenu->addAction("刷新(&R)");
    refreshAction->setShortcut(QKeySequence::Refresh);
    connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshRequested);
    
    fileMenu->addSeparator();
    
    QAction* settingsAction = fileMenu->addAction("设置(&S)");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettingsRequested);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu* helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction* aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于", 
            "Git Pilot 客户端 v1.0\n\n"
            "GitLab Workflow Automation Tool");
    });
}

void MainWindow::connectServices() {
    // Git服务信号 - 操作开始时显示进度
    connect(m_gitService, &GitService::operationStarted, 
            [this](const QString& op) {
        m_operationLabel->setText(QString::fromUtf8("正在执行: %1").arg(op));
    });
    
    connect(m_gitService, &GitService::operationFinished,
            [this](const QString& op, bool success) {
        // 操作完成后，恢复显示就绪
        m_operationLabel->setText(QString::fromUtf8("就绪"));
    });
    
    // 文件监控器事件
    connect(m_branchWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        LOG_INFO(QString("监测到分支文件变化: %1").arg(path));
        // Git在某些操作（如 checkout）时可能会删除并重新创建 HEAD 文件，导致 watcher 失效
        // 因此需要重新将其加入监控
        if (!m_branchWatcher->files().contains(path)) {
            QFileInfo fileInfo(path);
            if (fileInfo.exists()) {
                m_branchWatcher->addPath(path);
            }
        }
        onBranchChanged();
    });
}

// 提取的新方法，用于随时重新配置 watcher
void MainWindow::setupBranchWatcher() {
    // 先清空旧的监控
    QStringList oldFiles = m_branchWatcher->files();
    if (!oldFiles.isEmpty()) {
        m_branchWatcher->removePaths(oldFiles);
    }
    
    ConfigManager& config = ConfigManager::instance();
    QString repoPath = config.getRepoPath();
    if (repoPath.isEmpty()) return;
    
    // 监控 .git/HEAD 文件
    QDir repoDir(repoPath);
    QString headFilePath = repoDir.filePath(".git/HEAD");
    
    QFileInfo headInfo(headFilePath);
    if (headInfo.exists()) {
        m_branchWatcher->addPath(headFilePath);
        LOG_INFO(QString("已启动分支监控: %1").arg(headFilePath));
    } else {
        LOG_WARNING(QString("找不到 HEAD 文件，无法建立监控: %1").arg(headFilePath));
    }
}

void MainWindow::loadCurrentBranch() {
    if (!m_gitService->isValidRepo()) {
        // 仓库未配置或无效时，显示友好提示而不是警告弹窗
        m_operationLabel->setText(QString::fromUtf8("请在菜单 [文件 > 设置] 中配置仓库路径"));
        m_branchButton->setText(QString::fromUtf8("⚙️ 未配置"));
        m_branchButton->setEnabled(false);
        return;
    }
    
    // 启用分支按钮
    m_branchButton->setEnabled(true);
    
    QString branch = m_gitService->getCurrentBranch();
    if (branch != m_currentBranch) {
        m_currentBranch = branch;
        switchToAppropriateView(branch);
    }
}

void MainWindow::switchToAppropriateView(const QString& branchName) {
    LOG_INFO(QString("切换视图: 分支=%1").arg(branchName));
    
    ConfigManager& config = ConfigManager::instance();
    QStringList protectedBranches = config.getProtectedBranches();
    QString databaseBranch = config.getDatabaseBranchName();
    
    if (branchName == "main" || branchName == "master") {
        // 🔴 主分支只读视图
        m_stackedWidget->setCurrentWidget(m_mainBranchView);
        setWindowTitle(QString("GitPilot客户端 - 🔴 %1 (主干分支)").arg(branchName));
    }
    else if (protectedBranches.contains(branchName)) {
        // 🔒 保护分支同步视图
        m_stackedWidget->setCurrentWidget(m_protectedBranchView);
        setWindowTitle(QString("GitPilot客户端 - 🔒 %1 (集成分支)").arg(branchName));
    }
    else if (branchName == databaseBranch) {
        // 🟣 数据库分支受限视图
        m_stackedWidget->setCurrentWidget(m_databaseBranchView);
        setWindowTitle(QString("GitPilot客户端 - 🟣 %1 (数据库版本升级专用分支)").arg(branchName));
    }
    else {
        // 🟢 开发分支活跃视图
        m_stackedWidget->setCurrentWidget(m_featureBranchView);
        // 刷新视图以更新UI（特别是欢迎区域的样式）
        m_featureBranchView->refreshView();
        setWindowTitle(QString("GitPilot客户端 - 🟢 %1 (工作分支)").arg(branchName));
    }
    
    m_branchButton->setText(QString::fromUtf8("🌿 %1").arg(branchName));
}

void MainWindow::onBranchChanged() {
    loadCurrentBranch();
}

void MainWindow::onRefreshRequested() {
    LOG_INFO("手动刷新请求");
    loadCurrentBranch();
    QMessageBox::information(this, "刷新", "已刷新当前分支状态");
}

void MainWindow::onSettingsRequested() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 重新加载配置
        ConfigManager& config = ConfigManager::instance();
        m_gitService->setRepoPath(config.getRepoPath());
        m_gitLabApi->setBaseUrl(config.getGitLabUrl());
        m_gitLabApi->setApiToken(config.getGitLabToken());
        m_gitLabApi->setProjectId(config.getCurrentProjectId());
        
        loadCurrentBranch();
        setupBranchWatcher(); // 更新repoPath后需要重新设置文件监控
    }
}

#include <QProgressDialog>
#include <QtConcurrent>
#include <QFutureWatcher>

void MainWindow::onBranchSwitchClicked() {
    QStringList branches = m_gitService->getAllBranches();
    if (branches.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可用的本地分支");
        return;
    }
    
    QString currentBranch = m_gitService->getCurrentBranch();
    
    // 获取配置中的数据库分支名
    QString databaseBranch = ConfigManager::instance().getDatabaseBranchName();
    
    // 使用新的分支切换对话框
    BranchSwitchDialog dialog(currentBranch, branches, databaseBranch, this);
    
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    
    QString targetBranch = dialog.getTargetBranch();
    if (targetBranch.isEmpty() || targetBranch == currentBranch) {
        return;
    }

    // 创建进度条对话框
    QProgressDialog* progress = new QProgressDialog(QString::fromUtf8("正在切换分支到 %1...").arg(targetBranch), QString(), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0); // 立即显示
    progress->setCancelButton(nullptr); // 禁止取消
    progress->show();
    
    // 使用 QtConcurrent 在后台线程执行切换操作
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    QFuture<bool> future = QtConcurrent::run([this, targetBranch]() {
        return m_gitService->switchBranch(targetBranch);
    });
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress, targetBranch]() {
        bool success = watcher->result();
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            // 切换成功，不显示弹窗，直接刷新界面
            loadCurrentBranch();
            // 可选：在状态栏显示短暂的成功消息
            statusBar()->showMessage(QString::fromUtf8("已切换到分支: %1").arg(targetBranch), 3000);
        } else {
            QMessageBox::critical(this, "错误", QString::fromUtf8("切换分支失败\n请检查是否有未提交的更改或冲突"));
        }
    });
    
    watcher->setFuture(future);
}
