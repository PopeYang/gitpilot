#include "SettingsDialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("设置");
    resize(500, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("设置对话框开发中...", this));
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}