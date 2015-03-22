#include "kaidanprocedure.h"

KaidanProcedure::KaidanProcedure(QObject *parent,VariableHandler* varHandler,EnvironmentContainer* envContainer) : QObject(parent)
{
    this->varHandler = varHandler;
    this->envContainer = envContainer;
}

KaidanProcedure::~KaidanProcedure()
{

}

QHash<QString,KaidanStep*> KaidanProcedure::resolveDependencies(QVariantMap *totalMap){
    varHandler->variableHandle("STARTDIR",QDir::currentPath());
    QList<KaidanStep*> steps;
    for(auto it=totalMap->begin();it!=totalMap->end();it++){
        if(it.key()=="variables")
            varHandler->variablesImport(it.value().toList());
        if(it.key()=="environment")
            for(QVariant env : it.value().toList())
                envContainer->environmentImport(StatFuncs::mapToHash(env.toMap()));
    }
    varHandler->resolveVariables();
    envContainer->resolveVariables();

    for(auto it=totalMap->begin();it!=totalMap->end();it++){
        if(it.key()=="steps")
            for(QVariant step : it.value().toList())
                steps.append(new KaidanStep(step.toMap(),varHandler,envContainer,shell,shellArg));
    }
    QHash<QString,KaidanStep*> runQueue;
    for(KaidanStep* step : steps){
        runQueue.insert(step->getStepName(),step);
    }
    return runQueue;
}
