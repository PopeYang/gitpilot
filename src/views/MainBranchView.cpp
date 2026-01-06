#include "MainBranchView.h"
#include <QLabel>
#include <QVBoxLayout>

MainBranchView::MainBranchView(GitService*, GitLabApi*, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("🔴 主分支只读视图", this));
}