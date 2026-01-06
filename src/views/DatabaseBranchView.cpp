#include "DatabaseBranchView.h"
#include <QLabel>
#include <QVBoxLayout>

DatabaseBranchView::DatabaseBranchView(GitService*, GitLabApi*, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("🟣 数据库分支受限视图", this));
}