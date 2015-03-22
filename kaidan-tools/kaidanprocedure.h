#ifndef KAIDANPROCEDURE_H
#define KAIDANPROCEDURE_H

#include <QObject>
#include <QString>
#include "modules/variablehandler.h"
#include "modules/environmentcontainer.h"
#include "jsonstaticfuncs.h"
#include "kaidan-tools/kaidanstep.h"

class KaidanProcedure : public QObject
{
    Q_OBJECT
public:
    explicit KaidanProcedure(QObject *parent = 0,VariableHandler* varHandler = 0,EnvironmentContainer* envContainer = 0);
    ~KaidanProcedure();

    QHash<QString, KaidanStep *> resolveDependencies(QVariantMap *totalMap);
    void setShellOpts(QString shell,QString shellArg){
        this->shell = shell;
        this->shellArg = shellArg;
    }

signals:
    void updateProgressText(QString);
    void updateProgressIcon(QString);
    void failedProcessing();
    void reportError(int,QString);
public slots:

private:
    QString shell;
    QString shellArg;
    VariableHandler* varHandler;
    EnvironmentContainer* envContainer;
    QStringList filesystemLog;
};

#endif // KAIDANPROCEDURE_H
