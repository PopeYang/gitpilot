#include "ProtectedBranchView.h"
#include <QLabel>
#include <QVBoxLayout>

ProtectedBranchView::ProtectedBranchView(GitService*, GitLabApi*, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("🔒 保护分支同步视图", this));
}