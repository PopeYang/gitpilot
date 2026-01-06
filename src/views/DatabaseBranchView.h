#ifndef DATABASEBRANCHVIEW_H
#define DATABASEBRANCHVIEW_H

#include <QWidget>

class GitService;
class GitLabApi;

class DatabaseBranchView : public QWidget {
    Q_OBJECT
public:
    explicit DatabaseBranchView(GitService* gitService, GitLabApi* gitLabApi, QWidget* parent = nullptr);
};

#endif