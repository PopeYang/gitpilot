#ifndef MAINBRANCHVIEW_H
#define MAINBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;

class MainBranchView : public QWidget {
    Q_OBJECT
public:
    explicit MainBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
};

#endif