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
    
    // 使用进度对话框执行clone
    ProgressDialog* progressDlg = new ProgressDialog(
        QString::fromUtf8("正在Clone仓库"),
        QString("git clone %1 %2").arg(url, targetPath),
        this
    );
    
    bool success = false;
    connect(progressDlg, &ProgressDialog::commandFinished, [&success](bool result) {
        success = result;
    });
    
    progressDlg->executeCommand("git", QStringList() << "clone" << url << targetPath, parentDir);
    progressDlg->exec();
    
    if (success) {
        // 自动填充仓库路径
        m_repoPathEdit->setText(targetPath);
        
        QMessageBox::information(this, QString::fromUtf8("Clone成功"),
            QString::fromUtf8("仓库已成功Clone到：\n%1\n\n"
                             "已自动填充到仓库路径").arg(targetPath));
        
        // 自动提取项目信息
        onExtractFromGit();
    }
    
    progressDlg->deleteLater();
}

void SettingsDialog::onBrowseRepoPath() {
    QString dir = QFileDialog::getExistingDirectory(
        this,
        QString::fromUtf8("选择Git仓库目录"),
        m_repoPathEdit->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        // 验证是否是Git仓库
        QFileInfo gitDir(dir + "/.git");
        if (gitDir.exists() && gitDir.isDir()) {
            m_repoPathEdit->setText(dir);
        } else {
            QMessageBox::warning(this, QString::fromUtf8("无效仓库"),
                QString::fromUtf8("所选目录不是有效的Git仓库！\n请确保目录包含.git文件夹。"));
        }
    }
}

void SettingsDialog::onExtractFromGit() {
    QString repoPath = m_repoPathEdit->text().trimmed();
    
    if (repoPath.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
            QString::fromUtf8("请先选择仓库路径"));
        return;
    }
    
    // 使用GitService获取远程URL
    GitService gitService;
    gitService.setRepoPath(repoPath);
    
    QString remoteUrl = gitService.getRemoteUrl().trimmed();
    if (remoteUrl.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
            QString::fromUtf8("无法获取Git远程URL\n请确保仓库已配置远程仓库"));
        return;
    }
    
    // 解析URL: https://gitlab.example.com/yanghaozhe/test.git
    QRegularExpression regex(R"(https?://([^/]+)/(.+?)(?:\.git)?$)");
    QRegularExpressionMatch match = regex.match(remoteUrl);
    
    if (match.hasMatch()) {
        QString server = match.captured(1);
        QString projectPath = match.captured(2);
        
        // 更新项目路径
        m_projectPathEdit->setText(projectPath);
        
        // 从项目路径提取项目名
        QStringList parts = projectPath.split('/');
        if (!parts.isEmpty()) {
            m_projectNameEdit->setText(parts.last());
        }
        
        QMessageBox::information(this, QString::fromUtf8("提取成功"),
            QString::fromUtf8("已从远程URL提取项目信息：\n\n"
                             "服务器: %1\n"
                             "项目路径: %2").arg(server, projectPath));
    } else {
        QMessageBox::warning(this, QString::fromUtf8("解析失败"),
            QString::fromUtf8("无法解析远程URL格式：\n%1\n\n"
                             "期望格式: https://server/namespace/project.git").arg(remoteUrl));
    }
}

void SettingsDialog::onTestConnection() {
    QString url = m_gitlabUrlEdit->text().trimmed();
    QString token = m_gitlabTokenEdit->text().trimmed();
    
    if (url.isEmpty() || token.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("输入错误"),
            QString::fromUtf8("请先填写服务器地址和Token"));
        return;
    }
    
    m_testConnectionBtn->setEnabled(false);
    m_testConnectionBtn->setText(QString::fromUtf8("测试中..."));
    
    // 清理旧的测试实例
    if (m_testApi) {
        m_testApi->deleteLater();
    }
    
    // 创建新的API实例（作为成员变量保持存活）
    m_testApi = new GitLabApi(this);
    m_testApi->setBaseUrl(url);
    m_testApi->setApiToken(token);
    
    // 连接成功信号
    connect(m_testApi, &GitLabApi::userInfoReceived, this,
        [this](const UserInfo& user) {
            QString message = QString::fromUtf8(
                "✅ 连接成功！\n\n"
                "用户: %1 (@%2)\n"
                "邮箱: %3\n"
                "ID: %4"
            ).arg(user.name, user.username, user.email).arg(user.id);
            
            QMessageBox::information(this, QString::fromUtf8("测试成功"), message);
            m_testConnectionBtn->setEnabled(true);
            m_testConnectionBtn->setText(QString::fromUtf8("🔍 测试连接"));
            
            // 清理
            m_testApi->deleteLater();
            m_testApi = nullptr;
        });
    
    // 连接失败信号
    connect(m_testApi, &GitLabApi::networkError, this,
        [this](const QString& error) {
            QMessageBox::warning(this, QString::fromUtf8("连接失败"),
                QString::fromUtf8("无法连接到GitLab：\n\n%1\n\n请检查：\n"
                                 "1. 服务器URL是否正确\n"
                                 "2. Token是否有效\n"
                                 "3. 网络连接").arg(error));
            m_testConnectionBtn->setEnabled(true);
            m_testConnectionBtn->setText(QString::fromUtf8("🔍 测试连接"));
            
            // 清理
            m_testApi->deleteLater();
            m_testApi = nullptr;
        });
    
    connect(m_testApi, &GitLabApi::apiError, this,
        [this](const QString& endpoint, const QString& error) {
            QMessageBox::warning(this, QString::fromUtf8("API错误"),
                QString::fromUtf8("GitLab API调用失败：\n\n%1").arg(error));
            m_testConnectionBtn->setEnabled(true);
            m_testConnectionBtn->setText(QString::fromUtf8("🔍 测试连接"));
            
            // 清理
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