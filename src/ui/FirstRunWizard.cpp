#include "FirstRunWizard.h"
#include "config/ConfigManager.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QFileInfo>

// Simplified version - avoiding complex string concatenation issues with MSVC

class WelcomePage : public QWizardPage {
public:
    WelcomePage(QWidget* parent = nullptr) : QWizardPage(parent) {
        setTitle(QString::fromUtf8("欢迎使用"));
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        QLabel* label = new QLabel(QString::fromUtf8(
            "GitPilot 客户端 - GitLab自动化工具\n\n"
            "本工具旨在简化GitLab CI/CD流程。\n\n"
            "点击下一步开始配置..."), this);
        
        layout->addWidget(label);
        layout->addStretch();
    }
};

class GitLabConfigPage : public QWizardPage {
    Q_OBJECT
public:
    GitLabConfigPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        setTitle(QString::fromUtf8("GitLab配置"));
        setSubTitle(QString::fromUtf8("请填写服务器信息"));
        
        QFormLayout* layout = new QFormLayout(this);
        
        urlEdit = new QLineEdit(this);
        urlEdit->setText(ConfigManager::DEFAULT_GITLAB_URL);
        layout->addRow(QString::fromUtf8("服务器:"), urlEdit);
        
        tokenEdit = new QLineEdit(this);
        tokenEdit->setEchoMode(QLineEdit::Password);
        layout->addRow("Token:", tokenEdit);
        
        QPushButton* helpBtn = new QPushButton(QString::fromUtf8("帮助"), this);
        connect(helpBtn, &QPushButton::clicked, this, &GitLabConfigPage::showHelp);
        layout->addRow("", helpBtn);
        
        registerField("gitlabUrl*", urlEdit);
        registerField("gitlabToken*", tokenEdit);
    }
    
private slots:
    void showHelp() {
        QMessageBox::information(this, QString::fromUtf8("获取Token"),
            QString::fromUtf8("请在GitLab中获取Personal Access Token\n需要api权限"));
    }
    
private:
    QLineEdit* urlEdit;
    QLineEdit* tokenEdit;
};

class RepoPathPage : public QWizardPage {
    Q_OBJECT
public:
    RepoPathPage(QWidget* parent = nullptr) : QWizardPage(parent) {
        setTitle(QString::fromUtf8("仓库路径"));
        setSubTitle(QString::fromUtf8("选择Git仓库目录"));
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        
        QHBoxLayout* pathLayout = new QHBoxLayout();
        pathEdit = new QLineEdit(this);
        
        QPushButton* browseBtn = new QPushButton(QString::fromUtf8("浏览"), this);
        connect(browseBtn, &QPushButton::clicked, this, &RepoPathPage::browse);
        
        pathLayout->addWidget(pathEdit);
        pathLayout->addWidget(browseBtn);
        layout->addLayout(pathLayout);
        
        statusLabel = new QLabel(this);
        layout->addWidget(statusLabel);
        layout->addStretch();
        
        connect(pathEdit, &QLineEdit::textChanged, this, &RepoPathPage::validate);
        
        registerField("repoPath*", pathEdit);
    }
    
    bool validatePage() override {
        QFileInfo gitDir(pathEdit->text() + "/.git");
        return gitDir.exists() && gitDir.isDir();
    }
    
private slots:
    void browse() {
        QString dir = QFileDialog::getExistingDirectory(this, QString::fromUtf8("选择目录"));
        if (!dir.isEmpty()) pathEdit->setText(dir);
    }
    
    void validate() {
        if (pathEdit->text().isEmpty()) {
            statusLabel->clear();
            return;
        }
        
        QFileInfo gitDir(pathEdit->text() + "/.git");
        if (gitDir.exists() && gitDir.isDir()) {
            statusLabel->setText(QString::fromUtf8("有效仓库"));
            statusLabel->setStyleSheet("color: green;");
        } else {
            statusLabel->setText(QString::fromUtf8("无效仓库"));
            statusLabel->setStyleSheet("color: red;");
        }
    }
    
private:
    QLineEdit* pathEdit;
    QLabel* statusLabel;
};

class CompletePage : public QWizardPage {
public:
    CompletePage(QWidget* parent = nullptr) : QWizardPage(parent) {
        setTitle(QString::fromUtf8("完成"));
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        QLabel* label = new QLabel(QString::fromUtf8("配置已完成！\n\n点击完成按钮开始使用。"), this);
        layout->addWidget(label);
        layout->addStretch();
    }
};

FirstRunWizard::FirstRunWizard(QWidget* parent) : QWizard(parent) {
    setWindowTitle(QString::fromUtf8("配置向导"));
    resize(600, 450);
    
    addPage(new WelcomePage(this));
    addPage(new GitLabConfigPage(this));
    addPage(new RepoPathPage(this));
    addPage(new CompletePage(this));
    
    setButtonText(QWizard::NextButton, QString::fromUtf8("下一步"));
    setButtonText(QWizard::BackButton, QString::fromUtf8("上一步"));
    setButtonText(QWizard::FinishButton, QString::fromUtf8("完成"));
    setButtonText(QWizard::CancelButton, QString::fromUtf8("取消"));
    
    connect(this, &QWizard::accepted, this, &FirstRunWizard::saveConfig);
}

void FirstRunWizard::saveConfig() {
    ConfigManager& config = ConfigManager::instance();
    config.setGitLabUrl(field("gitlabUrl").toString());
    config.setGitLabToken(field("gitlabToken").toString());
    config.setRepoPath(field("repoPath").toString());
    
    QMessageBox::information(this, QString::fromUtf8("成功"), QString::fromUtf8("配置已保存"));
}

#include "FirstRunWizard.moc"