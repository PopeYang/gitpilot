#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QProcess>

/**
 * @brief 实时显示Git命令执行进度的对话框
 */
class ProgressDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit ProgressDialog(const QString& title, const QString& command, QWidget* parent = nullptr);
    
    // 开始执行命令
    bool executeCommand(const QString& program, const QStringList& arguments, const QString& workingDir);
    
    // 添加输出文本
    void appendOutput(const QString& text);
    void appendError(const QString& text);
    
signals:
    void commandFinished(bool success);
    
private slots:
    void onProcessReadyReadOutput();
    void onProcessReadyReadError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onCancelClicked();
    
private:
    void setupUi();
    
    QString m_command;
    QTextEdit* m_outputText;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    QPushButton* m_closeButton;
    QProcess* m_process;
    bool m_cancelled;
};

#endif // PROGRESSDIALOG_H
