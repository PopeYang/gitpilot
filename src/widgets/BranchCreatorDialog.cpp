#include "BranchCreatorDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QMessageBox>
#include <QRegularExpressionValidator>

BranchCreatorDialog::BranchCreatorDialog(const QString& baseBranch, QWidget* parent)
    : QDialog(parent)
    , m_selectedType(Feature)
    , m_baseBranch(baseBranch)
{
    setupUi();
}

void BranchCreatorDialog::setupUi() {
    setWindowTitle(QString::fromUtf8("新建分支"));
    resize(500, 350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 分支类型选择
    QGroupBox* typeGroup = new QGroupBox(QString::fromUtf8("请选择分支类型"), this);
    QVBoxLayout* typeLayout = new QVBoxLayout(typeGroup);
    
    m_typeGroup = new QButtonGroup(this);
    
    QRadioButton* databaseRadio = new QRadioButton(QString::fromUtf8("📊 数据库变更"), this);
    QRadioButton* featureRadio = new QRadioButton(QString::fromUtf8("✨ 新功能开发"), this);
    QRadioButton* bugfixRadio = new QRadioButton(QString::fromUtf8("🐛 Bug修复"), this);
    QRadioButton* customRadio = new QRadioButton(QString::fromUtf8("📝 其他（自定义）"), this);
    
    m_typeGroup->addButton(databaseRadio, Database);
    m_typeGroup->addButton(featureRadio, Feature);
    m_typeGroup->addButton(bugfixRadio, Bugfix);
    m_typeGroup->addButton(customRadio, Custom);
    
    featureRadio->setChecked(true);
    
    typeLayout->addWidget(databaseRadio);
    typeLayout->addWidget(featureRadio);
    typeLayout->addWidget(bugfixRadio);
    typeLayout->addWidget(customRadio);
    
    // 只有在develop分支上才能创建数据库分支
    if (m_baseBranch != "develop") {
        databaseRadio->setEnabled(false);
        databaseRadio->setToolTip(QString::fromUtf8("只能在 develop 分支上创建/切换到数据库分支"));
    }
    
    mainLayout->addWidget(typeGroup);
    
    // Git分支名验证器: 只允许字母、数字、连字符、下划线、斜杠
    QRegularExpression branchRegex("^[a-zA-Z0-9/_-]*$");
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(branchRegex, this);
    
    // 输入区域（堆叠widget）
    m_inputStack = new QStackedWidget(this);
    
    // 页面0：数据库分支
    QWidget* databasePage = new QWidget(this);
    QVBoxLayout* dbLayout = new QVBoxLayout(databasePage);
    QLabel* dbLabel = new QLabel(
        QString::fromUtf8("将切换到 develop-database 分支\n\n此分支用于数据库变更，只能向develop合并。"),
        this
    );
    dbLabel->setWordWrap(true);
    dbLayout->addWidget(dbLabel);
    dbLayout->addStretch();
    
    // 页面1：新功能
    QWidget* featurePage = new QWidget(this);
    QVBoxLayout* featureMainLayout = new QVBoxLayout(featurePage);
    
    QLabel* featureLabel = new QLabel(QString::fromUtf8("分支名称:"), this);
    featureMainLayout->addWidget(featureLabel);
    
    QHBoxLayout* featureInputLayout = new QHBoxLayout();
    QLabel* featurePrefix = new QLabel("feature/", this);
    featurePrefix->setStyleSheet("font-weight: bold; color: #2196F3; font-size: 13px;");
    
    m_featureEdit = new QLineEdit(this);
    m_featureEdit->setPlaceholderText("login-module");
    m_featureEdit->setValidator(validator);
    
    featureInputLayout->addWidget(featurePrefix);
    featureInputLayout->addWidget(m_featureEdit, 1);
    featureMainLayout->addLayout(featureInputLayout);
    
    QLabel* featureHint = new QLabel(
        QString::fromUtf8("💡 建议使用小写字母和连字符，例如: user-login"),
        this
    );
    featureHint->setStyleSheet("color: #666; font-size: 11px;");
    featureMainLayout->addWidget(featureHint);
    featureMainLayout->addStretch();
    
    // 页面2：Bug修复
    QWidget* bugfixPage = new QWidget(this);
    QVBoxLayout* bugfixMainLayout = new QVBoxLayout(bugfixPage);
    
    QLabel* bugfixLabel = new QLabel(QString::fromUtf8("分支名称:"), this);
    bugfixMainLayout->addWidget(bugfixLabel);
    
    QHBoxLayout* bugfixInputLayout = new QHBoxLayout();
    QLabel* bugfixPrefix = new QLabel("bugfix/", this);
    bugfixPrefix->setStyleSheet("font-weight: bold; color: #FF9800; font-size: 13px;");
    
    m_bugfixEdit = new QLineEdit(this);
    m_bugfixEdit->setPlaceholderText("crash-on-startup");
    m_bugfixEdit->setValidator(validator);
    
    bugfixInputLayout->addWidget(bugfixPrefix);
    bugfixInputLayout->addWidget(m_bugfixEdit, 1);
    bugfixMainLayout->addLayout(bugfixInputLayout);
    
    QLabel* bugfixHint = new QLabel(
        QString::fromUtf8("💡 建议使用小写字母和连字符，例如: page-crash-fix"),
        this
    );
    bugfixHint->setStyleSheet("color: #666; font-size: 11px;");
    bugfixMainLayout->addWidget(bugfixHint);
    bugfixMainLayout->addStretch();
    
    // 页面3：自定义
    QWidget* customPage = new QWidget(this);
    QFormLayout* customLayout = new QFormLayout(customPage);
    
    m_customEdit = new QLineEdit(this);
    m_customEdit->setPlaceholderText("custom/my-branch");
    m_customEdit->setValidator(validator);
    
    customLayout->addRow(QString::fromUtf8("分支名称:"), m_customEdit);
    
    QLabel* customHint = new QLabel(
        QString::fromUtf8("💡 完整分支名\n只允许: 字母、数字、-、_、/"),
        this
    );
    customHint->setStyleSheet("color: #666; font-size: 11px;");
    customLayout->addRow("", customHint);
    
    m_inputStack->addWidget(databasePage);
    m_inputStack->addWidget(featurePage);
    m_inputStack->addWidget(bugfixPage);
    m_inputStack->addWidget(customPage);
    m_inputStack->setCurrentIndex(Feature);
    
    mainLayout->addWidget(m_inputStack);
    
    // 按钮
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this
    );
    buttonBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("创建"));
    buttonBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &BranchCreatorDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // 连接信号
    connect(m_typeGroup, QOverload<int>::of(&QButtonGroup::idClicked), 
            this, &BranchCreatorDialog::onTypeChanged);
}

void BranchCreatorDialog::onTypeChanged(int id) {
    m_selectedType = static_cast<BranchType>(id);
    m_inputStack->setCurrentIndex(id);
}

void BranchCreatorDialog::onAccept() {
    switch (m_selectedType) {
    case Database:
        m_branchName = "develop-database";
        break;
        
    case Feature:
        m_branchName = m_featureEdit->text().trimmed();
        if (m_branchName.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("输入错误"), 
                QString::fromUtf8("请输入分支名称"));
            return;
        }
        m_branchName = "feature/" + m_branchName;
        break;
        
    case Bugfix:
        m_branchName = m_bugfixEdit->text().trimmed();
        if (m_branchName.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                QString::fromUtf8("请输入分支名称"));
            return;
        }
        m_branchName = "bugfix/" + m_branchName;
        break;
        
    case Custom:
        m_branchName = m_customEdit->text().trimmed();
        if (m_branchName.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("输入错误"),
                QString::fromUtf8("请输入分支名称"));
            return;
        }
        break;
    }
    
    accept();
}
