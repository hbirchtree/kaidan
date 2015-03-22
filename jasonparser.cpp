#include "jasonparser.h"

#include <QDebug>

JasonParser::JasonParser(){

}

JasonParser::~JasonParser(){
    for(QMetaObject::Connection cnct : connectedSlots)
        disconnect(cnct);
    delete parser;
}

void JasonParser::quitProcess(){
    emit changeProgressBarRange(0,100);
    emit changeProgressBarValue(100);
    emit failedProcessing();
}

void JasonParser::startParse(){
    updateProgressText(tr("Starting to parse JSON document"));

    exitResult=0;

    parser = new jsonparser();

    connectedSlots.append(connect(parser,&jsonparser::sendProgressTextUpdate,[=](QString message){this->updateProgressText(message);}));
    connectedSlots.append(connect(parser,&jsonparser::failedProcessing,[=](){this->failedProcessing();}));
    connectedSlots.append(connect(parser,&jsonparser::reportError,[=](int severity,QString message){exitResult++;this->broadcastMessage(severity,message);}));
    connectedSlots.append(connect(parser,&jsonparser::sendProgressBarUpdate,[=](int value){this->changeProgressBarValue(value);}));

    if(parser->jsonParse(parser->jsonOpenFile(startDoc))!=0){
        quitProcess();
        return;
    }
    QHash<QString,KaidanStep*> runQueue = parser->getRunQueue();

    QString nextStep;
    for(KaidanStep* step : runQueue.values())
        if(step->isFirstStep()){
            step->performStep();
            nextStep = step->getNextStep();
            break;
        }
    while(!nextStep.isEmpty()){
        KaidanStep* cStep = runQueue.value(nextStep);
        nextStep = cStep->getNextStep();
        if(cStep->performStep()!=0){
            quitProcess();
            return;
        }
    }

    updateProgressText(tr("All done!"));
    emit finishedProcessing();
    return;
}

void JasonParser::hookupStep(KaidanStep* step){
    connect(step,&KaidanStep::updateProgressText,[=](QString text){
        updateProgressText(text);
    });
    connect(step,&KaidanStep::updateProgressIcon,[=](QString text){
        updateProgressIcon(text);
    });
    connect(step,&KaidanStep::changeSecondaryProgressBarVisibility,[=](bool visible){
        changeSecondProgressBarVisibility(visible);
    });
    connect(step,&KaidanStep::updateSecondaryProgress,[=](int value){
        changeSecondProgressBarValue(value);
    });
}

void JasonParser::detachedProgramExit(){
    emit detachedRunEnd();
}
