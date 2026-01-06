#ifndef WORKFLOWENGINE_H
#define WORKFLOWENGINE_H

#include <QObject>

class WorkflowEngine : public QObject {
    Q_OBJECT
public:
    explicit WorkflowEngine(QObject* parent = nullptr);
};

#endif