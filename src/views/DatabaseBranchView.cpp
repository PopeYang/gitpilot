#include "DatabaseBranchView.h"
#include "service/GitService.h"
#include "api/GitLabApi.h"
#include "widgets/MrZone.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>

DatabaseBranchView::DatabaseBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent)
    : QWidget(parent)
    , m_gitService(gitService)
    , m_gitLabApi(gitLabApi)
{
    setupUi();
    connectSignals();
}

void DatabaseBranchView::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // 顶部警告区域
    QGroupBox* warningGroup = new QGroupBox(QString::fromUtf8("🟣 数据库变更专区"), this);
    warningGroup->setStyleSheet(
        "QGroupBox {"
        "   background-color: #F3E5F5;"
        "   border: 2px solid #9C27B0;"
        "   border-radius: 8px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   padding: 10px;"
        "}"
        "QGroupBox::title {"
        "   color: #6A1B9A;"
        "}"
    );
    
    QVBoxLayout* warningLayout = new QVBoxLayout(warningGroup);
    m_warningLabel = new QLabel(
        QString::fromUtf8("⚠️ 此分支仅用于数据库迁移脚本\n\n"
                         "• 仅可合并到 develop 分支\n"
                         "• 建议遵循数据库变更规范\n"
                         "• 提交前检查脚本可回滚性"), 
        this);
    m_warningLabel->setStyleSheet("color: #6A1B9A; font-size: 13px; background: transparent; border: none;");
    m_warningLabel->setWordWrap(true);
    warningLayout->addWidget(m_warningLabel);
    
    mainLayout->addWidget(warningGroup);
    
    // 文件状态区域
    QGroupBox* filesGroup = new QGroupBox(QString::fromUtf8("📂 待提交文件"), this);
    filesGroup->setStyleSheet("QGroupBox { font-size: 13px; font-weight: bold; padding: 10px; }");
    
    QVBoxLayout* filesLayout = new QVBoxLayout(filesGroup);
    
    m_filesListWidget = new QListWidget(this);
    m_filesListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    m_filesListWidget->setMaximumHeight(200);
    m_filesListWidget->setStyleSheet(
        "QListWidget {"
        "   border: 1px solid #ddd;"
        "   border-radius: 4px;"
        "   background-color: white;"
        "   font-size: 12px;"
        "}"
        "QListWidget::item {"
        "   padding: 5px;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #E1BEE7;"
        "   color: black;"
        "}"
    );
    filesLayout->addWidget(m_filesListWidget);
    
    // 文件操作按钮
    QHBoxLayout* fileButtonsLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_stageAllButton = new QPushButton(QString::fromUtf8("📋 全部暂存"), this);
    
    m_refreshButton->setStyleSheet(
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
    );
    
    m_stageAllButton->setStyleSheet(
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
    );
    
    fileButtonsLayout->addWidget(m_refreshButton);
    fileButtonsLayout->addWidget(m_stageAllButton);
    filesLayout->addLayout(fileButtonsLayout);
    
    mainLayout->addWidget(filesGroup);
    
    // 提交操作区域
    QGroupBox* commitGroup = new QGroupBox(QString::fromUtf8("📝 提交操作"), this);
    commitGroup->setStyleSheet("QGroupBox { font-size: 13px; font-weight: bold; padding: 10px; }");
    
    QVBoxLayout* commitLayout = new QVBoxLayout(commitGroup);
    
    QHBoxLayout* commitButtonsLayout = new QHBoxLayout();
    
    m_commitButton = new QPushButton(QString::fromUtf8("✅ 提交"), this);
    m_commitButton->setMinimumHeight(40);
    m_commitButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #FF9800;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F57C00;"
        "}"
    );
    
    m_pushButton = new QPushButton(QString::fromUtf8("⬆️ 推送"), this);
    m_pushButton->setMinimumHeight(40);
    m_pushButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #9C27B0;"
        "   color: white;"
        "   font-size: 13px;"
        "   font-weight: bold;"
        "   border: none;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #7B1FA2;"
        "}"
    );
    
    commitButtonsLayout->addWidget(m_commitButton);
    commitButtonsLayout->addWidget(m_pushButton);
    commitLayout->addLayout(commitButtonsLayout);
    
    mainLayout->addWidget(commitGroup);
    
    // MR提交专区（目标锁定为develop）
    m_mrZone = new MrZone(m_gitService, m_gitLabApi, this);
    mainLayout->addWidget(m_mrZone);
    
    mainLayout->addStretch();
}

void DatabaseBranchView::connectSignals() {
    connect(m_refreshButton, &QPushButton::clicked, this, &DatabaseBranchView::onRefreshClicked);
    connect(m_stageAllButton, &QPushButton::clicked, this, &DatabaseBranchView::onStageAllClicked);
    connect(m_commitButton, &QPushButton::clicked, this, &DatabaseBranchView::onCommitClicked);
    connect(m_pushButton, &QPushButton::clicked, this, &DatabaseBranchView::onPushClicked);
    
    // MR Zone信号
    connect(m_mrZone, &MrZone::conflictCheckRequested, this, &DatabaseBranchView::onConflictCheckRequested);
    connect(m_mrZone, &MrZone::mrSubmitted, this, &DatabaseBranchView::onMrSubmitted);
}

void DatabaseBranchView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    updateFileList();
    updateMrZone();
}

void DatabaseBranchView::updateFileList() {
    m_filesListWidget->clear();
    
    QList<FileStatus> fileStatuses = m_gitService->getFileStatus();
    
    if (fileStatuses.isEmpty()) {
        m_filesListWidget->addItem(QString::fromUtf8("💚 工作区干净"));
    } else {
        for (const FileStatus& fs : fileStatuses) {
            m_filesListWidget->addItem(fs.displayText);
        }
    }
}

void DatabaseBranchView::updateMrZone() {
    QString currentBranch = m_gitService->getCurrentBranch();
    m_mrZone->updateForBranch(currentBranch);
}

void DatabaseBranchView::onRefreshClicked() {
    updateFileList();
}

void DatabaseBranchView::onStageAllClicked() {
    if (m_gitService->stageAll()) {
        QMessageBox::information(this, QString::fromUtf8("暂存成功"),
            QString::fromUtf8("✅ 所有修改已暂存"));
        updateFileList();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("暂存失败"),
            QString::fromUtf8("暂存文件失败"));
    }
}

void DatabaseBranchView::onCommitClicked() {
    bool ok;
    QString message = QInputDialog::getText(this, QString::fromUtf8("提交信息"),
        QString::fromUtf8("请输入提交信息："), QLineEdit::Normal, QString(), &ok);
    
    if (!ok || message.trimmed().isEmpty()) {
        return;
    }
    
    if (m_gitService->commit(message)) {
        QMessageBox::information(this, QString::fromUtf8("提交成功"),
            QString::fromUtf8("✅ 代码已提交到本地仓库"));
        updateFileList();
    } else {
        QMessageBox::warning(this, QString::fromUtf8("提交失败"),
            QString::fromUtf8("提交失败，请检查是否有文件已暂存"));
    }
}

void DatabaseBranchView::onPushClicked() {
    QString currentBranch = m_gitService->getCurrentBranch();
    
    int ret = QMessageBox::question(
        this,
        QString::fromUtf8("确认推送"),
        QString::fromUtf8("确认要推送 %1 分支到远程仓库？").arg(currentBranch),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在推送到远程仓库..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("推送中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(this);
    
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, progress]() {
        bool success = watcher->result();
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (success) {
            QMessageBox::information(this, QString::fromUtf8("推送成功"),
                QString::fromUtf8("✅ 代码已成功推送到远程仓库"));
        } else {
            QMessageBox::warning(this, QString::fromUtf8("推送失败"),
                QString::fromUtf8("推送失败，请检查网络连接和权限"));
        }
    });
    
    QFuture<bool> future = QtConcurrent::run([this, currentBranch]() {
        return m_gitService->pushBranch(currentBranch, true);
    });
    
    watcher->setFuture(future);
}

void DatabaseBranchView::onConflictCheckRequested(const QString& targetBranch) {
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在检查冲突..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("检查中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    
    typedef QPair<bool, QString> CheckResult;
    QFutureWatcher<CheckResult>* watcher = new QFutureWatcher<CheckResult>(this);
    
    connect(watcher, &QFutureWatcher<CheckResult>::finished, this, [this, watcher, progress, targetBranch]() {
        CheckResult result = watcher->result();
        bool hasNoConflict = result.first;
        QString conflictInfo = result.second;
        
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        
        if (hasNoConflict) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("检查完成"));
            msgBox.setText(QString::fromUtf8("✅ ") + conflictInfo + QString::fromUtf8("\n\n可以继续发起合并请求。"));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setMinimumWidth(255);
            
            QPushButton* mergeBtn = msgBox.addButton(QString::fromUtf8("发起合并"), QMessageBox::AcceptRole);
            msgBox.addButton(QMessageBox::Close);
            
            msgBox.exec();
            
            if (msgBox.clickedButton() == mergeBtn) {
                if (m_mrZone) {
                    m_mrZone->triggerSubmit();
                }
            }
        } else {
            QString helpText = QString::fromUtf8(
                "\n\n🛠️ 如何解决冲突：\n"
                "1. 在本地终端运行：\n   git pull origin %1\n"
                "2. 打开IDE解决冲突文件\n"
                "3. 提交修改并再次推送"
            ).arg(targetBranch);
            
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("发现冲突"));
            msgBox.setText(conflictInfo + helpText);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setMinimumWidth(255);
            msgBox.exec();
        }
    });
    
    QFuture<CheckResult> future = QtConcurrent::run([this, targetBranch]() {
        QString info;
        bool result = m_gitService->checkMergeConflict(targetBranch, info);
        return qMakePair(result, info);
    });
    
    watcher->setFuture(future);
}

void DatabaseBranchView::onMrSubmitted(const QString& targetBranch, const QString& title, const QString& description) {
    QString sourceBranch = m_gitService->getCurrentBranch();
    
    MrParams params;
    params.sourceBranch = sourceBranch;
    params.targetBranch = targetBranch;
    params.title = title;
    params.description = description;
    params.removeSourceBranch = false;
    params.squash = false;
    
    QProgressDialog* progress = new QProgressDialog(
        QString::fromUtf8("正在创建合并请求..."), 
        QString(), 0, 0, this);
    progress->setWindowTitle(QString::fromUtf8("提交中"));
    progress->setMinimumWidth(255);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->setCancelButton(nullptr);
    progress->setValue(0);
    progress->show();
    QApplication::processEvents();
    
    connect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, 
        [this, progress](const MrResponse& mr) {
            progress->close();
            progress->deleteLater();
            
            QString message = QString(
                "<h3 style='color: green;'>✅ 合并请求创建成功！</h3>"
                "<p><b>编号:</b> %1</p>"
                "<p><b>标题:</b> %2</p>"
                "<p><b>状态:</b> %3</p>"
                "<p><b>链接:</b> ⬇️⬇️⬇️ <br>"
                "<a href='%4'>%4</a></p>"
                "<p style='color: #666; font-size: 11px;'>💡 点击链接在浏览器中查看合并请求详情</p>"
            ).arg(mr.iid).arg(mr.title, mr.state, mr.webUrl);
            
            QMessageBox msgBox(this);
            msgBox.setWindowTitle(QString::fromUtf8("合并请求创建成功"));
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setText(message);
            msgBox.setIcon(QMessageBox::NoIcon);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setMinimumWidth(255);
            msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
            msgBox.exec();
            
            disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
            disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
        });
    
    connect(m_gitLabApi, &GitLabApi::apiError, this,
        [this, progress](const QString& endpoint, const QString& errorMessage) {
            if (endpoint.contains("merge_request")) {
                progress->close();
                progress->deleteLater();
                
                QString userMessage;
                if (errorMessage.contains("409")) {
                    userMessage = QString::fromUtf8(
                        "⚠️ MR已存在\n\n"
                        "该分支的MR可能已经创建过了。\n\n"
                        "详细错误：\n%1"
                    ).arg(errorMessage);
                } else {
                    userMessage = QString::fromUtf8("创建MR失败：\n\n%1").arg(errorMessage);
                }
                
                QMessageBox::warning(this, QString::fromUtf8("创建失败"), userMessage);
                
                disconnect(m_gitLabApi, &GitLabApi::mergeRequestCreated, this, nullptr);
                disconnect(m_gitLabApi, &GitLabApi::apiError, this, nullptr);
            }
        });
    
    m_gitLabApi->createMergeRequest(params);
}