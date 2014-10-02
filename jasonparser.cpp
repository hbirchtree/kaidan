#include "jasonparser.h"

JasonParser::JasonParser(){

}

JasonParser::~JasonParser(){

}

void JasonParser::testEnvironment(){
    qDebug() << "substitutes:" << substitutes;
//    qDebug() << subsystems;
//    qDebug() << systemTable;
    qDebug() << "activeOptions:" << activeOptions;
    qDebug() << "procEnv:" << procEnv.toStringList();
    qDebug() << "launchables:" << runtimeValues.value("launchables");
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
    QString startDocument,actionId,desktopFile,jasonPath;
    startDocument=startOpts.value("start-document");
    jasonPath=startOpts.value("kaidan-path");
    if(jsonParse(jsonOpenFile(startDocument))!=0){
        updateProgressText(tr("Error occured"));
        broadcastMessage(2,tr("Apples is stuck in a tree! We need to call the fire department!\n"));
        emit toggleCloseButton(true);
        emit failedProcessing();
        return;
    }

    if(desktopFile.isEmpty()){
        if(runProcesses(actionId)!=0){
            updateProgressText(tr("Error occured while trying to launch"));
            broadcastMessage(2,tr("Shit.\n"));
            emit toggleCloseButton(true);
            emit failedProcessing();
            return;
        }
    }else{
        updateProgressText(tr("We are generating a .desktop file now. Please wait for possible on-screen prompts."));
        generateDesktopFile(desktopFile,jasonPath,startDocument);
    }
    QEventLoop waitForEnd;
    connect(this,SIGNAL(finishedProcessing()),&waitForEnd,SLOT(quit()));
    waitForEnd.exec();
    return;
}

void JasonParser::setStartOpts(QString startDocument, QString jasonPath){
    if(!startDocument.isEmpty())
        startOpts.insert("start-document",startDocument);
    if(!jasonPath.isEmpty())
        startOpts.insert("jason-path",jasonPath);
    QFileInfo cw(startDocument);
    startOpts.insert("working-directory",cw.canonicalPath());
    return;
}

QJsonDocument JasonParser::jsonOpenFile(QString filename){
    QFile jDocFile;
    QFileInfo cw(filename);

    jDocFile.setFileName(startOpts.value("working-directory")+"/"+cw.fileName());
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
    QHash<QString,QHash<QString,QVariant> > underlyingObjects;
    foreach(QString key, mainObject.keys()){
        QJsonValue instanceValue = mainObject.value(key);
        if(key=="variables")
            underlyingObjects.insert("variables",jsonExamineArray(instanceValue.toArray()));
    }
    if(parseUnderlyingObjects(underlyingObjects)!=0)
        return 1;
    procEnv.insert(QProcessEnvironment::systemEnvironment());
    return 0;
}

void JasonParser::stage2ActiveOptionAdd(QJsonValue instance,QString key){
    if(!instance.isNull()){
        QString insertKey = key;
        int insertInt = 0;
        while(activeOptions.contains(insertKey)){
            insertInt++;
            insertKey = key+"."+QString::number(insertInt);
        }
        if(instance.isString())
            activeOptions.insert(insertKey,instance.toString());
        if(instance.isObject())
            activeOptions.insert(insertKey,instance.toObject());
        if(instance.isDouble())
            activeOptions.insert(insertKey,instance.toDouble());
        if(instance.isArray())
            activeOptions.insert(insertKey,instance.toArray());
        if(instance.isBool())
            activeOptions.insert(insertKey,instance.toBool());
    }
}

int JasonParser::parseStage2(QJsonObject mainObject){
    /*
     * Stage 2:
     * Where options specified by the user or distributor are applied.
     * Parses through the options of the initial file only, will see if it is a good idea to parse
     * through the imported files as well.
    */
    foreach(QString key,mainObject.keys()){
        QJsonValue instanceValue = mainObject.value(key);

    }
    return 0;
}

int JasonParser::jsonParse(QJsonDocument jDoc){ //level is used to identify the parent instance of jsonOpenFile
/*
     *  - The JSON file is parsed in two stages; the JSON file is parsed randomly and as such
     * you cannot expect data to appear at the right time. For this, parsing in two stages
     * allows substituted values, systems, subsystems and etc. to listed in the first run
     * and applied in the second run. The int level is used for recursive parsing where
     * you do not want the process to proceed before everything is done.
     *  - The execution and/or creation of a desktop file is done post-parsing when all variables
     * are known and resolved.
     *
*/
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

    updateProgressText(tr("Gathering active options"));
    parseStage2(mainTree);
    foreach(QString import,importedFiles){
        parseStage2(jsonOpenFile(import).object());
    }

/*
 * Stage 3:
 * The active options have been listed and need to be parsed. With this it needs to look for
 * active subsystems and apply their options, prepare a list of programs to execute with QProcess
 * and prepare the system for launch.
 * Systems have default switches such as .exec and .workdir, but can also define variables according to a
 * switch, ex. wine.version referring to the internal variable %WINEVERSION%.
 * Subsystems will be triggered according to their type and what option is entered.
 *
*/
    updateProgressText(tr("Activating main system"));
    if(runtimeValues.isEmpty()){ //If the table is empty, initialize it with empty objects
        runtimeValues.insert("run-prefix",QList<QVariant>());
        runtimeValues.insert("run-suffix",QList<QVariant>());
        runtimeValues.insert("sys-prerun",QList<QVariant>());
        runtimeValues.insert("sys-postrun",QList<QVariant>());
        runtimeValues.insert("launchables",QHash<QString,QVariant>());
    }
    QStringList activeSystems;
    foreach(QString option,activeOptions.keys()){
        if(option=="launchtype"){
            if(!systemTable.value(activeOptions.value("launchtype").toString()).isValid())
                return 1;
            QHash<QString,QVariant> currentSystem = systemTable.value(activeOptions.value("launchtype").toString()).toHash();
            foreach(QString key,currentSystem.keys()){
                if(key=="identifier")
                    activeSystems.append(currentSystem.value(key).toString());
                if(key=="inherit")
                    activeSystems.append(currentSystem.value(key).toString().split(","));
//                if(key=="config-prefix")
//                    currSystemConfPrefix=currentSystem.value(key).toString();
            }
            if(systemActivate(currentSystem,activeSystems)!=0){
                updateProgressText(tr("Failed to activate system. Will not proceed."));
                return 1;
            }
        }
    }
    updateProgressText(tr("Resolving variables"));
    resolveVariables();
    updateProgressText(tr("Activating subsystems"));
    foreach(QString option,activeOptions.keys())
        if(option.startsWith("subsystem.")){
            QString subsystemName = option;
            subsystemName.remove("subsystem.");
            foreach(int sub,subsystems.keys())
                if(subsystems.value(sub).value("enabler")==subsystemName)
                    subsystemActivate(subsystems.value(sub),activeOptions.value(option).toString(),activeSystems);
        }

    updateProgressText(tr("Resolving variables"));
    resolveVariables();

    updateProgressText(tr("Processing preruns and postruns"));
    foreach(QString key, activeOptions.keys()){
        QVariant instanceValue = activeOptions.value(key);
        foreach(QString system,activeSystems)
            if(key.startsWith(systemTable.value(system).toHash().value("config-prefix").toString())){ //I puked all over my keyboard while writing this.
                if(key.contains(".prerun")){
                    insertPrerunPostrun(jsonExamineArray(instanceValue.toJsonArray()),0);
                }
                if(key.contains(".postrun")){
                    insertPrerunPostrun(jsonExamineArray(instanceValue.toJsonArray()),1);
                }
            }
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


void JasonParser::variablesImport(QHash<QString,QVariant> variables){
    foreach(QString var,variables.keys()){
        QHash<QString,QVariant> varHash = variables.value(var).toHash();
        QString varType;
        foreach(QString option,varHash.keys())
            if(option=="type")
                varType = varHash.value(option).toString();
        if(varType=="config-input"){
            QString defaultValue = varHash.value("default").toString(); //insert default value if no option is found in activeOptions
            foreach(QString option,varHash.keys())
                if((option=="input")&&(varHash.contains("name"))){
                    QString variable = activeOptions.value(resolveVariable("%CONFIG_PREFIX%."+varHash.value(option).toString())).toString();
                    if(variable.isEmpty())
                        variable = defaultValue;
                    substitutes.insert(varHash.value("name").toString(),variable);
                }
        }else if(varHash.value("name").isValid()&&varHash.value("value").isValid()){
            setEnvVar(varHash.value("name").toString(),resolveVariable(varHash.value("value").toString()));
        }else
            broadcastMessage(1,"unsupported variable type"+varType);
    }
}


int JasonParser::parseUnderlyingObjects(QHash<QString, QHash<QString, QVariant> > underlyingObjects){
    QHash<QString, QVariant> importTable;
    QHash<QString, QVariant> variablesTable;
    QHash<QString, QVariant> subsystemTable;
    QHash<QString, QVariant> systemTTable;
    //Grab tables from inside the input table
    foreach(QString key, underlyingObjects.keys()){
        if(key=="imports")
            importTable = underlyingObjects.value(key);
        if(key=="variables")
            variablesTable = underlyingObjects.value(key);
        if(key=="subsystems")
            subsystemTable = underlyingObjects.value(key);
        if(key=="systems")
            systemTTable = underlyingObjects.value(key);
    }
    //Handle the tables in their different ways, nothing will be returned
    foreach(QString key,importTable.keys())
        foreach(QString kKey, importTable.value(key).toHash().keys())
            if(kKey=="file"){
                QString filename = importTable.value(key).toHash().value(kKey).toString();
                QJsonDocument importDoc = jsonOpenFile(filename);
                parseStage1(importDoc.object());
                importedFiles.append(filename);
            }

    foreach(QString key,variablesTable.keys()){
        foreach(QString kKey, variablesTable.value(key).toHash().keys())
            if(kKey=="name")
                variableHandle(variablesTable.value(key).toHash().value("name").toString(),variablesTable.value(key).toHash().value("value").toString());
    }
    foreach(QString key,subsystemTable.keys())
        subsystemHandle(subsystemTable.value(key).toHash());
    foreach(QString key,systemTTable.keys())
        if(systemHandle(systemTTable.value(key).toHash())!=0)
            return 1;

    return 0;
}


void JasonParser::resolveVariables(){
    QStringList systemVariables;
    //System variables that are used in substitution for internal variables. Messy indeed, but it kind of works in a simple way.
    foreach(QString opt, activeOptions.keys())
        if(opt.startsWith("shell.properties"))
            foreach(QString key, activeOptions.value(opt).toJsonObject().keys())
                if(key=="import-env-variables")
                    systemVariables=activeOptions.value(opt).toJsonObject().value(key).toString().split(",");
    if(systemVariables.isEmpty())
        broadcastMessage(1,tr("Note: No system variables were imported. This may be a bad sign.\n"));
    foreach(QString variable, systemVariables){
        QProcessEnvironment variableValue = QProcessEnvironment::systemEnvironment();
        substitutes.insert(variable,variableValue.value(variable));
    }


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
    if(!variable.contains("%"))
        return variable;
    return variable;
}


void JasonParser::desktopFileBuild(QJsonObject desktopObject){
/*
 * Desktop files execute by ./Jason [JSON-file]
 * A desktop action entry will use the appendage of --action [id] to make a distinction between the
 * different entries that may exist.
*/
    QString dName,dDesc,dWMClass,dIcon,dCat;
    QHash<QString,QVariant> containerHash = runtimeValues.value("launchables").toHash();
    foreach(QString key, desktopObject.keys()){
        if(key=="desktop.displayname")
            dName = desktopObject.value(key).toString();
        if(key=="desktop.description")
            dDesc = desktopObject.value(key).toString();
        if(key=="desktop.wmclass")
            dWMClass = desktopObject.value(key).toString();
        if(key=="desktop.icon")
            dIcon = desktopObject.value(key).toString();
        if(key=="desktop.categories")
            dCat = desktopObject.value(key).toString();
        if(key.startsWith("desktop.action.")){
            QJsonObject action = desktopObject.value(key).toObject();
            QString aName,aExec,aID,aWD,aCPrefix,aLPrefix;
            foreach(QString actKey, action.keys()){
                if(actKey.endsWith(".exec")){
                    if(!actKey.split(".")[0].isEmpty())
                        aCPrefix = actKey.split(".")[0];
                    aExec = action.value(actKey).toString();
                }
                if(actKey.endsWith(".workdir"))
                    aWD = action.value(actKey).toString();
                if(actKey=="action-id")
                    aID = action.value(actKey).toString();
                if(actKey=="desktop.displayname")
                    aName = action.value(actKey).toString();
            }
            if(!aCPrefix.isEmpty()){
                foreach(QString system,systemTable.keys())
                    if(systemTable.value(system).toHash().value("config-prefix").toString()==aCPrefix)
                        if(!systemTable.value(system).toHash().value("launch-prefix").toString().isEmpty())
                            aLPrefix = systemTable.value(system).toHash().value("launch-prefix").toString();
            }
            QHash<QString,QVariant> actionHash;
            actionHash.insert("command",aExec);
            if(!aWD.isEmpty())
                actionHash.insert("workingdir",aWD);
            if(!aLPrefix.isEmpty())
                actionHash.insert("launch-prefix",aLPrefix);
            actionHash.insert("displayname",aName);
            containerHash.insert(aID,actionHash);
        }
    }
    QHash<QString,QVariant> desktopHash;
    desktopHash.insert("displayname",dName);
    desktopHash.insert("description",dDesc);
    desktopHash.insert("wmclass",dWMClass);
    desktopHash.insert("categories",dCat);
    desktopHash.insert("icon",dIcon);
    runtimeValues.remove("launchables");
    containerHash.insert("default.desktop",desktopHash);
    runtimeValues.insert("launchables",containerHash);
    return;
}

void JasonParser::environmentActivate(QHash<QString,QVariant> environmentHash,QStringList activeSystems){
    QString type;
    QStringList activeSystemsConfig;
    foreach(QString system, activeSystems)
        activeSystemsConfig.append(systemTable.value(system).toHash().value("config-prefix").toString());
    //Get type in order to determine how to treat this environment entry
    foreach(QString key, environmentHash.keys()){
        if(key=="type")
            type = environmentHash.value(key).toString();
    }
    //Treat the different types according to their parameters and realize their properties
    if(type=="run-prefix"){
        foreach(QString key,environmentHash.keys())
            if(key.endsWith(".exec.prefix")) //Execution prefix, given that the system is active
                foreach(QString system,activeSystemsConfig)
                    if(key.split(".")[0]==system){
                        QString execLine = resolveVariable(environmentHash.value(key).toString());
                        QHash<QString,QVariant> runtimeReturnHash;
                        runtimeReturnHash.insert("command",resolveVariable(execLine));
                        addToRuntime("run-prefix",runtimeReturnHash);
                    }
    }else if(type=="run-suffix"){
        foreach(QString key,environmentHash.keys())
            if(key.endsWith(".exec.suffix")) //Execution prefix, given that the system is active
                foreach(QString system,activeSystemsConfig)
                    if(key.split(".")[0]==system){
                        QHash<QString,QVariant> runtimeReturnHash;
                        runtimeReturnHash.insert("command",resolveVariable(environmentHash.value(key).toString()));
                        addToRuntime("run-suffix",runtimeReturnHash);
                    }
    }else if(type=="variable"){
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

int JasonParser::runProcesses(QString launchId){
    /*
     *
     * launchId:
     *  - Either is empty (thus launching the default program) or filled with an ID. All preruns and postruns are
     * run as normal before and after, but prefixes and suffixes are not added.
     *
     * Runmodes:
     *  - 0: Launch action or default option
    */
    if(launchId.isEmpty())
        launchId = "default";

    QList<QVariant> runPrefix;
    QList<QVariant> runSuffix;
    QHash<QString,QVariant> launchables;
    foreach(QString element, runtimeValues.keys()){
        if((element=="launchables")&&(!runtimeValues.value(element).toHash().isEmpty()))
            launchables = runtimeValues.value(element).toHash();
    }

    //This is where the magic happens.
    // We need to see if the 'global.detachable-process'-key is true
    bool isDetach = false;
    if(launchId=="default")
        foreach(QString key, activeOptions.keys())
            if(key=="global.detachable-process")
                isDetach=activeOptions.value(key).toBool();

    QString desktopTitle;
    foreach(QString launchable,launchables.keys())
        if(launchable==launchId){
            QString argument,program,workDir,name;
            QHash<QString,QVariant> launchObject = launchables.value(launchable).toHash();
            foreach(QString key,launchObject.keys()){
                if(key=="command")
                    argument=resolveVariable(launchObject.value(key).toString());
                if(key=="launch-prefix")
                    program=resolveVariable(launchObject.value(key).toString());
                if(key=="workingdir")
                    workDir=resolveVariable(launchObject.value(key).toString());
                if(key=="desktop.title")
                    name=resolveVariable(launchObject.value(key).toString());
            }
            if(launchId=="default"){
                foreach(QString key,launchables.value("default.desktop").toHash().keys())
                    if(key=="displayname")
                        name=resolveVariable(launchables.value("default.desktop").toHash().value(key).toString());
            }
            desktopTitle=name;
            updateProgressTitle(name);
            if((doHideUi))
                toggleProgressVisible(false);
            connect(this,SIGNAL(mainProcessEnd()),SLOT(doPostrun()));
            if(!argument.isEmpty())
                executeProcess(argument,program,workDir,name,runprefixStr,runsuffixStr);
            if((doHideUi))
                toggleProgressVisible(true);
        }
    //Aaaand it's gone.

    if((isDetach)){
        emit displayDetachedMessage(desktopTitle);
    }else
        emit mainProcessEnd();
    return 0;
}

void JasonParser::executeProcess(QString argument, QString program, QString workDir, QString title, QString runprefix, QString runsuffix){
    /*
     * program - Prefixed to argument, specifically it could be 'wine' or another frontend program such as
     * 'mupen64plus'. It is not supposed to run shells.
     * argument - The command line, only in this context it is indeed an argument.
     * workDir - The wished working directory for the operation.
    */

    /*
     * TODO:
     *  - Insert prefix and suffix into the command line by way of prepending and appending.
     *  - Show some GUI magic run by a separate thread as a user-friendly indicator for progress, may also
     *      want to show a dialog button for detaching processes so that postrun doesn't run prematurely.
     *  - Implement programming to pick up aforementioned option
     *  - Add the desktop file generation function and decide how the Exec option should look
     *  - Add even more pretty GUIs
     *
    */

    QString shell = "sh"; //Just defaults, in case nothing is specified.
    QString shellArg = "-c"; //We don't want the shell to pick up more arguments.
    QJsonObject shellOptions = activeOptions.value("kaidan-opts").toJsonObject();
    foreach(QString key,shellOptions.keys()){
        if(key=="shell")
            shell=shellOptions.value(key).toString();
        if(key=="command.argument")
            shellArg=shellOptions.value(key).toString();
    }
    if(shell.isEmpty())
        return;

    QProcess *executer = new QProcess(this);
    QStringList arguments;
    executer->setProcessEnvironment(procEnv);
    executer->setProcessChannelMode(QProcess::SeparateChannels);
    if(!workDir.isEmpty())
        executer->setWorkingDirectory(workDir);
    executer->setProgram(shell);
    arguments.append(shellArg);
    arguments.append("--");

    QString execString;
    execString = program+" "+argument;
    //We apply the prefixes/suffixes
    if(!runprefix.isEmpty())
        execString.prepend(runprefix+" ");
    if(!runsuffix.isEmpty())
        execString.append(" "+runsuffix);

    arguments.append(execString);
    executer->setArguments(arguments);

    //Connect signals and slots as well as update the title.
    connect(executer, SIGNAL(finished(int,QProcess::ExitStatus)),SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(executer, SIGNAL(error(QProcess::ProcessError)),SLOT(processOutputError(QProcess::ProcessError)));
    connect(executer,SIGNAL(started()),SLOT(processStarted()));
    updateProgressText(title);

    executer->start();
    executer->waitForFinished(-1);
    if((executer->exitCode()!=0)||(executer->exitStatus()!=0)){
        qDebug() << "showing output";
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
    if(exitCode!=0)
        broadcastMessage(1,tr("Process exited with status: %1.\n").arg(QString::number(exitCode)));
    if(exitStatus!=0)
        broadcastMessage(1,tr("QProcess exited with status: %1.\n").arg(QString::number(exitStatus)));
}

void JasonParser::processOutputError(QProcess::ProcessError processError){
    broadcastMessage(2,tr("QProcess exited with status: %1.\n").arg(QString::number(processError)));
    emit processFailed(processError);
}

void JasonParser::processStarted(){

}

void JasonParser::detachedMainProcessClosed(){
    emit mainProcessEnd();
}

void JasonParser::generateDesktopFile(QString desktopFile, QString jasonPath, QString inputDoc){
    //Not a very robust function, but I believe it is sufficient for its purpose.
    QFile outputDesktopFile;
    if(!desktopFile.endsWith(".desktop"))
        broadcastMessage(1,tr("Warning: The filename specified for the output .desktop file does not have the correct extension.\n"));
    outputDesktopFile.setFileName(desktopFile);
    if(outputDesktopFile.exists()){
        emit toggleCloseButton(true);
        updateProgressText(tr("The file exists. Will not proceed."));
        emit failedProcessing();
        return;
    }
    if(!outputDesktopFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        emit toggleCloseButton(true);
        updateProgressText(tr("Failed to open the output file for writing. Will not proceed."));
        emit failedProcessing();
        return;
    }
    QString desktopActions;
    QString outputContents;
    outputContents.append("[Desktop Entry]\nVersion=1.0\nType=Application\nTerminal=False\nExec='%JASON_EXEC%' '%INPUT_FILE%'\n%DESKTOP_ACTION_LIST%\n");
    foreach(QString entry,runtimeValues.keys())
        if(entry=="launchables"){
            QHash<QString,QVariant> launchables = runtimeValues.value(entry).toHash();
            foreach(QString key,launchables.keys()){
                if(key=="default.desktop"){
                    QHash<QString,QVariant> defaultDesktop = launchables.value(key).toHash();
                    foreach(QString dKey,defaultDesktop.keys()){
                        if(dKey=="displayname")
                            outputContents.append("Name="+defaultDesktop.value(dKey).toString()+"\n");
                        if((dKey=="description")&&(!defaultDesktop.value(dKey).toString().isEmpty()))
                            outputContents.append("Comment="+defaultDesktop.value(dKey).toString()+"\n");
                        if((dKey=="wmclass")&&(!defaultDesktop.value(dKey).toString().isEmpty()))
                            outputContents.append("StartupWMClass="+defaultDesktop.value(dKey).toString()+"\n");
                        if((dKey=="icon")&&(!defaultDesktop.value(dKey).toString().isEmpty()))
                            outputContents.append("Icon="+defaultDesktop.value(dKey).toString()+"\n");
                        if((dKey=="categories")&&(!defaultDesktop.value(dKey).toString().isEmpty()))
                            outputContents.append("Categories="+defaultDesktop.value(dKey).toString()+"\n");
                    }
                }
            }
            foreach(QString key,launchables.keys()){
                if((key!="default")&&(key!="default.desktop")){
                    QHash<QString,QVariant> desktopAction = launchables.value(key).toHash();
                    QString desktopActionEntry;
                    foreach(QString dKey,desktopAction.keys())
                        if(dKey=="displayname"){
                            desktopActions.append(key+";");
                            desktopActionEntry = "[Desktop Action "+key+"]\nName="+desktopAction.value(dKey).toString()+"\nExec='%JASON_EXEC%' --action "+key+" '%INPUT_FILE%'\n";
                        }
                    outputContents.append("\n"+desktopActionEntry);
                }
            }
        }
    if(!jasonPath.isEmpty()){
        QFileInfo jasonInfo(jasonPath);
        outputContents.replace("%JASON_EXEC%",jasonInfo.canonicalFilePath());
    }
    if(!jasonPath.isEmpty()){
        QFileInfo docInfo(inputDoc);
        outputContents.replace("%INPUT_FILE%",docInfo.canonicalFilePath());
        outputContents.replace("%WORKINGDIR%",docInfo.canonicalPath());
    }
    if(!desktopActions.isEmpty()){
        outputContents.replace("%DESKTOP_ACTION_LIST%","Actions="+desktopActions);
    }else
        outputContents.replace("%DESKTOP_ACTION_LIST%",QString());

    QTextStream outputDocument(&outputDesktopFile);
    outputDocument << outputContents;
    outputDesktopFile.setPermissions(QFile::ExeOwner|outputDesktopFile.permissions());
    outputDesktopFile.close();

    updateProgressText(tr("Desktop file was generated successfully."));

    emit toggleCloseButton(true);
    emit finishedProcessing();
}
