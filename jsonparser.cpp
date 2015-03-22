#include "jsonparser.h"

#include <QDebug>

jsonparser::jsonparser(QObject *parent) :
    QObject(parent)
{
}

QJsonDocument jsonparser::jsonOpenFile(QString filename){
    QFile jDocFile;
    QFileInfo cw(filename);
    if(startDir.isEmpty()){
        startDir = cw.absolutePath();
        jDocFile.setFileName(startDir+"/"+cw.fileName());
    }else
        jDocFile.setFileName(startDir+"/"+cw.fileName());
    if (!jDocFile.exists()) {
        sendProgressTextUpdate(tr("Failed due to the file %1 not existing").arg(jDocFile.fileName()));
        return QJsonDocument();
    }
    if (!jDocFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        sendProgressTextUpdate(tr("Failed to open the requested file"));
        return QJsonDocument();
    }
    QJsonParseError initError;
    QJsonDocument jDoc = QJsonDocument::fromJson(jDocFile.readAll(),&initError);
    if (initError.error != 0){
        reportError(2,tr("ERROR: jDoc: %s\n").arg(initError.errorString()));
        sendProgressTextUpdate(tr("An error occured. Please take a look at the error message to see what went wrong."));
        return QJsonDocument();
    }
    if (jDoc.isNull() || jDoc.isEmpty()) {
        sendProgressTextUpdate(tr("Failed to import file"));
        return QJsonDocument();
    }
    return jDoc;
}

void jsonparser::getMap(QVariantMap* depmap, QVariantMap* totalMap, KaidanProcedure *jCore){
    QStringList excludes;
    for(QVariantMap::const_iterator it = depmap->begin(); it!=depmap->end(); it++){
        if(it.key()=="imports")
            for(QVariant el : it.value().toList()){
                getMap(new QVariantMap(jsonOpenFile(el.toMap().value("file").toString()).object().toVariantMap()),totalMap,jCore);
                continue;
            }
        totalMap->insertMulti(it.key(),it.value()); //We want to generate a complete map of all values from all files. We'll deal with conflicts later.
    }
}

int jsonparser::jsonParse(QJsonDocument jDoc){
    VariableHandler* varHandler = new VariableHandler();
    EnvironmentContainer* envContainer = new EnvironmentContainer(this,varHandler);
    KaidanProcedure* jCore = new KaidanProcedure(this,varHandler,envContainer);

    QVariantMap* totalMap = new QVariantMap();

    connect(varHandler,&VariableHandler::sendProgressTextUpdate,[=](QString message){this->sendProgressTextUpdate(message);});
    connect(jCore,&KaidanProcedure::reportError,[=](int severity,QString message){this->reportError(severity,message);});
    connect(jCore,&KaidanProcedure::failedProcessing,[=](){
        emit failedProcessing();
    });

    QJsonObject mainTree = jDoc.object();
    if((mainTree.isEmpty())||(jDoc.isEmpty())){
//        sendProgressTextUpdate(tr("No objects found. Will not proceed."));
        return 1;
    }

    QVariantMap *mainMap = new QVariantMap(mainTree.toVariantMap());

    getMap(mainMap,totalMap,jCore);

    QList<QVariant> shellOpts;
    for(auto it=totalMap->begin();it!=totalMap->end();it++)
        if(it.key()=="kaidan-opts")
            shellOpts.append(it.value());
    QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    for(QVariant opt : shellOpts){
        QMap<QString,QVariant> optMap = opt.toMap();
        for(QString var : optMap.value("import-env-variables").toString().split(",")){
            varHandler->variableHandle(var,sysEnv.value(var));
        }
        for(QString key : optMap.keys()){
            if(key=="shell")
                shellString = optMap.value(key).toString();
            if(key=="command.argument")
                shellArgString = optMap.value(key).toString();
        }
    }
    jCore->setShellOpts(shellString,shellArgString);
    runQueue = jCore->resolveDependencies(totalMap);
    return 0;
}
