#ifndef PROTECTEDBRANCHVIEW_H
#define PROTECTEDBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;
class QPushButton;
class QLabel;
class QListWidget;

class ProtectedBranchView : public QWidget {
    Q_OBJECT
public:
    explicit ProtectedBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
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
};

#endif // PROTECTEDBRANCHVIEW_H