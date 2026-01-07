#include "ProgressDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>

ProgressDialog::ProgressDialog(const QString& title, const QString& command, QWidget* parent)
    : QDialog(parent)
    , m_command(command)
    , m_process(nullptr)
    , m_cancelled(false)
{
    setWindowTitle(title);
    setModal(true);
    resize(600, 400);
    setupUi();
}

void ProgressDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 命令标签
    QLabel* commandLabel = new QLabel(QString::fromUtf8("执行命令: ") + m_command, this);
    commandLabel->setStyleSheet("font-weight: bold; color: #333;");
    mainLayout->addWidget(commandLabel);
    
    // 输出文本框
    m_outputText = new QTextEdit(this);
    m_outputText->setReadOnly(true);
    m_outputText->setFont(QFont("Consolas", 9));
    m_outputText->setStyleSheet(
        "QTextEdit {"
        "   background-color: #1e1e1e;"
        "   color: #d4d4d4;"
        "   border: 1px solid #555;"
        "   padding: 5px;"
        "}"
    );
    mainLayout->addWidget(m_outputText);
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);  // 不确定进度
    m_progressBar->setTextVisible(true);
    m_progressBar->setFormat(QString::fromUtf8("正在执行..."));
    mainLayout->addWidget(m_progressBar);
    
    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_cancelButton = new QPushButton(QString::fromUtf8("取消"), this);
    m_cancelButton->setStyleSheet(
        "QPushButton { background-color: #dc3545; color: white; padding: 5px 15px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #c82333; }"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    
    m_closeButton = new QPushButton(QString::fromUtf8("关闭"), this);
    m_closeButton->setEnabled(false);
    m_closeButton->setStyleSheet(
        "QPushButton { background-color: #28a745; color: white; padding: 5px 15px; border-radius: 3px; }"
        "QPushButton:hover { background-color: #218838; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

bool ProgressDialog::executeCommand(const QString& program, const QStringList& arguments, const QString& workingDir) {
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(workingDir);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    
    connect(m_process, &QProcess::readyReadStandardOutput, this, &ProgressDialog::onProcessReadyReadOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &ProgressDialog::onProcessReadyReadError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProgressDialog::onProcessFinished);
    
    appendOutput(QString::fromUtf8("$ %1 %2\n\n").arg(program, arguments.join(' ')));
    
    m_process->start(program, arguments);
    
    if (!m_process->waitForStarted()) {
        appendError(QString::fromUtf8("错误: 无法启动命令\n"));
        m_progressBar->setFormat(QString::fromUtf8("失败"));
        m_cancelButton->setEnabled(false);
        m_closeButton->setEnabled(true);
        return false;
    }
    
    return true;
}

void ProgressDialog::appendOutput(const QString& text) {
    m_outputText->setTextColor(QColor("#d4d4d4"));
    m_outputText->append(text);
    m_outputText->ensureCursorVisible();
    QApplication::processEvents();  // 保持UI响应
}

void ProgressDialog::appendError(const QString& text) {
    m_outputText->setTextColor(QColor("#f48771"));
    m_outputText->append(text);
    m_outputText->ensureCursorVisible();
    QApplication::processEvents();
}

void ProgressDialog::onProcessReadyReadOutput() {
    if (m_process) {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        if (!output.isEmpty()) {
            appendOutput(output);
        }
    }
}

void ProgressDialog::onProcessReadyReadError() {
    if (m_process) {
        QString error = QString::fromUtf8(m_process->readAllStandardError());
        if (!error.isEmpty()) {
            // Git的进度信息通常在stderr，不一定是错误
            m_outputText->setTextColor(QColor("#569cd6"));
            m_outputText->append(error);
            m_outputText->ensureCursorVisible();
            QApplication::processEvents();
        }
    }
}

void ProgressDialog::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_cancelButton->setEnabled(false);
    m_closeButton->setEnabled(true);
    
    if (m_cancelled) {
        m_progressBar->setFormat(QString::fromUtf8("已取消"));
        appendError(QString::fromUtf8("\n操作已取消\n"));
        emit commandFinished(false);
    } else if (exitStatus == QProcess::CrashExit) {
        m_progressBar->setFormat(QString::fromUtf8("崩溃"));
        appendError(QString::fromUtf8("\n进程崩溃\n"));
        emit commandFinished(false);
    } else if (exitCode != 0) {
        m_progressBar->setFormat(QString::fromUtf8("失败 (退出码: %1)").arg(exitCode));
        appendError(QString::fromUtf8("\n命令执行失败\n"));
        emit commandFinished(false);
    } else {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(100);
        m_progressBar->setFormat(QString::fromUtf8("✓ 完成"));
        appendOutput(QString::fromUtf8("\n✓ 命令执行成功\n"));
        emit commandFinished(true);
    }
}

void ProgressDialog::onCancelClicked() {
    if (m_process && m_process->state() == QProcess::Running) {
        int ret = QMessageBox::question(this, QString::fromUtf8("确认取消"),
            QString::fromUtf8("确定要取消当前操作吗？"),
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            m_cancelled = true;
            m_process->kill();
            appendError(QString::fromUtf8("\n正在取消...\n"));
        }
    }
}
