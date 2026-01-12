#ifndef PROTECTEDBRANCHVIEW_H
#define PROTECTEDBRANCHVIEW_H

#include <QWidget>
#include <QShowEvent>

class GitService;
class GitLabApi;
class QPushButton;
class QLabel;
class QListWidget;
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
    QListWidget* m_mrListWidget;
    
private slots:
    void onMergeRequestsReceived(const QList<MrResponse>& mrs);
    void refreshMrs();
};

#endif // PROTECTEDBRANCHVIEW_H