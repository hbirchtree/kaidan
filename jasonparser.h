#ifndef JASONPARSER_H
#define JASONPARSER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <stdio.h>
#include <QString>
#include <QStringList>

#include <QProcess>
#include <QProcessEnvironment>
#include <QEventLoop>
#include <QThread>

class JasonParser : public QObject
{
    Q_OBJECT
public:
    JasonParser();
    ~JasonParser();

    //General
    void testEnvironment();
    void setStartOpts(QString startDocument);

    int exitResult;

public slots:
    void processStarted();
    void startParse();

private slots:
    void processOutputError(QProcess::ProcessError processError);
    void processFinished(int exitCode,QProcess::ExitStatus exitStatus);
    void forwardSecondaryProgress(int value);

signals:

    //Related to the general look and workings
    void finishedProcessing();
    void failedProcessing();
    //Directly about the GUI
    void updateProgressText(QString);
    void updateProgressTitle(QString);
    void updateProgressIcon(QString);
    void broadcastMessage(int,QString);
    void changeProgressBarRange(qint64,qint64); //0,0 will make it indefinite, something else will make it normal.
    void changeProgressBarValue(int);
    void changeSProgressBarRange(qint64,qint64);
    void changeSProgressBarValue(int);
    void changeSProgressBarVisibility(bool);
    void updateIconSize(int);

    //Related to processes
    void mainProcessStart();
    void mainProcessEnd();
    void processFailed(QProcess::ProcessError);
    void processReturnOutput();
    void emitOutput(QString,QString);

private:
    //General
    QHash<QString, QString> startOpts;
    int jsonParse(QJsonDocument jDoc);
    //Sections of parsing process
    int parseStage1(QJsonObject mainObject);
    int parseStage2(QJsonObject mainObject);

    //JSON
    QJsonDocument jsonOpenFile(QString filename);
    QHash<QString,QVariant> jsonExamineArray(QJsonArray jArray);
    QVariant jsonExamineValue(QJsonValue jValue);
    QHash<QString,QVariant> jsonExamineObject(QJsonObject jObject);

    //Handlers
    void setEnvVar(QString key, QString value);
    QProcessEnvironment procEnv;
    void variableHandle(QString key, QString value);
    void resolveVariables();
    QString resolveVariable(QString variable);

    QProcessEnvironment createProcEnv(QJsonArray environment);

    //Activate options
    void environmentActivate(QHash<QString,QVariant> environmentHash);

    //Fucking finally
    int runProcesses(QJsonArray stepsArray);
    int executeProcess(QString programs,QStringList arguments,QProcessEnvironment localProcEnv,QString workDir,bool lazyExitStatus);
    QProcess *executer;

    //Kaidan steps
    int evaluateKaidanStep(QJsonObject kaidanStep); //We use this to see if it's a valid step
    int executeKaidanStep(QJsonObject kaidanStep);

    //Payload operations, they do recursion. We should optimize it so that we won't encounter a stack overflow.
    int copyDirectory(QString oldName, QString newName, bool caseSense, bool overwriteFile,bool appendFile);
    int applyPayloadFile(QString sourceFilename, QString targetFilename, QFileInfo *entryInfo, bool overwriteFile, bool appendFile);
    QFileInfo *entryInfo;
    QFileInfo *destEntryInfo;
    QFile *fileOperator;
    QFile *destFileOperator;

    //Hashes/arrays/vectors
    QHash<QString, QString> substitutes;
    QHash<QString,QVariant> activeOptions;

};

#endif // JASONPARSER_H
