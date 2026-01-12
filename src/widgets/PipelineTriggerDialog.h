#ifndef PIPELINETRIGGERDIALOG_H
#define PIPELINETRIGGERDIALOG_H

#include <QDialog>

class GitService;
class QPushButton;
class QComboBox;
class QLabel;

/**
 * @brief 分支选择对话框，用于触发 Pipeline
 * 提供快捷按钮（develop/internal）和其他分支下拉框
 */
class PipelineTriggerDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit PipelineTriggerDialog(GitService* gitService, QWidget* parent = nullptr);
    
    /**
     * @brief 获取用户选择的分支名
     * @return 选择的分支名，如果取消则返回空字符串
     */
    QString getSelectedBranch() const;
    
private slots:
    void onDevelopClicked();
    void onInternalClicked();
    void onOkClicked();
    void onCancelClicked();
    
private:
    void setupUi();
    void loadBranches();
    QStringList getFilteredBranches();
    
    GitService* m_gitService;
    QString m_selectedBranch;
    
    QPushButton* m_developButton;
    QPushButton* m_internalButton;
    QComboBox* m_otherBranchCombo;
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QLabel* m_titleLabel;
};

#endif // PIPELINETRIGGERDIALOG_H
