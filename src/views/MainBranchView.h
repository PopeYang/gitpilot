#ifndef MAINBRANCHVIEW_H
#define MAINBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;
class QGroupBox;
class QTimer;
struct PipelineStatus;

/**
 * @brief 主分支只读视图
 * 🔴 绝对只读：不允许任何本地修改操作
 */
class MainBranchView : public QWidget {
    Q_OBJECT
    
public:
    explicit MainBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
signals:
    void branchSwitched();  // 通知主窗口刷新
    
private slots:
    void onPullClicked();
    void onTriggerBuildClicked();
    void onSwitchBranchClicked();
    void refreshPipelines();
    void onPipelinesReceived(const QList<PipelineStatus>& pipelines);
    void onPipelineContextMenuRequested(const QPoint& pos);
    void onPipelineActionClicked();
    void onPipelineOperationCompleted(const PipelineStatus& pipeline);
    
private:
    void setupUi();
    void connectSignals();
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QPushButton* m_pullButton;
    QPushButton* m_triggerBuildButton;
    QPushButton* m_switchBranchButton;
    QLabel* m_warningLabel;
    
    // Pipeline List
    QGroupBox* m_pipelineGroup;
    QTreeWidget* m_pipelineTreeWidget;
    QPushButton* m_refreshPipelinesButton;
    QTimer* m_refreshTimer;
    
    int m_selectedPipelineId;
};

#endif