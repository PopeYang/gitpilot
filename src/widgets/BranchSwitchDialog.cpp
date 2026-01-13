#include "BranchSwitchDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QSet>

BranchSwitchDialog::BranchSwitchDialog(const QString& currentBranch, 
                                       const QStringList& allBranches, 
                                       const QString& databaseBranchName,
                                       QWidget* parent)
    : QDialog(parent)
    , m_currentBranch(currentBranch)
    , m_allBranches(allBranches)
    , m_databaseBranchName(databaseBranchName)
{
    setWindowTitle(QString::fromUtf8("切换分支"));
    setMinimumWidth(380);
    setMaximumWidth(450);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    // 修正后的调用
    setupUi();
}

QString BranchSwitchDialog::getTargetBranch() const {
    return m_selectedBranch;
}

void BranchSwitchDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 指导语
    QLabel* tipLabel = new QLabel(QString::fromUtf8("当前位于: <b>%1</b>").arg(m_currentBranch), this);
    tipLabel->setStyleSheet("font-size: 14px; color: #555;");
    mainLayout->addWidget(tipLabel);
    
    // 1. 核心分支区域 (固定按钮)
    QGroupBox* coreGroup = new QGroupBox(QString::fromUtf8("🚀 常用核心分支"), this);
    coreGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #333; }");
    
    QGridLayout* coreLayout = new QGridLayout(coreGroup);
    coreLayout->setSpacing(10);
    
    // 定义核心分支列表 (优先级排序)
    QStringList coreBranches;
    // 检测本地有哪些核心分支
    if (m_allBranches.contains("main")) coreBranches << "main";
    if (m_allBranches.contains("master")) coreBranches << "master";
    if (m_allBranches.contains("develop")) coreBranches << "develop";
    if (m_allBranches.contains("internal")) coreBranches << "internal";
    if (!m_databaseBranchName.isEmpty() && m_allBranches.contains(m_databaseBranchName)) {
        if (!coreBranches.contains(m_databaseBranchName)) {
             coreBranches << m_databaseBranchName;
        }
    }
    
    if (coreBranches.isEmpty()) {
        QLabel* emptyLabel = new QLabel(QString::fromUtf8("未检测到标准核心分支"), this);
        emptyLabel->setStyleSheet("color: #999;");
        coreLayout->addWidget(emptyLabel, 0, 0);
    }
    
    int row = 0;
    int col = 0;
    
    for (const QString& branch : coreBranches) {
        QPushButton* btn = new QPushButton(branch, this);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(36);
        
        // 样式：区分当前分支和目标分支
        if (branch == m_currentBranch) {
            // 当前分支
            btn->setStyleSheet(
                "QPushButton {"
                "   background-color: #E0E0E0;"
                "   border: 2px solid #9E9E9E;"
                "   color: #666;"
                "   border-radius: 5px;"
                "   font-weight: bold;"
                "}"
            );
            btn->setToolTip(QString::fromUtf8("这是当前所在的分支"));
            btn->setEnabled(false); // 禁用点击
            btn->setText(branch + QString::fromUtf8(" (当前)"));
        } else {
            // 目标分支
            btn->setStyleSheet(
                "QPushButton {"
                "   background-color: #E3F2FD;"
                "   border: 1px solid #2196F3;"
                "   color: #1565C0;"
                "   border-radius: 5px;"
                "   font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "   background-color: #BBDEFB;"
                "   border: 2px solid #1976D2;"
                "}"
                "QPushButton:pressed {"
                "   background-color: #90CAF9;"
                "}"
            );
        }
        
        connect(btn, &QPushButton::clicked, this, [this, branch]() {
            m_selectedBranch = branch;
            accept();
        });
        
        coreLayout->addWidget(btn, row, col);
        
        // 每行2个按钮
        col++;
        if (col > 1) {
            col = 0;
            row++;
        }
    }
    
    mainLayout->addWidget(coreGroup);
    
    // 2. 其他分支区域 (下拉选择)
    QGroupBox* otherGroup = new QGroupBox(QString::fromUtf8("🌿 其他开发分支"), this);
    otherGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #333; }");
    QHBoxLayout* otherLayout = new QHBoxLayout(otherGroup);
    
    m_otherBranchCombo = new QComboBox(this);
    m_otherBranchCombo->setEditable(true); // 允许搜索
    m_otherBranchCombo->setPlaceholderText(QString::fromUtf8("🔍 搜索或选择分支..."));
    m_otherBranchCombo->setMinimumHeight(30);
    
    // 过滤掉已经在上面的核心分支
    QSet<QString> coreSet(coreBranches.begin(), coreBranches.end());
    
    int addedCount = 0;
    for (const QString& branch : m_allBranches) {
        if (!coreSet.contains(branch)) {
             m_otherBranchCombo->addItem(branch);
             addedCount++;
        }
    }
    
    // 如果没有其他分支
    if (addedCount == 0) {
        m_otherBranchCombo->addItem(QString::fromUtf8("(无其他分支)"));
        m_otherBranchCombo->setEnabled(false);
    } else {
        // 尝试恢复选中上次的（如果有记忆的话，这里没有）
        // 如果当前分支在下拉中
        int idx = m_otherBranchCombo->findText(m_currentBranch);
        if (idx >= 0) m_otherBranchCombo->setCurrentIndex(idx);
        else m_otherBranchCombo->setCurrentIndex(-1);
    }

    QPushButton* switchBtn = new QPushButton(QString::fromUtf8("切换"), this);
    switchBtn->setCursor(Qt::PointingHandCursor);
    switchBtn->setMinimumHeight(30);
    switchBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 0 20px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #ccc;"
        "}"
    );
    
    // 如果没有其他分支，禁用切换按钮
    if (addedCount == 0) switchBtn->setEnabled(false);
    
    connect(switchBtn, &QPushButton::clicked, this, [this]() {
        QString txt = m_otherBranchCombo->currentText();
        if (txt.isEmpty() || txt == QString::fromUtf8("(无其他分支)")) return;
        
        m_selectedBranch = txt;
        accept();
    });
    
    otherLayout->addWidget(m_otherBranchCombo, 1);
    otherLayout->addWidget(switchBtn);
    
    mainLayout->addWidget(otherGroup);
    
    // 底部取消
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();
    QPushButton* cancelBtn = new QPushButton(QString::fromUtf8("取消"), this);
    cancelBtn->setMinimumHeight(28);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(cancelBtn);
    
    mainLayout->addLayout(bottomLayout);
}
