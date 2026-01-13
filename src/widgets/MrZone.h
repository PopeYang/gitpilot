#ifndef MRZONE_H
#define MRZONE_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class QListWidget;  // 新增
class GitService;
class GitLabApi;
struct ProjectMember;  // 新增

/**
 * @brief MR提交专区组件
 * 
 * 核心防呆功能：
 * - develop-database分支时，目标分支锁定为develop
 * - 其他分支可选择develop或internal
 */
class MrZone : public QWidget {
    Q_OBJECT
    
public:
    explicit MrZone(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
    
    // 根据当前分支更新UI状态
    void updateForBranch(const QString& currentBranch);
    
    // 触发提交动作（供外部调用）
    void triggerSubmit();
    
signals:
    void mrSubmitted(const QString& targetBranch, const QString& title, const QString& description);
    void conflictCheckRequested(const QString& targetBranch);
    
private slots:
    void onCheckConflictClicked();
    void onSubmitClicked();
    void onProjectMembersReceived(const QList<ProjectMember>& members);
    void updateAssigneeComboText();  // 新增：更新显示文本
    
private:
    void setupUi();
    void lockTargetBranch(const QString& branch);
    void unlockTargetBranch();
    void loadProjectMembers();

    void showAssigneePopup();
    void hideAssigneePopup();  // 新增：统一隐藏逻辑
    void setArrowState(bool isUp); // 新增：控制三角方向
    
    // 事件过滤器
    bool eventFilter(QObject* obj, QEvent* event) override;
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QComboBox* m_targetBranchCombo;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QPushButton* m_checkConflictButton;
    QPushButton* m_submitButton;
    QLabel* m_statusLabel;
    QComboBox* m_assigneeCombo;    // 新增：审核人下拉框
    QListWidget* m_assigneeList;   // 新增：弹出列表
    
    QList<ProjectMember> m_projectMembers;  // 新增：缓存成员列表
    QString m_currentBranch;
    bool m_isLocked;
};

#endif // MRZONE_H