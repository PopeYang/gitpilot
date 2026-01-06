#ifndef MRZONE_H
#define MRZONE_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class GitService;
class GitLabApi;

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
    
signals:
    void mrSubmitted(const QString& targetBranch, const QString& title, const QString& description);
    
private slots:
    void onSubmitClicked();
    
private:
    void setupUi();
    void lockTargetBranch(const QString& branch);
    void unlockTargetBranch();
    
    GitService* m_gitService;
    GitLabApi* m_gitLabApi;
    
    QComboBox* m_targetBranchCombo;
    QLineEdit* m_titleEdit;
    QTextEdit* m_descriptionEdit;
    QPushButton* m_submitButton;
    QLabel* m_statusLabel;
    
    QString m_currentBranch;
    bool m_isLocked;
};

#endif // MRZONE_H