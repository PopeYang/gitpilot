#ifndef PROTECTEDBRANCHVIEW_H
#define PROTECTEDBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;

class ProtectedBranchView : public QWidget {
    Q_OBJECT
public:
    explicit ProtectedBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
};

#endif