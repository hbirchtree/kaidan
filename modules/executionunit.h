#ifndef EXECUTIONUNIT_H
#define EXECUTIONUNIT_H

#include <QObject>
#include <QString>
#include "modules/systemcontainer.h"
#include "modules/environmentcontainer.h"
#include "modules/variablehandler.h"

class ExecutionUnit : public QObject
{
    Q_OBJECT
public:
    explicit ExecutionUnit(QObject *parent = 0,VariableHandler* varHandler = 0, QHash<QString,QVariant> inputHash = QHash<QString,QVariant>(),QList<SystemContainer*> systems = QList<SystemContainer*>());
    ExecutionUnit(QObject *parent, VariableHandler* varHandler, QMap<QString, QVariant> inputHash);

    ~ExecutionUnit();

    QString getExecString(){
        return m_execString;
    }
    QString getExecType(){
        return m_execType;
    }
    bool isLazyExit(){
        return m_lazyExit;
    }
    bool isDetachable(){
        return m_detachable;
    }
    bool startsDetached(){
        return m_startsDetached;
    }
    QString getWorkDir(){
        return m_workDir;
    }
    EnvironmentContainer* getEnvironment(){
        return envContainer;
    }
    QString getTitle(){
        return d_title;
    }
    QString getIcon(){
        return d_icon;
    }

    void resolveVariables(VariableHandler* varHandler){
        m_execString = varHandler->resolveVariable(m_execString);
        m_workDir = varHandler->resolveVariable(m_workDir);
        d_title = varHandler->resolveVariable(d_title);
        d_icon = varHandler->resolveVariable(d_icon);
    }

    void addFixation(EnvironmentContainer* envContainer){
        m_execString.prepend(envContainer->getRunPrefix()).append(envContainer->getRunSuffix());
    }

    bool isValid(){
        return s_validUnit;
    }

signals:

public slots:

private:
    bool s_validUnit = false;

    QString d_title;
    QString d_icon;

    QString m_execString;
    QString m_execType;
    QString m_execCommand;
    bool m_lazyExit = false;
    bool m_detachable = false;
    bool m_startsDetached = false;
    QString m_workDir;
    EnvironmentContainer* envContainer = 0;
};

#endif // EXECUTIONUNIT_H
