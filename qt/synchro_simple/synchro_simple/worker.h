#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

#include "dialog.h"
#include "../../../synchro_test/synchro.h"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(Dialog* w);
    void printMsg(const char* buf);
    void sendValue(const int64 val);

signals:
    void finished();
    void error(QString err);
    void print(const char* buf);
    void progressBar(int64 total, int64 val);

public slots:
    void process();

private:
    Dialog* dialog;
    QSettings settings;
    int64 progressTotal;

    filter_t* filters;
    int filters_length;

    void setFilters();
    void freeFilters();
};

#endif // WORKER_H
