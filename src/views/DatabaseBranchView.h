#ifndef DATABASEBRANCHVIEW_H
#define DATABASEBRANCHVIEW_H

#include <QWidget>
#include <QShowEvent>

class GitService;
class GitLabApi;
class MrZone;
class QListWidget;
class QPushButton;
class QLabel;

/**
 * @brief 数据库分支受限视图
 * 🟣 专用分支：仅用于数据库脚本变更，强制合并到 develop
 */
class DatabaseBranchView : public QWidget {
    Q_OBJECT
    
public:
    explicit DatabaseBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
protected:
    void showEvent(QShowEvent* event) override;
    
private slots:
    void onRefreshClicked();
    void onStageAllClicked();
    void onCommitClicked();
    void onPushClicked();
    void onConflictCheckRequested(const QString& targetBranch);
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
    QLabel* m_warningLabel;
};

#endif