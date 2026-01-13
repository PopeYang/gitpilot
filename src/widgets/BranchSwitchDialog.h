#ifndef BRANCHSWITCHDIALOG_H
#define BRANCHSWITCHDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;
class QPushButton;

class BranchSwitchDialog : public QDialog {
    Q_OBJECT

public:
    explicit BranchSwitchDialog(const QString& currentBranch, 
                                const QStringList& allBranches, 
                                const QString& databaseBranchName = "develop-database",
                                QWidget* parent = nullptr);
    
    QString getTargetBranch() const;

private:
    void setupUi();
    
    QString m_currentBranch;
    QStringList m_allBranches;
    QString m_databaseBranchName; // 动态的数据库分支名
    
    QString m_selectedBranch;
    
    QComboBox* m_otherBranchCombo;
};

#endif // BRANCHSWITCHDIALOG_H
