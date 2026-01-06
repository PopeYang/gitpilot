#include "MrZone.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

MrZone::MrZone(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
    , m_isLocked(false)
{
    setupUi();
}

void MrZone::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // MR提交专区标题
    QGroupBox* mrGroup = new QGroupBox(QString::fromUtf8("📤 MR提交专区"), this);
    QVBoxLayout* groupLayout = new QVBoxLayout(mrGroup);
    
    // 表单布局
    QFormLayout* formLayout = new QFormLayout();
    
    // 目标分支选择
    m_targetBranchCombo = new QComboBox(this);
    m_targetBranchCombo->addItem("develop");
    m_targetBranchCombo->addItem("internal");
    formLayout->addRow(QString::fromUtf8("目标分支:"), m_targetBranchCombo);
    
    // MR标题
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(QString::fromUtf8("例如: feat: 添加用户登录功能"));
    formLayout->addRow(QString::fromUtf8("MR标题:"), m_titleEdit);
    
    // MR描述
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText(
        QString::fromUtf8("描述本次修改的内容：\n"
                         "- 实现了什么功能\n"
                         "- 修复了什么问题\n"
                         "- 注意事项等")
    );
    m_descriptionEdit->setMaximumHeight(100);
    formLayout->addRow(QString::fromUtf8("描述:"), m_descriptionEdit);
    
    groupLayout->addLayout(formLayout);
    
    // 提交按钮 - 只负责发起MR
    m_submitButton = new QPushButton(QString::fromUtf8("📤 发起MR"), this);
    m_submitButton->setMinimumHeight(40);
    m_submitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(m_submitButton, &QPushButton::clicked, this, &MrZone::onSubmitClicked);
    groupLayout->addWidget(m_submitButton);
    
    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");
    m_statusLabel->setWordWrap(true);
    groupLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(mrGroup);
    mainLayout->addStretch();
}

void MrZone::updateForBranch(const QString& currentBranch) {
    m_currentBranch = currentBranch;
    
    // QC关键防呆逻辑：develop-database分支只能向develop合并
    if (currentBranch == "develop-database") {
        lockTargetBranch("develop");
        m_statusLabel->setText(
            QString::fromUtf8("⚠️ 数据库分支只能向 develop 合并（已锁定）")
        );
        m_statusLabel->setStyleSheet("color: #FF9800; font-size: 11px; font-weight: bold;");
    } else {
        unlockTargetBranch();
        m_statusLabel->setText(
            QString::fromUtf8("💡 请选择目标分支并填写MR信息")
        );
        m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");
    }
}

void MrZone::lockTargetBranch(const QString& branch) {
    m_targetBranchCombo->clear();
    m_targetBranchCombo->addItem(branch);
    m_targetBranchCombo->setEnabled(false);
    m_targetBranchCombo->setStyleSheet("background-color: #FFE6E6;");
    m_isLocked = true;
}

void MrZone::unlockTargetBranch() {
    m_targetBranchCombo->clear();
    m_targetBranchCombo->addItem("develop");
    m_targetBranchCombo->addItem("internal");
    m_targetBranchCombo->setEnabled(true);
    m_targetBranchCombo->setStyleSheet("");
    m_isLocked = false;
}

void MrZone::onSubmitClicked() {
    // 验证输入
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("输入错误"),
            QString::fromUtf8("请输入MR标题"));
        return;
    }
    
    QString description = m_descriptionEdit->toPlainText().trimmed();
    if (description.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("输入错误"),
            QString::fromUtf8("请输入MR描述"));
        return;
    }
    
    QString targetBranch = m_targetBranchCombo->currentText();
    
    // 确认对话框
    QString confirmMsg = QString::fromUtf8(
        "即将创建MR：\n\n"
        "源分支: %1\n"
        "目标分支: %2\n"
        "标题: %3\n\n"
        "确认继续？\n\n"
        "💡 提示：请确保代码已提交并推送到远程仓库"
    ).arg(m_currentBranch, targetBranch, title);
    
    int ret = QMessageBox::question(this, QString::fromUtf8("确认提交"),
        confirmMsg,
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // 发送信号通知父组件执行工作流
        emit mrSubmitted(targetBranch, title, description);
    }
}