#include "SettingsDialog.h"
#include "config/ConfigManager.h"
#include "api/GitLabApi.h"
#include "api/ApiModels.h"
#include "service/GitService.h"
#include "widgets/ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget* parent) 
    : QDialog(parent)
    , m_testApi(nullptr)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    setWindowTitle(QString::fromUtf8("设置"));
    resize(400, 450);  // 缩小窗口以匹配主窗口600px宽度
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // ========== GitLab配置标签页 ==========
    QWidget* gitlabTab = new QWidget(this);
    QVBoxLayout* gitlabLayout = new QVBoxLayout(gitlabTab);
    
    QGroupBox* gitlabGroup = new QGroupBox(QString::fromUtf8("GitLab服务器"), this);
    gitlabGroup->setStyleSheet(
        "QGroupBox {"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   margin-top: 12px;"
        "   padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 5px;"
        "   margin-top: 0px;"
        "}"
    );
    QFormLayout* gitlabForm = new QFormLayout(gitlabGroup);
    
    m_gitlabUrlEdit = new QLineEdit(this);
    m_gitlabUrlEdit->setPlaceholderText("https://gitlab.example.com");
    gitlabForm->addRow(QString::fromUtf8("服务器地址:"), m_gitlabUrlEdit);
    
    m_gitlabTokenEdit = new QLineEdit(this);
    m_gitlabTokenEdit->setEchoMode(QLineEdit::Password);
    m_gitlabTokenEdit->setPlaceholderText("glpat-xxxxxxxxxxxxxxxxxxxx");
    gitlabForm->addRow("Access Token:", m_gitlabTokenEdit);
    
    m_testConnectionBtn = new QPushButton(QString::fromUtf8("测试连接"), this);
    connect(m_testConnectionBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    gitlabForm->addRow("", m_testConnectionBtn);
    
    gitlabLayout->addWidget(gitlabGroup);
    
    QLabel* tokenHint = new QLabel(
        QString::fromUtf8("💡 如何获取Token：\n"
                         "GitLab → Settings → Access Tokens → 创建Token\n"
                         "需要权限：api, read_api, read_repository"),
        this
    );
    tokenHint->setStyleSheet("color: #666; font-size: 11px;");
    tokenHint->setWordWrap(true);
    gitlabLayout->addWidget(tokenHint);
    gitlabLayout->addStretch();
    
    tabWidget->addTab(gitlabTab, QString::fromUtf8("GitLab"));
    
    // ========== 仓库配置标签页 ==========
    QWidget* repoTab = new QWidget(this);
    QVBoxLayout* repoLayout = new QVBoxLayout(repoTab);
    
    // 远程仓库URL部分
    QGroupBox* remoteGroup = new QGroupBox(QString::fromUtf8("远程仓库"), this);
    remoteGroup->setStyleSheet(
        "QGroupBox {"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   margin-top: 12px;"
        "   padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 5px;"
        "   margin-top: 0px;"
        "}"
    );
    QVBoxLayout* remoteLayout = new QVBoxLayout(remoteGroup);
    
    QLabel* remoteLabel = new QLabel(QString::fromUtf8("仓库URL (HTTPS):"), this);
    remoteLayout->addWidget(remoteLabel);
    
    m_remoteUrlEdit = new QLineEdit(this);
    m_remoteUrlEdit->setPlaceholderText("https://gitlab.example.com/namespace/project.git");
    remoteLayout->addWidget(m_remoteUrlEdit);
    
    // Clone按钮
    m_cloneButton = new QPushButton(QString::fromUtf8("📥 Clone到本地"), this);
    m_cloneButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; padding: 5px 15px; border-radius: 3px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0b7dda; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    connect(m_cloneButton, &QPushButton::clicked, this, &SettingsDialog::onCloneRepository);
    remoteLayout->addWidget(m_cloneButton);
    
    repoLayout->addWidget(remoteGroup);
    
    QGroupBox* repoGroup = new QGroupBox(QString::fromUtf8("本地仓库"), this);
    repoGroup->setStyleSheet(
        "QGroupBox {"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   margin-top: 12px;"
        "   padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 5px;"
        "   margin-top: 0px;"
        "}"
    );
    QVBoxLayout* repoGroupLayout = new QVBoxLayout(repoGroup);
    
    QLabel* repoLabel = new QLabel(QString::fromUtf8("仓库路径:"), this);
    repoGroupLayout->addWidget(repoLabel);
    
    QHBoxLayout* repoPathLayout = new QHBoxLayout();
    m_repoPathEdit = new QLineEdit(this);
    m_repoPathEdit->setPlaceholderText("D:/Projects/MyProject");
    
    m_browseBtn = new QPushButton(QString::fromUtf8("浏览..."), this);
    connect(m_browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseRepoPath);
    
    repoPathLayout->addWidget(m_repoPathEdit);
    repoPathLayout->addWidget(m_browseBtn);
    repoGroupLayout->addLayout(repoPathLayout);
    
    // 自动提取按钮
    QPushButton* extractBtn = new QPushButton(QString::fromUtf8("从 Git 提取项目信息"), this);
    extractBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; padding: 5px 10px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(extractBtn, &QPushButton::clicked, this, &SettingsDialog::onExtractFromGit);
    repoGroupLayout->addWidget(extractBtn);
    
    repoLayout->addWidget(repoGroup);
    
    // GitLab项目信息
    QGroupBox* projectGroup = new QGroupBox(QString::fromUtf8("GitLab项目"), this);
    projectGroup->setStyleSheet(
        "QGroupBox {"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   margin-top: 12px;"
        "   padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   padding: 0 5px;"
        "   margin-top: 0px;"
        "}"
    );
    QFormLayout* projectForm = new QFormLayout(projectGroup);
    
    m_projectPathEdit = new QLineEdit(this);
    m_projectPathEdit->setPlaceholderText("yanghaozhe/test");
    projectForm->addRow(QString::fromUtf8("项目路径:"), m_projectPathEdit);
    
    m_projectNameEdit = new QLineEdit(this);
    m_projectNameEdit->setPlaceholderText(QString::fromUtf8("我的项目"));
    projectForm->addRow(QString::fromUtf8("项目名称:"), m_projectNameEdit);
    
    repoLayout->addWidget(projectGroup);
    
    QLabel* repoHint = new QLabel(
        QString::fromUtf8("💡 点击'从 Git 提取项目信息'自动从远程 URL 获取项目路径"),
        this
    );
    repoHint->setStyleSheet("color: #666; font-size: 11px;");
    repoLayout->addWidget(repoHint);
    repoLayout->addStretch();
    
    tabWidget->addTab(repoTab, QString::fromUtf8("仓库"));
    
    mainLayout->addWidget(tabWidget);
    
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
        this
    );
    buttonBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确定"));
    buttonBox->button(QDialogButtonBox::Apply)->setText(QString::fromUtf8("应用"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void SettingsDialog::loadSettings() {
    ConfigManager& config = ConfigManager::instance();
    
    m_gitlabUrlEdit->setText(config.getGitLabUrl());
    m_gitlabTokenEdit->setText(config.getGitLabToken());
    m_repoPathEdit->setText(config.getRepoPath());
    m_projectPathEdit->setText(config.getCurrentProjectId());  // 现在存储的是项目路径
    m_projectNameEdit->setText(config.getCurrentProjectName());
}

void SettingsDialog::saveSettings() {
    ConfigManager& config = ConfigManager::instance();
    
    config.setGitLabUrl(m_gitlabUrlEdit->text().trimmed());
    config.setGitLabToken(m_gitlabTokenEdit->text().trimmed());
    config.setRepoPath(m_repoPathEdit->text().trimmed());
    config.setCurrentProjectId(m_projectPathEdit->text().trimmed());  // 保存项目路径
    config.setCurrentProjectName(m_projectNameEdit->text().trimmed());
}

void SettingsDialog::onCloneRepository() {
    QString url = m_remoteUrlEdit->text().trimmed();
    
    if (url.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("输入错误"),
            QString::fromUtf8("请先输入远程仓库URL"));
        return;
    }
    
    // 选择目标目录
    QString parentDir = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择Clone目标目录"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (parentDir.isEmpty()) {
        return;  // 用户取消
    }
    
    // 从URL提取项目名作为文件夹名
    QRegularExpression regex(R"(/([^/]+?)(?:\.git)?$)");
    QRegularExpressionMatch match = regex.match(url);
    QString projectName = match.hasMatch() ? match.captured(1) : "repository";
    
    QString targetPath = parentDir + "/" + projectName;
    
    // 检查目标目录是否已存在
    if (QDir(targetPath).exists()) {
        int ret = QMessageBox::question(this, QString::fromUtf8("目录已存在"),
            QString::fromUtf8("目录 %1 已存在\n是否仍要继续？").arg(targetPath),
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret != QMessageBox::Yes) {
            return;
        }
    }
    
    // 取消曾经的阻塞式ProgressDialog
    // 改为使用GitService的异步Clone
    m_cloneButton->setEnabled(false);
    m_cloneButton->setText(QString::fromUtf8("正在Clone..."));
    
    GitService* gitService = new GitService(this);
    
    connect(gitService, &GitService::cloneFinished, this, [this, gitService, targetPath](bool success, const QString& errorMsg) {
        m_cloneButton->setEnabled(true);
        m_cloneButton->setText(QString::fromUtf8("📥 Clone到本地"));
        
        if (success) {
            // 自动填充仓库路径
            m_repoPathEdit->setText(targetPath);
            
            QMessageBox::information(this, QString::fromUtf8("Clone成功"),
                QString::fromUtf8("仓库已成功Clone到：\n%1\n\n已自动填充到仓库路径").arg(targetPath));
            
            // 自动提取项目信息
            onExtractFromGit();
        } else {
            QMessageBox::critical(this, QString::fromUtf8("Clone失败"),
                QString::fromUtf8("Clone失败:\n%1").arg(errorMsg));
        }
        
        gitService->deleteLater();
    });
    
    gitService->cloneRepositoryAsync(url, targetPath);
}

void SettingsDialog::onBrowseRepoPath() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择Git仓库目录"),
        m_repoPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        m_repoPathEdit->setText(dir);
    }
}

void SettingsDialog::onExtractFromGit() {
    QString repoPath = m_repoPathEdit->text().trimmed();

    if (repoPath.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
            QString::fromUtf8("请先选择仓库路径"));
        return;
    }

    // 使用临时GitService获取信息
    GitService tempService(this);
    tempService.setRepoPath(repoPath);
    
    if (!tempService.isValidRepo()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), 
            QString::fromUtf8("该目录不是有效的Git仓库"));
        return;
    }
    
    // 获取远程URL
    QString remoteUrl = tempService.getRemoteUrl();
    if (remoteUrl.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), 
            QString::fromUtf8("未找到远程仓库(origin)配置"));
        return;
    }
    
    // 自动填充远程URL
    m_remoteUrlEdit->setText(remoteUrl);
    
    // 解析项目路径及Host
    // 支持解析Host以自动填充服务器地址
    // 捕获组1: Base URL (https://host 或 git@host)
    // 捕获组2: Project Path (支持多级group/subgroup/project)
    QRegularExpression regex(R"((https?://[^/]+|git@[^:]+)(?:/|:)(.+?)(?:\.git)?$)");
    QRegularExpressionMatch match = regex.match(remoteUrl);

    if (match.hasMatch()) {
        QString baseUrl = match.captured(1);
        QString projectPath = match.captured(2);
        
        // 如果是SSH格式 (git@domain.com), 尝试转换为HTTPS格式 (https://domain.com)
        if (baseUrl.startsWith("git@")) {
            baseUrl.replace("git@", "https://");
        }

        m_gitlabUrlEdit->setText(baseUrl);
        m_projectPathEdit->setText(projectPath);

        // 猜测项目名称 (取最后一段)
        QString projectName = projectPath.split('/').last();
        m_projectNameEdit->setText(projectName);

        QMessageBox::information(this, QString::fromUtf8("提取成功"), 
            QString::fromUtf8("成功提取项目信息：\n\n"
                              "服务器地址: %1\n"
                              "项目路径: %2\n"
                              "项目名称: %3\n\n"
                              "请检查是否准确，特别是Token需要手动填写。").arg(baseUrl, projectPath, projectName));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("解析失败"), 
            QString::fromUtf8("无法从URL中解析项目路径：\n%1\n\n"
                              "请手动填写，或确保URL格式标准。").arg(remoteUrl));
    }
}

#include <QSslSocket>

void SettingsDialog::onTestConnection() {
    QString url = m_gitlabUrlEdit->text().trimmed();
    QString token = m_gitlabTokenEdit->text().trimmed();
    
    if (url.isEmpty() || token.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("输入错误"),
            QString::fromUtf8("请输入服务器地址和Access Token"));
        return;
    }
    
    m_testConnectionBtn->setText(QString::fromUtf8("连接中..."));
    m_testConnectionBtn->setEnabled(false);
    
    if (m_testApi) {
        delete m_testApi;
    }
    m_testApi = new GitLabApi(this);
    m_testApi->setBaseUrl(url);
    m_testApi->setApiToken(token);
    
    // 检查SSL支持情况
    if (!QSslSocket::supportsSsl()) {
        QString sslVersion = QSslSocket::sslLibraryBuildVersionString();
        QMessageBox::critical(this, QString::fromUtf8("SSL库版本不匹配"),
            QString::fromUtf8("OpenSSL加载失败！\n\n"
                              "1. Qt构建依赖版本: %1\n"
                              "2. 当前运行时版本: %2\n\n"
                              "请确保复制了正确版本的DLL (v1.1 或 v3.0)。\n"
                              "建议检查 build/Release 目录下是否存在 libssl-*.dll 和 libcrypto-*.dll")
                              .arg(sslVersion)
                              .arg(QSslSocket::sslLibraryVersionString()));
        m_testConnectionBtn->setText(QString::fromUtf8("测试连接"));
        m_testConnectionBtn->setEnabled(true);
        return;
    }
    
    connect(m_testApi, &GitLabApi::userInfoReceived, this, [this](const UserInfo& user) {
        m_testConnectionBtn->setText(QString::fromUtf8("测试连接"));
        m_testConnectionBtn->setEnabled(true);
        QMessageBox::information(this, QString::fromUtf8("连接成功"),
            QString::fromUtf8("连接成功！\n\n用户名: %1\n姓名: %2")
            .arg(user.username, user.name));
        m_testApi->deleteLater();
        m_testApi = nullptr;
    });
    
    connect(m_testApi, &GitLabApi::networkError, this, [this](const QString& error) {
        m_testConnectionBtn->setText(QString::fromUtf8("测试连接"));
        m_testConnectionBtn->setEnabled(true);
        QMessageBox::critical(this, QString::fromUtf8("连接失败"),
            QString::fromUtf8("网络连接错误: %1").arg(error));
        m_testApi->deleteLater();
        m_testApi = nullptr;
    });
    
    // 发起测试请求
    m_testApi->getCurrentUser();
}

void SettingsDialog::onSave() {
    saveSettings();
    accept();
}

void SettingsDialog::onApply() {
    saveSettings();
    QMessageBox::information(this, QString::fromUtf8("保存成功"),
        QString::fromUtf8("设置已保存！"));
}