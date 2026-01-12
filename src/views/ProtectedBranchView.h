#ifndef PROTECTEDBRANCHVIEW_H
#define PROTECTEDBRANCHVIEW_H

#include <QWidget>
#include <QShowEvent>

class GitService;
class GitLabApi;
class QPushButton;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QGroupBox;
struct MrResponse;

class ProtectedBranchView : public QWidget {
    Q_OBJECT
public:
    explicit ProtectedBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    
signals:
    void branchChanged();
    
private slots:
    void onPullClicked();
    void onNewBranchClicked();
    void onSwitchBranchClicked();
    void onOperationStarted(const QString& operation);
    void onOperationFinished(const QString& operation, bool success);
    void onMrItemDoubleClicked(QTreeWidgetItem* item, int column);
    
private:
    void setupUi();
    void connectSignals();
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QPushButton* m_pullButton;
    QPushButton* m_newBranchButton;
    QPushButton* m_switchBranchButton;
    QLabel* m_statusLabel;
    
    QGroupBox* m_mrGroup;
    QTreeWidget* m_mrTreeWidget;
    QPushButton* m_mrRefreshButton;
    
private slots:
    void onMergeRequestsReceived(const QList<MrResponse>& mrs);
    void refreshMrs();
    void onMrContextMenuRequested(const QPoint& pos);
    void onMrApproveClicked();
    void onMrMergeClicked();
    void onMrCloseClicked();
    void onMrOperationCompleted(const MrResponse& mr);
    void onMrOperationFailed(const QString& endpoint, const QString& error);
    
private:
    int m_selectedMrIid;
};

#endif // PROTECTEDBRANCHVIEW_H