#include "jasonparser.h"
#include "downloader.h"

JasonParser::JasonParser(){

}

JasonParser::~JasonParser(){

}

void JasonParser::testEnvironment(){
    qDebug() << "substitutes:" << substitutes;
    qDebug() << "activeOptions:" << activeOptions;
    qDebug() << "procEnv:" << procEnv.toStringList();
    foreach(QString var,procEnv.toStringList()){
        if(var.contains("%"))
            qDebug() << "unresolved variable?" << var.split("=")[1];
        if(var.contains("//"))
            qDebug() << "unresolved variable?" << var.split("=")[1];
    }
    foreach(QString var,substitutes.keys()){
        if(substitutes.value(var).contains("%"))
            qDebug() << "unresolved variable?" << var << substitutes.value(var);
    }
}

void JasonParser::startParse(){
    updateProgressText(tr("Starting to parse JSON document"));
    QString startDocument,jasonPath;
    startDocument=startOpts.value("start-document");
    jasonPath=startOpts.value("kaidan-path");
    if(jsonParse(jsonOpenFile(startDocument))!=0){
//        updateProgressText(tr("Error occured"));
//        broadcastMessage(2,tr("Apples is stuck in a tree! We need to call the fire department!\n"));
        emit failedProcessing();
        return;
    }

    QEventLoop waitForEnd;
    connect(this,SIGNAL(finishedProcessing()),&waitForEnd,SLOT(quit()));
    waitForEnd.exec();
    return;
}

void JasonParser::setStartOpts(QString startDocument){
    if(!startDocument.isEmpty())
        startOpts.insert("start-document",startDocument);
    QFileInfo cw(startDocument);
    startOpts.insert("wd",cw.canonicalPath());
    return;
}

QJsonDocument JasonParser::jsonOpenFile(QString filename){
    QFile jDocFile;

    jDocFile.setFileName(filename);
    if (!jDocFile.exists()) {
        broadcastMessage(0,"ERROR: jDocFile::File not found\n");
        return QJsonDocument();
    }

    if (!jDocFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        broadcastMessage(2,"ERROR: jDocFile::Failed to open\n");
        updateProgressText(tr("Failed to open file"));
        return QJsonDocument();
    }
    QJsonParseError initError;
    QJsonDocument jDoc = QJsonDocument::fromJson(jDocFile.readAll(),&initError);
    if (initError.error != 0)
        broadcastMessage(2,"ERROR: jDoc: "+initError.errorString()+"\n");
    if (jDoc.isNull() || jDoc.isEmpty()) {
        broadcastMessage(2,"ERROR: jDoc::IsNull or IsEmpty\n");
        updateProgressText(tr("Failed to import file"));
        return QJsonDocument();
    }
    return jDoc;
}

int JasonParser::parseStage1(QJsonObject mainObject){
    /*
     * Get variables and environment variables
     *
     */
    updateProgressText(tr("Importing variables and options"));
    foreach(QString key, mainObject.keys()){
        QJsonValue instanceValue = mainObject.value(key);
        if(key=="variables"){
            QHash<QString,QVariant> variablesTable = jsonExamineArray(instanceValue.toArray());
            foreach(QString key,variablesTable.keys())
                foreach(QString kKey, variablesTable.value(key).toHash().keys())
                    if(kKey=="name")
                        variableHandle(variablesTable.value(key).toHash().value("name").toString(),variablesTable.value(key).toHash().value("value").toString());
        }
        if(key=="kaidan-opts")
            activeOptions.insert(key,instanceValue.toObject());
    }
    //Let's insert the system environment variables into the substitutes system here, before we start resolving any of them.
    procEnv.insert(QProcessEnvironment::systemEnvironment());
    if(activeOptions.value("kaidan-opts").isValid()){
        QJsonObject kaidanOpts = activeOptions.value("kaidan-opts").toJsonObject();
        foreach(QString key,kaidanOpts.keys()){
            if(key=="import-env-variables"){
                QProcessEnvironment variables = QProcessEnvironment::systemEnvironment();
                foreach(QString var, kaidanOpts.value(key).toString().split(",")){
                    variableHandle(var,variables.value(var));
                }
            }
            if(key=="icon-size")
                emit updateIconSize(kaidanOpts.value(key).toDouble());
            if(key=="window-title")
                emit updateProgressTitle(kaidanOpts.value(key).toString());
        }
    }
    if(!startOpts.value("wd").isEmpty())
        variableHandle("STARTDIR",startOpts.value("wd"));
    resolveVariables();
    foreach(QString key, mainObject.keys()){
        QJsonValue instanceValue = mainObject.value(key);
        if(key=="environment"){
            QHash<QString,QVariant> thisEnvironment = jsonExamineArray(instanceValue.toArray());
            foreach(QString thingy,thisEnvironment.keys())
                environmentActivate(thisEnvironment.value(thingy).toHash());
        }
    }


    updateProgressText(tr("Resolving variables"));
    resolveVariables();
    return 0;
}

int JasonParser::parseStage2(QJsonObject mainObject){
    updateProgressText(tr("Importing installation steps"));
    foreach(QString key,mainObject.keys()){
        QJsonValue instanceValue = mainObject.value(key);
        if(key=="steps"){
            if(runProcesses(instanceValue.toArray())!=0)
                return 1;
        }
    }
    return 0;
}

int JasonParser::jsonParse(QJsonDocument jDoc){
    QJsonObject mainTree = jDoc.object();
    if((mainTree.isEmpty())||(jDoc.isEmpty())){
        updateProgressText(tr("No objects found. Will not proceed."));
        return 1;
    }
    updateProgressText(tr("Gathering fundamental values"));
    if(parseStage1(mainTree)!=0){
        updateProgressText(tr("Failed to parse fundamental values. Will not proceed."));
        return 1;
    }

    updateProgressText(tr("Parsing runtime steps"));
    if(parseStage2(mainTree)!=0){
        return 1;
    }

    return 0;
}

QHash<QString,QVariant> JasonParser::jsonExamineArray(QJsonArray jArray){
    if (jArray.isEmpty())
        return QHash<QString,QVariant>();
    QHash<QString,QVariant> returnTable;
    //Iterate over the input array
    for(int i = 0;i<jArray.count();i++){
        QJsonValue instance = jArray.at(i);
        if (instance.isArray()){
            //Method used when an array is found
            returnTable.insert(instance.toString(),jsonExamineArray(instance.toArray()));
        } if (instance.isObject()) {
            //Method used when an object is found
            QHash<QString,QVariant> objectTable = jsonExamineObject(instance.toObject());
            int i2 = returnTable.count();
            returnTable.insert(QString::number(i2),objectTable);
        } else {
            // When all else fails, return the possible other type of value from the QJsonValue
            returnTable.insert(instance.toString(),jsonExamineValue(instance));
        }
    }
    return returnTable;
}


QVariant JasonParser::jsonExamineValue(QJsonValue jValue){
    if (jValue.isNull())
        return QVariant();
    QVariant returnValue;
    //Returns only what the QJsonValue contains, no branching off, parent will take care of the rest.
    if(jValue.isString()){
        returnValue = jValue.toString();
    }if(jValue.isDouble()){
        returnValue = jValue.toDouble();
    }if(jValue.isBool()){
        returnValue = jValue.toBool();
    }if(jValue.isObject()){
        returnValue = jValue.toObject();
    }
    return returnValue;
}


QHash<QString,QVariant> JasonParser::jsonExamineObject(QJsonObject jObject){
    if (jObject.isEmpty())
        return QHash<QString,QVariant>();
    QHash<QString,QVariant> returnTable; //Create a list for the returned objects
    //Objects may contain all different kinds of values, take care of this
    foreach(QString key, jObject.keys()){
        if(jObject.value(key).isBool())
            returnTable.insert(key,jObject.value(key).toBool());
        if(jObject.value(key).isString())
            returnTable.insert(key,jObject.value(key).toString());
        if(jObject.value(key).isDouble())
            returnTable.insert(key,jObject.value(key).toDouble());
        if(jObject.value(key).isArray())
            returnTable.insert(key,jObject.value(key).toArray());
        if(jObject.value(key).isObject())
            returnTable.insert(key,jObject.value(key).toObject());
    }

    return returnTable;
}


void JasonParser::setEnvVar(QString key, QString value) {
    //Insert the variable into the environment
    procEnv.insert(key,value);
    return;
}


void JasonParser::variableHandle(QString key, QString value){
    //Insert variable in form "NAME","VAR"
    substitutes.insert(key,value);
}


void JasonParser::resolveVariables(){
//    QStringList systemVariables;
    //System variables that are used in substitution for internal variables. Messy indeed, but it kind of works in a simple way.
//    foreach(QString opt, activeOptions.keys())
//        if(opt.startsWith("shell.properties"))
//            foreach(QString key, activeOptions.value(opt).toJsonObject().keys())
//                if(key=="import-env-variables")
//                    systemVariables=activeOptions.value(opt).toJsonObject().value(key).toString().split(",");
//    if(systemVariables.isEmpty())
//        broadcastMessage(1,tr("Note: No system variables were imported. This may be a bad sign.\n"));
//    foreach(QString variable, systemVariables){
//        QProcessEnvironment variableValue = QProcessEnvironment::systemEnvironment();
//        substitutes.insert(variable,variableValue.value(variable));
//    }


    int indicator = 0; //Indicator for whether the operation is done or not. May cause an infinite loop when a variable cannot be resolved.
    while(indicator!=1){
        foreach(QString key, substitutes.keys()){
            QString insert = substitutes.value(key);
            substitutes.remove(key);
            substitutes.insert(key,resolveVariable(insert));
        }
        int indicatorLocal = 0;
        foreach(QString key,substitutes.keys())
            if(!substitutes.value(key).contains("%")){
                indicatorLocal++;
            }else
                updateProgressText(tr("Variable %1 is being slightly problematic.").arg(key));
        if(indicatorLocal==substitutes.count())
            indicator = 1;
    }
    return;
}


QString JasonParser::resolveVariable(QString variable){
    //Takes the variable's contents as input and replaces %%-enclosed pieces of text with their variables. Will always return a variable, whether it has all variables resolved or not.
    if(variable.isEmpty())
        return QString();
    foreach(QString sub, substitutes.keys()){
        QString replace = sub;
        replace.prepend("%");replace.append("%");
        variable = variable.replace(replace,substitutes.value(sub));
    }
    return variable;
}

void JasonParser::environmentActivate(QHash<QString,QVariant> environmentHash){
    QString type;
    //Get type in order to determine how to treat this environment entry
    foreach(QString key, environmentHash.keys()){
        if(key=="type")
            type = environmentHash.value(key).toString();
    }
    //Treat the different types according to their parameters and realize their properties
    if(type=="variable"){
        foreach(QString key,environmentHash.keys()){
            if(key=="name")
                if(environmentHash.keys().contains("value"))
                    setEnvVar(environmentHash.value(key).toString(),resolveVariable(environmentHash.value("value").toString()));
        }
    }else{
        broadcastMessage(1,tr("unsupported environment type").arg(type));
        return;
    }
    return;
}


int JasonParser::evaluateKaidanStep(QJsonObject kaidanStep){
    if(kaidanStep.value("type").isUndefined())
        return 1; //No type? No go.
    QString type = kaidanStep.value("type").toString();
    if(type=="install-payload"){
        if(kaidanStep.value("source").isUndefined())
            return 1;
        if(kaidanStep.value("target").isUndefined())
            return 1;
    }
    if(type=="download-file"){
        if(kaidanStep.value("source").isUndefined())
            return 1;
        if(kaidanStep.value("target").isUndefined())
            return 1;
    }
    if(type=="execute-commandline")
        if(kaidanStep.value("commandline").isUndefined())
            return 1;
    if(type=="execute-file")
        if(kaidanStep.value("target").isUndefined())
            return 1;
    if(type=="user-message")
        if(kaidanStep.value("message").isUndefined())
            return 1;
    return 0;
}


int JasonParser::executeKaidanStep(QJsonObject kaidanStep){
    if(!kaidanStep.value("desktop.title").isUndefined())
        updateProgressText(kaidanStep.value("desktop.title").toString());
    if(!kaidanStep.value("desktop.icon").isUndefined())
        updateProgressIcon(resolveVariable(kaidanStep.value("desktop.icon").toString()));
    QString type = kaidanStep.value("type").toString();
    if(type=="download-file"){
        QString targetFile,sourceUrl;
        targetFile = resolveVariable(kaidanStep.value("target").toString());
        sourceUrl = kaidanStep.value("source").toString();
        QFile downloadChecker(targetFile);
        if(downloadChecker.exists()){
            updateProgressText(tr("File exists. Will assume it is a preloaded copy."));
            return 0;
        }
        Downloader dlInstance; //We need to pass signals through from here to the GUI, with some modifications to what is presented.
        QThread workerThread;
        dlInstance.moveToThread(&workerThread);
        QEventLoop downloadWaiter;
        emit changeSProgressBarVisibility(true);
        connect(&dlInstance,SIGNAL(finishedDownload()),&workerThread,SLOT(quit()));
        connect(&dlInstance,SIGNAL(failedDownload()),&workerThread,SLOT(quit()));
        connect(&dlInstance,SIGNAL(finishedDownload()),&downloadWaiter,SLOT(quit()));
        connect(&dlInstance,SIGNAL(failedDownload()),&downloadWaiter,SLOT(quit()));
        connect(&dlInstance,SIGNAL(reportDownloadProgress(int)),this,SLOT(forwardSecondaryProgress(int)));
        connect(&workerThread,SIGNAL(started()),&dlInstance,SLOT(startDownloads()));
        dlInstance.downloadUrl(sourceUrl,targetFile);
        workerThread.start();
        downloadWaiter.exec();
        if(!downloadChecker.exists())
            return 1;
        emit changeSProgressBarVisibility(false);
    }
    if(type=="execute-commandline"){
        QJsonObject shellOptions = activeOptions.value("kaidan-opts").toJsonObject();
        QString shell = "sh";
        QString shellArg = "-c";
        QStringList arguments;
        QString workdir;
        bool validate = false;
        bool lazyExitStatus = false;
        foreach(QString key,shellOptions.keys()){
            if(key=="shell")
                shell=shellOptions.value(key).toString();
            if(key=="command.argument")
                shellArg=shellOptions.value(key).toString();
        }
        arguments.append(shellArg);
        arguments.append("--");
        arguments.append(resolveVariable(kaidanStep.value("commandline").toString()));
        workdir = resolveVariable(kaidanStep.value("workdir").toString(""));
        if(!kaidanStep.value("validate").isUndefined())
            validate = kaidanStep.value("validate").toBool();
        if(!kaidanStep.value("lazy-exit-status").isUndefined())
            lazyExitStatus = kaidanStep.value("lazy-exit-status").toBool();
        int execResult = executeProcess(shell,arguments,createProcEnv(kaidanStep.value("environment").toArray()),workdir,lazyExitStatus);
        if(validate)
            if(execResult!=0){
                updateProgressText(tr("Executing commandline exited with invalid exit status."));
                return 1;
            }
    }
    if(type=="execute-file"){
        QString program;
        QStringList arguments;
        QString workdir;
        bool validate = false;
        bool lazyExitStatus = false;
        program = resolveVariable(kaidanStep.value("target").toString());
        if(!kaidanStep.value("arguments").isUndefined())
            arguments.append(kaidanStep.value("arguments").toString());
        arguments.append(kaidanStep.value("commandline").toString());
        workdir = resolveVariable(kaidanStep.value("workdir").toString(""));
        if(!kaidanStep.value("validate").isUndefined())
            validate = kaidanStep.value("validate").toBool();
        if(!kaidanStep.value("lazy-exit-status").isUndefined())
            lazyExitStatus = kaidanStep.value("lazy-exit-status").toBool();
        int execResult = executeProcess(program,arguments,createProcEnv(kaidanStep.value("environment").toArray()),workdir,lazyExitStatus);
        if(validate)
            if(execResult!=0){
                updateProgressText(tr("Executing file exited with invalid exit status."));
                return 1;
            }
    }
    if(type=="install-payload"){
        QString sourceFilename,targetFilename;
        sourceFilename = resolveVariable(kaidanStep.value("source").toString());
        targetFilename = resolveVariable(kaidanStep.value("target").toString());
        bool caseSense = kaidanStep.value("case-sensitivity").toBool(true);
        bool overwriteFile = kaidanStep.value("overwrite-files").toBool(false);
        bool appendFile = kaidanStep.value("append-file").toBool(false);
        if((sourceFilename.isEmpty())||(targetFilename.isEmpty())){
            updateProgressText(tr("Failed to install payload; no filenames provided."));
            return 1;
        }
        QFileInfo sourceFileInfo(sourceFilename);
        if(sourceFileInfo.isFile()){
            QFileInfo *entryInfo = new QFileInfo(sourceFilename);
            applyPayloadFile(sourceFilename,targetFilename,entryInfo,overwriteFile,appendFile);
        }else if(sourceFileInfo.isDir()){
            QDir sourceDir(sourceFilename);
            QDir targetDir(targetFilename);
            if(!sourceDir.exists()){
                updateProgressText(tr("Payload referenced in JSON does not exist."));
                return 1;
            }
            if(!targetDir.exists())
                targetDir.mkpath(targetFilename);
            foreach(QString entry,sourceDir.entryList(QDir::Files|QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot)){
                QString entryFN = sourceFilename+QString("/")+entry;
                QString entryDFN = targetFilename+QString("/")+entry;
                QFileInfo *entryInfo = new QFileInfo(entryFN);
                if(entryInfo->isDir()){
                    if(copyDirectory(entryFN,entryDFN,caseSense,overwriteFile,appendFile)!=0){
                        return 1;
                    }
                }
                if(entryInfo->isFile()){
                    applyPayloadFile(entryFN,entryDFN,entryInfo,overwriteFile,appendFile);
                }
            }
        }
    }
    if(type=="user-message"){
        if(kaidanStep.value("message").toString().isEmpty())
            return 0;
        QString message = kaidanStep.value("message").toString();
        broadcastMessage(0,message);
    }
    return 0;
}

int JasonParser::applyPayloadFile(QString sourceFilename,QString targetFilename,QFileInfo *entryInfo, bool overwriteFile,bool appendFile){
    QFile *fileOperator = new QFile(sourceFilename);
    QFile *destFileOperator = new QFile(targetFilename);
    QFileInfo *destEntryInfo = new QFileInfo(targetFilename);
    QDir targetDir(destEntryInfo->path());
    if(!targetDir.exists())
        targetDir.mkpath(destEntryInfo->path());
    if(appendFile&&destFileOperator->exists()){
        if(destFileOperator->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)&&fileOperator->open(QIODevice::ReadOnly)){
            if(destFileOperator->write(fileOperator->readAll())==-1){
                updateProgressText(tr("Failure upon installing payload: Could not append to file"));
                return 1;
            }else
                destFileOperator->close();
        }else{
            updateProgressText(tr("Failure upon installing payload: Could not open file to append"));
            return 1;
        }
        return 0;
    }
    if(overwriteFile&&destFileOperator->exists()){
        if(!destFileOperator->remove()){
            updateProgressText(tr("Failure upon installing payload: Could not remove existing file"));
            return 1;
        }
    }
    if(!fileOperator->copy(targetFilename)){
        updateProgressText(tr("Failure upon installing payload: Will not overwrite existing file."));
        return 1;
    }
    return 0;
}

int JasonParser::copyDirectory(QString oldName, QString newName, bool caseSense, bool overwriteFile, bool appendFile){
    QDir sourceDir(oldName);
    QDir targetDir(newName);
    if(!targetDir.exists())
        targetDir.mkdir(newName);
    foreach(QString entry,sourceDir.entryList(QDir::Files|QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot)){
        QString entryFN = oldName+QString("/")+entry;
        QString entryDFN = newName+QString("/")+entry;
        QFileInfo *entryInfo = new QFileInfo(entryFN);
        if(entryInfo->isDir()){
            if(copyDirectory(entryFN,entryDFN,caseSense,overwriteFile,appendFile)!=0){
                updateProgressText(tr("Failure upon installing payload: Failed to copy directory."));
                return 1;
            }
        }
        if(entryInfo->isFile()){
            applyPayloadFile(entryFN,entryDFN,entryInfo,overwriteFile,appendFile);
        }
    }
    return 0;
}


void JasonParser::forwardSecondaryProgress(int value){
    emit changeSProgressBarValue(value);
}


QProcessEnvironment JasonParser::createProcEnv(QJsonArray environment){
    QProcessEnvironment returnEnv;
    for(int i=0;i<environment.count();i++){
        QJsonObject currentEnv = environment.at(i).toObject();
        returnEnv.insert(currentEnv.value("name").toString(),currentEnv.value("value").toString());
    }
    return returnEnv;
}


int JasonParser::runProcesses(QJsonArray stepsArray){
    //First, we want to parse and put all the steps into memory. Once the Kaidan train starts, there's no stopping it. Also, error-checking.
    QHash<QString,QJsonObject> stepsHash;
    for(int i=0;i<stepsArray.count();i++){
        QJsonObject currentStep = stepsArray.at(i).toObject();
        foreach(QString key, currentStep.keys()){
            if(key=="name"){
                QString stepName = currentStep.value(key).toString();
                currentStep.remove(key);
                stepsHash.insert(stepName,currentStep);
                break; //We don't want to waste time
            }
            if((key=="init-step")&&(currentStep.value(key).toBool(false))){
                currentStep.remove(key);
                stepsHash.insert("init-step",currentStep); //A quick and easy way of identifying the first step.
                break;
            }
        }
    }
    //After inserting the objects, we do a testrun to see that they are all okay and functional.
    if(!stepsHash.value("init-step").isEmpty()){ //We need this to begin, otherwise, puke.
        foreach(QJsonObject step,stepsHash.values())
            if(evaluateKaidanStep(step)!=0)
                return 1;
    }else
        return 1;
    QString currentStepKey = "init-step";
    QJsonObject currentStep = stepsHash.value(currentStepKey);
    stepsHash.remove(currentStepKey);
    executeKaidanStep(currentStep);
    while(!stepsHash.isEmpty()){
        currentStepKey = currentStep.value("proceed-to").toString();
        if(currentStepKey.isEmpty())
            break;
        currentStep = stepsHash.value(currentStepKey);
        stepsHash.remove(currentStepKey);
        if(executeKaidanStep(currentStep)!=0){
            emit failedProcessing();
            return 1;
        }
    }
    finishedProcessing();
    return 0;
}

int JasonParser::executeProcess(QString program,QStringList arguments,QProcessEnvironment localProcEnv,QString workDir,bool lazyExitStatus){
    /*
     * program - Prefixed to argument, specifically it could be 'wine' or another frontend program such as
     * 'mupen64plus'. It is not supposed to run shells.
     * argument - The command line, only in this context it is indeed an argument.
     * workDir - The wished working directory for the operation.
    */

    if(program.isEmpty())
        return 1;

    QProcess *executer = new QProcess(this);
    QProcessEnvironment localEnv = procEnv;
    localEnv.insert(localProcEnv);
    executer->setProcessEnvironment(localEnv);
    executer->setProcessChannelMode(QProcess::SeparateChannels);
    if(!workDir.isEmpty())
        executer->setWorkingDirectory(workDir);
    executer->setProgram(program);
    executer->setArguments(arguments);

    //Connect signals and slots as well as update the title.
    if(!lazyExitStatus)
        connect(executer, SIGNAL(finished(int,QProcess::ExitStatus)),SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(executer, SIGNAL(error(QProcess::ProcessError)),SLOT(processOutputError(QProcess::ProcessError)));
    connect(executer,SIGNAL(started()),SLOT(processStarted()));

    executer->start();
    executer->waitForFinished(-1);
    if(!lazyExitStatus)
        if((executer->exitCode()!=0)||(executer->exitStatus()!=0)){
            QString stdOut,stdErr,argumentString;
            stdOut = executer->readAllStandardOutput();
            stdErr = executer->readAllStandardError();
            foreach(QString arg,executer->arguments())
                argumentString.append(arg+" ");
            stdOut.prepend(tr("Executed: %1 %2\n").arg(executer->program()).arg(argumentString));
            stdOut.prepend(tr("Process returned with status: %1.\n").arg(QString::number(executer->exitCode())));
            stdOut.prepend(tr("QProcess returned with status: %1.\n").arg(QString::number(executer->exitStatus())));
            emit emitOutput(stdOut,stdErr);
        }
    return executer->exitCode();
}

void JasonParser::processFinished(int exitCode, QProcess::ExitStatus exitStatus){
    /*
     * We won't care about exitStatus for now.
     * broadcastMessage() function shows a QMessageBox, its prototype is:
     * int,QString
     * where int is either 0, success, 1, warning, 2, error, or 3, disabled. (The last one exists because I am lazy.)
     * The QString is just the message it shows.
    */
    exitResult+=exitCode;
    exitResult+=exitStatus;
//    if(exitCode!=0)
//        broadcastMessage(1,tr("Process exited with status: %1.\n").arg(QString::number(exitCode)));
//    if(exitStatus!=0)
//        broadcastMessage(1,tr("QProcess exited with status: %1.\n").arg(QString::number(exitStatus)));
}

void JasonParser::processOutputError(QProcess::ProcessError processError){
    broadcastMessage(2,tr("QProcess exited with status: %1.\n").arg(QString::number(processError)));
    emit processFailed(processError);
}

void JasonParser::processStarted(){

}
