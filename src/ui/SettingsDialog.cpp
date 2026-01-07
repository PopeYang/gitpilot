#include "SettingsDialog.h"
#include "config/ConfigManager.h"
#include "api/GitLabApi.h"
#include "api/ApiModels.h"
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

SettingsDialog::SettingsDialog(QWidget* parent) 
    : QDialog(parent)
    , m_testApi(nullptr)
{
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    setWindowTitle(QString::fromUtf8("设置"));
    resize(500, 400);  // 缩小窗口以匹配主窗口600px宽度
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // ========== GitLab配置标签页 ==========
    QWidget* gitlabTab = new QWidget(this);
    QVBoxLayout* gitlabLayout = new QVBoxLayout(gitlabTab);
    
    QGroupBox* gitlabGroup = new QGroupBox(QString::fromUtf8("GitLab服务器"), this);
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
    
    QGroupBox* repoGroup = new QGroupBox(QString::fromUtf8("本地仓库"), this);
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
    
    repoLayout->addWidget(repoGroup);
    
    QLabel* repoHint = new QLabel(
        QString::fromUtf8("💡 选择一个有效的Git仓库目录（包含.git文件夹）"),
        this
    );
    repoHint->setStyleSheet("color: #666; font-size: 11px;");
    repoLayout->addWidget(repoHint);
    repoLayout->addStretch();
    
    tabWidget->addTab(repoTab, QString::fromUtf8("仓库"));
    
    // ========== 项目配置标签页 ==========
    QWidget* projectTab = new QWidget(this);
    QVBoxLayout* projectLayout = new QVBoxLayout(projectTab);
    
    QGroupBox* projectGroup = new QGroupBox(QString::fromUtf8("GitLab项目"), this);
    QFormLayout* projectForm = new QFormLayout(projectGroup);
    
    m_projectIdEdit = new QLineEdit(this);
    m_projectIdEdit->setPlaceholderText("123");
    projectForm->addRow(QString::fromUtf8("项目ID:"), m_projectIdEdit);
    
    m_projectNameEdit = new QLineEdit(this);
    m_projectNameEdit->setPlaceholderText(QString::fromUtf8("我的项目"));
    projectForm->addRow(QString::fromUtf8("项目名称:"), m_projectNameEdit);
    
    projectLayout->addWidget(projectGroup);
    
    QLabel* projectHint = new QLabel(
        QString::fromUtf8("💡 项目ID可以在GitLab项目页面的URL中找到"),
        this
    );
    projectHint->setStyleSheet("color: #666; font-size: 11px;");
    projectLayout->addWidget(projectHint);
    projectLayout->addStretch();
    
    tabWidget->addTab(projectTab, QString::fromUtf8("项目"));
    
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
    m_projectIdEdit->setText(config.getCurrentProjectId());
    m_projectNameEdit->setText(config.getCurrentProjectName());
}

void SettingsDialog::saveSettings() {
    ConfigManager& config = ConfigManager::instance();
    
    config.setGitLabUrl(m_gitlabUrlEdit->text().trimmed());
    config.setGitLabToken(m_gitlabTokenEdit->text().trimmed());
    config.setRepoPath(m_repoPathEdit->text().trimmed());
    config.setCurrentProjectId(m_projectIdEdit->text().trimmed());
    config.setCurrentProjectName(m_projectNameEdit->text().trimmed());
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