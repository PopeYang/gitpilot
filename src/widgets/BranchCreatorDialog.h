#ifndef BRANCHCREATORDIALOG_H
#define BRANCHCREATORDIALOG_H

#include <QDialog>

class QButtonGroup;
class QRadioButton;
class QLineEdit;
class QLabel;
class QStackedWidget;

class BranchCreatorDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit BranchCreatorDialog(const QString& baseBranch, QWidget* parent = nullptr);
    
    enum BranchType {
        Database,
        Feature,
        Bugfix,
        Custom
    };
    
    BranchType getSelectedType() const { return m_selectedType; }
    QString getBranchName() const { return m_branchName; }
    
private slots:
    void onTypeChanged(int id);
    void onAccept();
    
private:
    void setupUi();
    
    BranchType m_selectedType;
    QString m_branchName;
    QString m_baseBranch;
    
    QButtonGroup* m_typeGroup;
    QStackedWidget* m_inputStack;
    
    // 输入框
    QLineEdit* m_featureEdit;
    QLineEdit* m_bugfixEdit;
    QLineEdit* m_customEdit;
};

#endif // BRANCHCREATORDIALOG_H
