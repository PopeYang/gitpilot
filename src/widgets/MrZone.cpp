#include "MrZone.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include "api/ApiModels.h"  // 新增：为 ProjectMember
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>  // 新增
#include <QMessageBox>
#include <QEvent>  // 新增：事件过滤
#include <QMouseEvent>  // 新增：鼠标事件
#include <QCursor>  // 新增：光标位置
#include <QApplication>  // 新增：qApp
#include <QTimer>  // 新增：延迟更新
#include <QStyle>
#include <QStyleOption>

MrZone::MrZone(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
    , m_isLocked(false)
{
    setupUi();
    
    // 连接成员列表信号
    connect(m_gitLabApi, &GitLabApi::projectMembersReceived,
            this, &MrZone::onProjectMembersReceived);
    
    // 加载项目成员
    loadProjectMembers();
    
    // 安装应用级事件过滤器，用于检测外部点击
    qApp->installEventFilter(this);
    
    // 初始化箭头样式
    setArrowState(false);
}

void MrZone::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // MR提交专区标题
    QGroupBox* mrGroup = new QGroupBox(QString::fromUtf8("📤 创建合并请求"), this);
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
    formLayout->addRow(QString::fromUtf8("合并标题:"), m_titleEdit);
    
    // MR描述
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setPlaceholderText(
        QString::fromUtf8("描述本次修改的内容：\n"
                         "- 实现了什么功能\n"
                         "- 修复了什么问题\n"
                         "- 注意事项等")
    );
    m_descriptionEdit->setMinimumHeight(100);
    formLayout->addRow(QString::fromUtf8("修改内容:"), m_descriptionEdit);
    
    // 新增：审核人选择（下拉框式）
    QHBoxLayout* assigneeLayout = new QHBoxLayout();
    
    // 下拉框（显示已选中的审核人）
    m_assigneeCombo = new QComboBox(this);
    m_assigneeCombo->setEditable(true);
    m_assigneeCombo->lineEdit()->setPlaceholderText(QString::fromUtf8("点击选择审核人..."));
    m_assigneeCombo->lineEdit()->setReadOnly(true);  // 只能点击，不能输入
    
    // 创建带复选框的列表
    m_assigneeList = new QListWidget();
    // 使用 Tool 而不是 Popup，因为 Popup 点击外部会自动关闭，导致我们的 Toggle 逻辑失效（关了又开）
    // FramelessWindowHint 去掉窗口边框
    m_assigneeList->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_assigneeList->setFocusPolicy(Qt::NoFocus);
    m_assigneeList->setMouseTracking(true);
    
    // 安装事件过滤器到组合框、输入框和列表视口
    m_assigneeCombo->installEventFilter(this);
    m_assigneeCombo->lineEdit()->installEventFilter(this);
    m_assigneeList->viewport()->installEventFilter(this);
    
    // 禁用默认选择高亮（只用复选框）
    m_assigneeList->setSelectionMode(QAbstractItemView::NoSelection);
    
    // 使用 itemChanged 信号更新文本（更可靠，处理所有状态变更）
    connect(m_assigneeList, &QListWidget::itemChanged, this, [this](QListWidgetItem*) {
        updateAssigneeComboText();
    });

    
    assigneeLayout->addWidget(m_assigneeCombo);
    
    // 刷新按钮
    QPushButton* refreshButton = new QPushButton(QString::fromUtf8("🔄"), this);
    refreshButton->setMaximumWidth(35);
    refreshButton->setToolTip(QString::fromUtf8("刷新成员列表"));
    connect(refreshButton, &QPushButton::clicked, this, &MrZone::loadProjectMembers);
    assigneeLayout->addWidget(refreshButton);
    
    formLayout->addRow(QString::fromUtf8("指派审核人:"), assigneeLayout);
    
    groupLayout->addLayout(formLayout);
    
    // 按钮区域 - 两个按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // 检查冲突按钮
    m_checkConflictButton = new QPushButton(QString::fromUtf8("🔍 检查冲突"), this);
    m_checkConflictButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #2196F3;"
        "   color: white;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #0b7dda;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #0a6bc5;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(m_checkConflictButton, &QPushButton::clicked, this, &MrZone::onCheckConflictClicked);
    
    // 发起合并按钮
    m_submitButton = new QPushButton(QString::fromUtf8("📤 发起合并"), this);
    m_submitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #4CAF50;"
        "   color: white;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 4px;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "QPushButton:pressed {"
        "   background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #cccccc;"
        "   color: #666666;"
        "}"
    );
    connect(m_submitButton, &QPushButton::clicked, this, &MrZone::onSubmitClicked);
    
    buttonLayout->addWidget(m_checkConflictButton);
    buttonLayout->addWidget(m_submitButton);
    groupLayout->addLayout(buttonLayout);
    
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
    
    // 重新加载成员列表（以防成员变化）
    loadProjectMembers();
    
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

void MrZone::triggerSubmit() {
    onSubmitClicked();
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

void MrZone::onCheckConflictClicked() {
    QString targetBranch = m_targetBranchCombo->currentText();
    
    // 确认对话框
    int ret = QMessageBox::question(this, QString::fromUtf8("检查冲突"),
        QString::fromUtf8("将执行以下操作：\n\n"
                         "1. fetch远程%1分支\n"
                         "2. 尝试合并到当前分支（不提交）\n"
                         "3. 检测是否有冲突\n\n"
                         "确认继续？").arg(targetBranch),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 发射信号给父组件处理
    emit conflictCheckRequested(targetBranch);
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
    
    // 获取选中的审核人（从复选框）
    QList<int> assigneeIds;
    QStringList assigneeNames;
    for (int i = 0; i < m_assigneeList->count(); ++i) {
        QListWidgetItem* item = m_assigneeList->item(i);
        if (item->checkState() == Qt::Checked) {
            assigneeIds.append(item->data(Qt::UserRole).toInt());
            // 提取名字部分
            QString fullText = item->text();
            QString name = fullText.split(" (").first();
            assigneeNames.append(name);
        }
    }
    
    // 确认对话框
    QString confirmMsg = QString::fromUtf8(
        "即将创建MR：\n\n"
        "源分支: %1\n"
        "目标分支: %2\n"
        "标题: %3\n"
        "审核人: %4\n\n"
        "确认继续？\n\n"
        "💡 提示：请确保代码已提交并推送到远程仓库"
    ).arg(m_currentBranch, targetBranch, title,
          assigneeNames.isEmpty() ? "无" : assigneeNames.join(", "));
    
    int ret = QMessageBox::question(this, QString::fromUtf8("确认提交"),
        confirmMsg,
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // 直接创建MR，包含审核人
        MrParams params;
        params.sourceBranch = m_currentBranch;
        params.targetBranch = targetBranch;
        params.title = title;
        params.description = description;
        params.assigneeIds = assigneeIds;
        
        m_gitLabApi->createMergeRequest(params);
    }
}

void MrZone::loadProjectMembers() {
    m_gitLabApi->listProjectMembers();
}

void MrZone::onProjectMembersReceived(const QList<ProjectMember>& members) {
    m_projectMembers = members;
    m_assigneeList->clear();
    
    // 添加带复选框的成员项
    for (const ProjectMember& member : members) {
        QListWidgetItem* item = new QListWidgetItem(m_assigneeList);
        item->setText(QString("%1 (%2)").arg(member.name, member.username));
        item->setData(Qt::UserRole, member.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        m_assigneeList->addItem(item);
    }
    
    updateAssigneeComboText();
}

void MrZone::updateAssigneeComboText() {
    QStringList selectedNames;
    
    for (int i = 0; i < m_assigneeList->count(); ++i) {
        QListWidgetItem* item = m_assigneeList->item(i);
        if (item->checkState() == Qt::Checked) {
            // 只显示名字部分，不显示用户名
            QString fullText = item->text();
            QString name = fullText.split(" (").first();
            selectedNames.append(name);
        }
    }
    
    if (selectedNames.isEmpty()) {
        m_assigneeCombo->lineEdit()->clear();
        m_assigneeCombo->lineEdit()->setPlaceholderText(QString::fromUtf8("点击选择审核人..."));
    } else {
        m_assigneeCombo->lineEdit()->setText(selectedNames.join(", "));
    }
}

bool MrZone::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        // 1. 处理列表视口点击（实现全行点击勾选）
        if (obj == m_assigneeList->viewport()) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QListWidgetItem* item = m_assigneeList->itemAt(me->pos());
            if (item) {
                // 检查是否点击了复选框区域（如果是，让Qt自己处理）
                QStyleOptionViewItem option;
                option.initFrom(m_assigneeList);
                option.rect = m_assigneeList->visualItemRect(item);
                option.features |= QStyleOptionViewItem::HasCheckIndicator;
                option.viewItemPosition = QStyleOptionViewItem::Middle;
                
                QRect checkRect = m_assigneeList->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, m_assigneeList);
                
                if (!checkRect.contains(me->pos())) {
                    // 点击了文字区域 -> 手动切换状态
                    bool checked = (item->checkState() == Qt::Checked);
                    item->setCheckState(checked ? Qt::Unchecked : Qt::Checked);
                    return true;
                }
            }
        }
        
        // 2. 处理下拉框点击（Toggle）
        if (obj == m_assigneeCombo || obj == m_assigneeCombo->lineEdit()) {
            if (m_assigneeList->isVisible()) {
                hideAssigneePopup();
            } else {
                showAssigneePopup();
            }
            return true;
        }
        
        // 3. 处理外部点击（自动隐藏）
        if (m_assigneeList->isVisible()) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QPoint globalPos = mouseEvent->globalPosition().toPoint();
            
            bool inList = m_assigneeList->geometry().contains(globalPos);
            bool inCombo = m_assigneeCombo->rect().contains(m_assigneeCombo->mapFromGlobal(globalPos));
            
            if (!inList && !inCombo) {
                hideAssigneePopup();
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void MrZone::showAssigneePopup() {
    if (m_assigneeList->count() == 0) {
        return;
    }
    
    // 计算动态高度 (最多显示8行)
    int rowHeight = m_assigneeList->sizeHintForRow(0);
    if (rowHeight <= 0) rowHeight = 30; // 默认高度
    
    int totalItems = m_assigneeList->count();
    int visibleItems = (totalItems > 8) ? 8 : totalItems;
    // +4px 用于边框/Padding余量
    int listHeight = visibleItems * rowHeight + 4; 
    
    // 定位到下拉框下方
    QPoint pos = m_assigneeCombo->mapToGlobal(QPoint(0, m_assigneeCombo->height()));
    m_assigneeList->move(pos);
    m_assigneeList->setFixedWidth(m_assigneeCombo->width());
    m_assigneeList->setFixedHeight(listHeight);
    m_assigneeList->show();
    m_assigneeList->raise();
    m_assigneeList->activateWindow();
    
    setArrowState(true); // 显示上三角
}

void MrZone::hideAssigneePopup() {
    m_assigneeList->hide();
    setArrowState(false); // 显示下三角
}

void MrZone::setArrowState(bool isUp) {
    // 简单的 SVG Base64 图标 (灰色 Stroke) - 添加单引号
    QString downArrow = "url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCI+PHBhdGggZD0iTTYgOWw2IDYgNi02Ii8+PC9zdmc+')";
    QString upArrow = "url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0ibm9uZSIgc3Ryb2tlPSIjNjY2IiBzdHJva2Utd2lkdGg9IjIiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIgc3Ryb2tlLWxpbmVqb2luPSJyb3VuZCI+PHBhdGggZD0iTTE4IDE1bC02LTYtNiA2Ii8+PC9zdmc+')";

    QString style = QString(
        "QComboBox::down-arrow { "
        "   image: %1; "
        "   width: 14px; height: 14px; "
        "}"
        "QComboBox::drop-down { "
        "   border: none; "
        "   subcontrol-origin: padding;"
        "   subcontrol-position: top right;"
        "   width: 20px; "
        "}"
    ).arg(isUp ? upArrow : downArrow);
    
    m_assigneeCombo->setStyleSheet(style);
}
