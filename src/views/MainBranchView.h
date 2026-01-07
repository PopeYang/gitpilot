#ifndef MAINBRANCHVIEW_H
#define MAINBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;
class QListWidget;
class QPushButton;
class QLabel;

/**
 * @brief 主分支只读视图
 * 🔴 绝对只读：不允许任何本地修改操作
 */
class MainBranchView : public QWidget {
    Q_OBJECT
    
public:
    explicit MainBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
protected:
    void showEvent(QShowEvent* event) override;
    
private slots:
    void onPullClicked();
    void onTriggerBuildClicked();
    void onSwitchBranchClicked();
    void refreshTags();
    
private:
    void setupUi();
    void connectSignals();
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QListWidget* m_tagsListWidget;
    QPushButton* m_pullButton;
    QPushButton* m_triggerBuildButton;
    QPushButton* m_switchBranchButton;
    QLabel* m_warningLabel;
};

#endif