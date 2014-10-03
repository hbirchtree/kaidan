#ifndef JASONPARSER_H
#define JASONPARSER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <stdio.h>
#include <QString>
#include <QStringList>

#include <QProcess>
#include <QProcessEnvironment>
#include <QEventLoop>

class JasonParser : public QObject
{
    Q_OBJECT
public:
    JasonParser();
    ~JasonParser();

    //General
    void testEnvironment();
    void setStartOpts(QString startDocument, QString jasonPath);

    int exitResult;

public slots:
    void processStarted();
    void startParse();

private slots:
    void processOutputError(QProcess::ProcessError processError);
    void processFinished(int exitCode,QProcess::ExitStatus exitStatus);

signals:

    //Related to the general look and workings
    void finishedProcessing();
    void failedProcessing();
    //Directly about the GUI
    void toggleCloseButton(bool);
    void updateProgressText(QString);
    void updateProgressTitle(QString);
    void broadcastMessage(int,QString);
    void toggleProgressVisible(bool);
    void changeProgressBarRange(int,int); //0,0 will make it indefinite, something else will make it normal.
    void changeProgressBarValue(int);

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
    void stage2ActiveOptionAdd(QJsonValue instanceValue,QString key);

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
    int parseUnderlyingObjects(QHash<QString, QHash<QString,QVariant> > underlyingObjects);

    //Activate options
    void environmentActivate(QHash<QString,QVariant> environmentHash);
    void variablesImport(QHash<QString,QVariant> variables);

    //Fucking finally
    int runProcesses();
    void executeProcess(QString argument,QString program,QString workDir, QString title);
    QProcess *executer;

    //Hashes/arrays/vectors
    QHash<QString, QString> substitutes;
    QHash<QString,QVariant> activeOptions;
    QHash<QString,QVariant> runSteps;

};

#endif // JASONPARSER_H
