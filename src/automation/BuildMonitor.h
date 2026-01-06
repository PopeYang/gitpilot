#ifndef BUILDMONITOR_H
#define BUILDMONITOR_H

#include <QObject>

class BuildMonitor : public QObject {
    Q_OBJECT
public:
    explicit BuildMonitor(QObject* parent = nullptr);
};

#endif