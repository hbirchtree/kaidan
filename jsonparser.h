#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QProcessEnvironment>
#include "jsonstaticfuncs.h"
#include "kaidan-tools/kaidanprocedure.h"

class jsonparser : public QObject
{
    Q_OBJECT
public:
    explicit jsonparser(QObject *parent = 0);
    QJsonDocument jsonOpenFile(QString filename);
    int jsonParse(QJsonDocument jDoc);
    int jasonActivateSystems(QHash<QString,QVariant> const &jsonData, QHash<QString, QVariant> *runtimeValues);

    bool hasCompleted(){
        return b_hasCompleted;
    }

    QString getShell(){
        return shellString;
    }
    QString getShellArg(){
        return shellArgString;
    }
    QHash<QString,QVariant> getWindowOpts(){
        return windowOpts;
    }
    QHash<QString,KaidanStep*> getRunQueue(){
        return runQueue;
    }

private:
    bool b_hasCompleted = false;
    //Constants
    QString startDir;

    void getMap(QVariantMap *depmap, QVariantMap *totalMap, KaidanProcedure *jCore);

    QString shellString;
    QString shellArgString;

    QHash<QString,KaidanStep*> runQueue;

    QHash<QString,QVariant> windowOpts;

signals:
    void reportError(int errorLevel,QString message);
    void sendMessage(QString message);
    void sendProgressTextUpdate(QString message);
    void sendProgressBarUpdate(int value);
    void failedProcessing();

public slots:

};

#endif // JSONPARSER_H
