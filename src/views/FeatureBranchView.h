#ifndef FEATUREBRANCHVIEW_H
#define FEATUREBRANCHVIEW_H

#include <QWidget>
#include <QShowEvent>

class GitService;
class GitLabApi;
class MrZone;
class QListWidget;
class QPushButton;

class FeatureBranchView : public QWidget {
    Q_OBJECT
    
public:
    explicit FeatureBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
protected:
    void showEvent(QShowEvent* event) override;
    
private slots:
    void onRefreshClicked();
    void onStageAllClicked();
    void onCommitClicked();
    void onPushClicked();
    void onMrSubmitted(const QString& targetBranch, const QString& title, const QString& description);
    
private:
    void setupUi();
    void connectSignals();
    void updateFileList();
    void updateMrZone();
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QListWidget* m_filesListWidget;
    QPushButton* m_refreshButton;
    QPushButton* m_stageAllButton;
    QPushButton* m_commitButton;
    QPushButton* m_pushButton;
    MrZone* m_mrZone;
};

#endif // FEATUREBRANCHVIEW_H