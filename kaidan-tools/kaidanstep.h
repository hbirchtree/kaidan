#ifndef KAIDANSTEP
#define KAIDANSTEP

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QEventLoop>
#include "modules/variablehandler.h"
#include "modules/environmentcontainer.h"
#include "kaidan-tools/filesystemactions.h"
#include "kaidan-tools/downloader.h"
#include "modules/executionunit.h"
#include "executer.h"

class KaidanStep : public QObject {
    Q_OBJECT
public:
    KaidanStep(QMap<QString,QVariant> source,VariableHandler* varHandler, EnvironmentContainer* envContainer, QString shell, QString shellArg){
        this->varHandler = varHandler;
        this->envContainer = envContainer;
        for(QString key : source.keys()){
            if(key=="proceed-to")
                NEXTStep = source.value(key).toString();
            if(key=="init-step")
                b_firstStep = source.value(key).toBool();
            if(key=="desktop.title")
                d_title = varHandler->resolveVariable(source.value(key).toString());
            if(key=="desktop.icon")
                d_icon = varHandler->resolveVariable(source.value(key).toString());;
            if(key=="name")
                s_stepName = source.value(key).toString();
            if(key=="type"){
                QString typeString = source.value(key).toString();
                s_stepType = typeString;
                if(typeString=="install-payload"){
                    s_fileSource = varHandler->resolveVariable(source.value("source").toString());
                    s_fileTarget = varHandler->resolveVariable(source.value("target").toString());
                    appendFile = source.value("append-file").toBool();
                    overwriteFile = source.value("overwrite-file").toBool();
                    fsActor = new FilesystemActions(s_fileSource,s_fileTarget,overwriteFile,appendFile);
                    connect(fsActor,&FilesystemActions::updateProgressText,[=](QString text){
                        updateProgressText(text);
                    });
                }
                if(typeString=="download-file"){
                    dlActor = new Downloader();
                    s_fileSource = varHandler->resolveVariable(source.value("source").toString());
                    s_fileTarget = varHandler->resolveVariable(source.value("target").toString());
                    appendFile = source.value("append-file").toBool();
                    overwriteFile = source.value("overwrite-file").toBool();
                }
                if(typeString=="execute-commandline"){
                    execUnit = new ExecutionUnit(this,varHandler,source);
                    if(!execUnit->isValid()){
                        delete execUnit;
                        execUnit = 0;
                    }else{
                        execUnit->getEnvironment()->merge(envContainer);
                        execUnit->resolveVariables(varHandler);
                    }
                    this->shell = shell;
                    this->shellArg = shellArg;
                }
                if(typeString=="user-message")
                    s_userMsg = source.value("message").toString();
            }
        }
    }

    QString getTitle(){
        return d_title;
    }
    QString getIcon(){
        return d_icon;
    }

    bool isFirstStep(){
        return b_firstStep;
    }
    QString getNextStep(){
        return NEXTStep;
    }
    QString getStepName(){
        return s_stepName;
    }
    QString getStepType(){
        return s_stepType;
    }
    int performStep(){
        updateProgressText(d_title);
        updateProgressIcon(d_icon);
        if(s_stepType=="install-payload")
            return installPayload();
        if(s_stepType=="user-message")
            broadcastMessage(0,s_userMsg);
        if(s_stepType=="execute-commandline")
            return executeCommandline();
        if(s_stepType=="download-file")
            return downloadFile();
    }
    bool isReady(){
        return b_validState;
    }

signals:
    void updateProgressText(QString);
    void updateProgressIcon(QString);
    void updateSecondaryProgress(int);
    void broadcastMessage(int,QString);
    void emitOutput(QString,QString);
    void changeSecondaryProgressBarVisibility(bool);
    void displayDetachedMessage(QString);

private:
    int downloadFile(){
        QFile downloadChecker(s_fileTarget);
        if(downloadChecker.exists()&&!overwriteFile){
            updateProgressText(tr("File exists. Will assume it is a preloaded copy."));
            return 0;
        }
        QThread workerThread;
        dlActor->moveToThread(&workerThread);
        QEventLoop downloadWaiter;
        emit changeSecondaryProgressBarVisibility(true);
        connect(dlActor,SIGNAL(finishedDownload()),&workerThread,SLOT(quit()));
        connect(dlActor,SIGNAL(failedDownload()),&workerThread,SLOT(quit()));
        connect(dlActor,SIGNAL(finishedDownload()),&downloadWaiter,SLOT(quit()));
        connect(dlActor,SIGNAL(failedDownload()),&downloadWaiter,SLOT(quit()));
        connect(dlActor,&Downloader::reportDownloadProgress,[=](int progress){
            emit updateSecondaryProgress(progress);
        });
        connect(&workerThread,SIGNAL(started()),dlActor,SLOT(startDownloads()));
        dlActor->downloadUrl(s_fileSource,s_fileTarget);
        workerThread.start();
        downloadWaiter.exec();
        emit changeSecondaryProgressBarVisibility(false);
        if(!downloadChecker.exists())
            return 1;
        return 0;
    }

    int executeCommandline(){
        if(execUnit==0)
            return 1;
        execUnit->resolveVariables(varHandler);
        QEventLoop waiter;
        Executer e(this,shell,shellArg);
        QMetaObject::Connection logger = connect(&e,&Executer::emitOutput,[=](QString out,QString err){this->emitOutput(out,err);qDebug() << out << err;});
        if(!execUnit->getTitle().isEmpty())
            updateProgressText(execUnit->getTitle());
        if(execUnit->isDetachable()){
            connect(&e,SIGNAL(finished()),&waiter,SLOT(quit()));
        } else
            connect(this,SIGNAL(detachedRunEnd()),&waiter,SLOT(quit()));
        int returnValue = e.exec(execUnit);
        if(execUnit->isDetachable()){
            emit displayDetachedMessage(execUnit->getTitle());
            waiter.exec();
        }
        disconnect(logger);
        if(returnValue!=0&&!execUnit->isLazyExit())
            return 1;
        return 0;
    }

    int installPayload(){
        return fsActor->installPayload();
    }

    bool b_firstStep = false;
    QString NEXTStep;

    VariableHandler* varHandler;

    FilesystemActions* fsActor = 0;
    Downloader* dlActor = 0;
    bool appendFile = false;
    bool overwriteFile = false;
    QString s_fileSource;
    QString s_fileTarget;

    ExecutionUnit* execUnit;
    QString shell;
    QString shellArg;
    EnvironmentContainer* envContainer;

    bool b_validState = false;
    QString s_stepType;
    QString s_stepName;

    QString s_userMsg;

    QString d_title;
    QString d_icon;
};

#endif // KAIDANSTEP

