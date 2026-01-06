#ifndef FEATUREBRANCHVIEW_H
#define FEATUREBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;

class FeatureBranchView : public QWidget {
    Q_OBJECT
public:
    explicit FeatureBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
};

#endif