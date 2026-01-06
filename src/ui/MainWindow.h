#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include "service/GitService.h"
#include "api/GitLabApi.h"

// 前向声明
class MainBranchView;
class ProtectedBranchView;
class FeatureBranchView;
class DatabaseBranchView;

/**
 * @brief 主窗口
 * 根据当前分支类型动态切换视图
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void onBranchChanged();
    void onRefreshRequested();
    void onSettingsRequested();
    
private:
    void setupUi();
    void createMenuBar();
    void connectServices();
    void loadCurrentBranch();
    void switchToAppropriateView(const QString& branchName);
    
    // 核心服务
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    // UI组件
    QStackedWidget* m_stackedWidget;
    QLabel* m_statusLabel;
    
    // 视图组件
    MainBranchView* m_mainBranchView;
    ProtectedBranchView* m_protectedBranchView;
    FeatureBranchView* m_featureBranchView;
    DatabaseBranchView* m_databaseBranchView;
    
    // 定时器（监控分支变化）
    QTimer* m_refreshTimer;
    
    QString m_currentBranch;
};

#endif // MAINWINDOW_H
