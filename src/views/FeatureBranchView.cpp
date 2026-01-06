#include "FeatureBranchView.h"
#include <QLabel>
#include <QVBoxLayout>

FeatureBranchView::FeatureBranchView(GitService*, GitLabApi*, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("🟢 开发分支活跃视图", this));
}