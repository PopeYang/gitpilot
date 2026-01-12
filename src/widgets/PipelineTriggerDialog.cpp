#include "PipelineTriggerDialog.h"
#include "service/GitService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>

PipelineTriggerDialog::PipelineTriggerDialog(GitService* gitService, QWidget* parent)
    : QDialog(parent)
    , m_gitService(gitService)
{
    setupUi();
    loadBranches();
}

QString PipelineTriggerDialog::getSelectedBranch() const {
    return m_selectedBranch;
}

void PipelineTriggerDialog::setupUi() {
    setWindowTitle(QString::fromUtf8("选择触发 Pipeline 的分支"));
    setMinimumWidth(400);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 标题说明
    m_titleLabel = new QLabel(QString::fromUtf8("请选择要触发构建的分支："), this);
    m_titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333;");
    mainLayout->addWidget(m_titleLabel);
    
    // Develop 分支快捷按钮
    m_developButton = new QPushButton(QString::fromUtf8("🔷 Develop 分支"), this);
    m_developButton->setMinimumHeight(45);
    m_developButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
    );
    mainLayout->addWidget(m_developButton);
    
    // Internal 分支快捷按钮
    m_internalButton = new QPushButton(QString::fromUtf8("🔶 Internal 分支"), this);
    m_internalButton->setMinimumHeight(45);
    m_internalButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "   padding: 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #E65100;"
        "}"
    );
    mainLayout->addWidget(m_internalButton);
    
    // 其他分支下拉框
    QHBoxLayout* otherLayout = new QHBoxLayout();
    QLabel* otherLabel = new QLabel(QString::fromUtf8("其他分支:"), this);
    otherLabel->setStyleSheet("font-size: 13px; color: #666;");
    otherLabel->setMinimumWidth(70);
    
    m_otherBranchCombo = new QComboBox(this);
    m_otherBranchCombo->setMinimumHeight(35);
    m_otherBranchCombo->setStyleSheet(
        "QComboBox {"
        "   font-size: 13px;"
        "   padding: 5px;"
        "   border: 1px solid #ccc;"
        "   border-radius: 4px;"
        "}"
        "QComboBox:hover {"
        "   border: 1px solid #2196F3;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "}"
    );
    
    otherLayout->addWidget(otherLabel);
    otherLayout->addWidget(m_otherBranchCombo, 1);
    mainLayout->addLayout(otherLayout);
    
    // 分隔线
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line);
    
    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(QString::fromUtf8("取消"), this);
    m_cancelButton->setMinimumWidth(80);
    m_cancelButton->setMinimumHeight(35);
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "   background-color: white;"
        "   color: #333;"
        "   border: 1px solid #ccc;"
        "   font-size: 13px;"
        "   border-radius: 4px;"
        "   padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #f5f5f5;"
        "}"
    );
    
    m_okButton = new QPushButton(QString::fromUtf8("确定"), this);
    m_okButton->setMinimumWidth(80);
    m_okButton->setMinimumHeight(35);
    m_okButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0D47A1;"
        "}"
    );
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    mainLayout->addLayout(buttonLayout);
    
    // 连接信号
    connect(m_developButton, &QPushButton::clicked, this, &PipelineTriggerDialog::onDevelopClicked);
    connect(m_internalButton, &QPushButton::clicked, this, &PipelineTriggerDialog::onInternalClicked);
    connect(m_okButton, &QPushButton::clicked, this, &PipelineTriggerDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &PipelineTriggerDialog::onCancelClicked);
}

void PipelineTriggerDialog::loadBranches() {
    // 填充其他分支下拉框
    QStringList filteredBranches = getFilteredBranches();
    
    if (filteredBranches.isEmpty()) {
        m_otherBranchCombo->addItem(QString::fromUtf8("(无其他可用分支)"));
        m_otherBranchCombo->setEnabled(false);
    } else {
        m_otherBranchCombo->addItem(QString::fromUtf8("-- 请选择 --"));
        m_otherBranchCombo->addItems(filteredBranches);
    }
}

QStringList PipelineTriggerDialog::getFilteredBranches() {
    QStringList allBranches = m_gitService->getAllBranches();
    QStringList filtered;
    
    for (const QString& branch : allBranches) {
        // 排除 main 和 master（生产分支）
        if (branch == "main" || branch == "master") {
            continue;
        }
        // 排除已在快捷按钮中的分支
        if (branch == "develop" || branch == "internal") {
            continue;
        }
        filtered.append(branch);
    }
    
    return filtered;
}

void PipelineTriggerDialog::onDevelopClicked() {
    m_selectedBranch = "develop";
    accept();  // 立即关闭并返回 Accepted
}

void PipelineTriggerDialog::onInternalClicked() {
    m_selectedBranch = "internal";
    accept();  // 立即关闭并返回 Accepted
}

void PipelineTriggerDialog::onOkClicked() {
    // 从下拉框获取选择
    int index = m_otherBranchCombo->currentIndex();
    
    if (index <= 0) {
        // 未选择有效分支
        return;
    }
    
    m_selectedBranch = m_otherBranchCombo->currentText();
    accept();
}

void PipelineTriggerDialog::onCancelClicked() {
    m_selectedBranch.clear();
    reject();
}
