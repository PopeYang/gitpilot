#ifndef FIRSTRUNWIZARD_H
#define FIRSTRUNWIZARD_H

#include <QWizard>

class FirstRunWizard : public QWizard {
    Q_OBJECT
public:
    explicit FirstRunWizard(QWidget* parent = nullptr);
    
private slots:
    void saveConfig();
};

#endif