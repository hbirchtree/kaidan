#ifndef JASONPARSER_H
#define JASONPARSER_H

#include "executer.h"
#include "jsonparser.h"
#include "desktoptools.h"

#include <QMetaObject>
#include <QFileInfo>
#include <stdio.h>
#include <QString>
#include <QStringList>
#include <QHash>

#include <QProcessEnvironment>
#include <QEventLoop>

#include "modules/uiglue.h"

class JasonParser : public UIGlue
{
    Q_OBJECT
public:
    JasonParser();
    ~JasonParser();
    void setStartDoc(QString startDoc){
        if(startDoc.isEmpty())
           return;
        this->startDoc = startDoc;
        checkValidity();
    }
    bool isReady(){
        return b_validState;
    }


public slots:
    void startParse();
    void detachedProgramExit();

private:
    void checkValidity(){
        if(startDoc.isEmpty())
            return;
        b_validState = true;
    }

    QString startDoc;
    bool b_validState = false;

    int exitResult;

    QList<QMetaObject::Connection> connectedSlots;

    jsonparser *parser;
    QHash<QString,QVariant> *jsonFinalData;
    QHash<QString,QVariant> *runtimeValues;
    QEventLoop *waitLoop;

    void quitProcess();
    void hookupStep(KaidanStep *step);
};

#endif // JASONPARSER_H
