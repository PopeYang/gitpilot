#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QTabWidget;
class QLineEdit;
class QPushButton;
class GitLabApi;  // 前向声明

class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    
private slots:
    void onBrowseRepoPath();
    void onExtractFromGit();
    void onTestConnection();
    void onSave();
    void onApply();
    
private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    
    // GitLab配置
    QLineEdit* m_gitlabUrlEdit;
    QLineEdit* m_gitlabTokenEdit;
    QPushButton* m_testConnectionBtn;
    
    // 仓库配置
    QLineEdit* m_repoPathEdit;
    QPushButton* m_browseBtn;
    
    // 项目配置
    QLineEdit* m_projectPathEdit;  // 项目路径而非ID
    QLineEdit* m_projectNameEdit;
    
    // 用于测试连接的临时API实例
    GitLabApi* m_testApi;
};

#endif // SETTINGSDIALOG_H