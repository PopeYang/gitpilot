#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QTabWidget;
class QLineEdit;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    
private slots:
    void onBrowseRepoPath();
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
    QLineEdit* m_projectIdEdit;
    QLineEdit* m_projectNameEdit;
};

#endif // SETTINGSDIALOG_H