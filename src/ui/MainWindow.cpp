#include "MainWindow.h"
#include "config/ConfigManager.h"
#include "utils/Logger.h"
#include "SettingsDialog.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gitService(new GitService(this))
    , m_gitLabApi(new GitLabApi(this))
    , m_refreshTimer(new QTimer(this))
{
    setWindowTitle("Easy Git");
    resize(1000, 700);
    
    setupUi();
    createMenuBar();
    connectServices();
    
    // 加载配置
    ConfigManager& config = ConfigManager::instance();
    m_gitService->setRepoPath(config.getRepoPath());
    m_gitLabApi->setBaseUrl(config.getGitLabUrl());
    m_gitLabApi->setApiToken(config.getGitLabToken());
    m_gitLabApi->setProjectId(config.getCurrentProjectId());
    
    // 启动分支监控
    loadCurrentBranch();
    m_refreshTimer->start(5000); // 每5秒检查一次分支变化
    
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
    
    // 状态栏
    m_statusLabel = new QLabel("就绪", this);
    statusBar()->addWidget(m_statusLabel);
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
            "Git客户端 v1.0\n\n"
            "GitLab Workflow Automation Tool\n"
            "防呆式GitLab工作流工具");
    });
}

void MainWindow::connectServices() {
    // Git服务信号
    connect(m_gitService, &GitService::operationStarted, 
            [this](const QString& op) {
        m_statusLabel->setText("正在执行: " + op);
    });
    
    connect(m_gitService, &GitService::operationFinished,
            [this](const QString& op, bool success) {
        if (success) {
            m_statusLabel->setText("完成: " + op);
        } else {
            m_statusLabel->setText("失败: " + op);
        }
    });
    
    // 定时刷新
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onBranchChanged);
}

void MainWindow::loadCurrentBranch() {
    if (!m_gitService->isValidRepo()) {
        QMessageBox::warning(this, "仓库无效",
            "当前目录不是有效的Git仓库！\n"
            "请在设置中配置正确的仓库路径。");
        return;
    }
    
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
        setWindowTitle(QString("Git客户端 - 🔴 %1 (只读)").arg(branchName));
    }
    else if (protectedBranches.contains(branchName)) {
        // 🔒 保护分支同步视图
        m_stackedWidget->setCurrentWidget(m_protectedBranchView);
        setWindowTitle(QString("Git客户端 - 🔒 %1 (受保护)").arg(branchName));
    }
    else if (branchName == databaseBranch) {
        // 🟣 数据库分支受限视图
        m_stackedWidget->setCurrentWidget(m_databaseBranchView);
        setWindowTitle(QString("Git客户端 - 🟣 %1 (数据库专用)").arg(branchName));
    }
    else {
        // 🟢 开发分支活跃视图
        m_stackedWidget->setCurrentWidget(m_featureBranchView);
        setWindowTitle(QString("Git客户端 - 🟢 %1 (开发中)").arg(branchName));
    }
    
    m_statusLabel->setText(QString("当前分支: %1").arg(branchName));
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
    }
}
