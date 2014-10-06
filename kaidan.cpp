#include "kaidan.h"
#include "ui_kaidan.h"
#include "jasonparser.h"

Kaidan::Kaidan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Kaidan)
{
    ui->setupUi(this);
    ui->progressBar_2->setHidden(true);
}

Kaidan::~Kaidan()
{
    delete ui;
}

void Kaidan::initializeParse(QString startDocument){
    startDocument = "/home/havard/Code/kaidan-test/kaidan-test.json";
    QFile startDocTest(startDocument);
    if((startDocument.isEmpty())||(!startDocTest.exists())){
        QFileDialog getInputFile;
        QDir canonPath;
        startDocument = getInputFile.getOpenFileName(this,tr("Select the Kaidan file"),canonPath.absoluteFilePath(startDocument),tr("JSON file %1").arg("(*.json)"));
    }

    JasonParser parser;
    QThread parserThread;
    parser.moveToThread(&parserThread);
    parser.setStartOpts(startDocument);
    //Related to thread handling
    connect(&parser,SIGNAL(failedProcessing()),&parserThread,SLOT(quit()));
    connect(&parser,SIGNAL(finishedProcessing()),&parserThread,SLOT(quit()));
    connect(&parser,SIGNAL(finishedProcessing()),this,SLOT(handleFinished()));
    connect(&parser,SIGNAL(failedProcessing()),this,SLOT(handleFailed()));
    connect(&parserThread,SIGNAL(started()),&parser,SLOT(startParse()));
    //Cosmetics
    connect(&parser,SIGNAL(broadcastMessage(int,QString)),this,SLOT(displayMessage(int,QString)));
    connect(&parser,SIGNAL(emitOutput(QString,QString)),this,SLOT(displayLogs(QString,QString)));
    connect(&parser,SIGNAL(changeProgressBarRange(qint64,qint64)),this,SLOT(updateMainProgressBarLimits(qint64,qint64)));
    connect(&parser,SIGNAL(changeProgressBarValue(int)),this,SLOT(updateMainProgressBarValue(int)));
    connect(&parser,SIGNAL(changeSProgressBarRange(qint64,qint64)),this,SLOT(updateSecondProgressBarLimits(qint64,qint64)));
    connect(&parser,SIGNAL(changeSProgressBarValue(int)),this,SLOT(updateSecondaryProgressBarValue(int)));
    connect(&parser,SIGNAL(changeSProgressBarVisibility(bool)),this,SLOT(modifySecondPBarVisibility(bool)));
    connect(&parser,SIGNAL(updateProgressIcon(QString)),this,SLOT(updateProgressIcon(QString)));
    connect(&parser,SIGNAL(updateProgressText(QString)),this,SLOT(updateProgressText(QString)));
    connect(&parser,SIGNAL(updateProgressTitle(QString)),this,SLOT(updateWindowTitle(QString)));
    connect(&parser,SIGNAL(updateIconSize(int)),this,SLOT(setIconLabelSize(int)));
    QEventLoop workerLoop;
    connect(&parserThread,SIGNAL(finished()),&workerLoop,SLOT(quit()));
    parserThread.start();
    show();
    workerLoop.exec();
}

void Kaidan::displayMessage(int status, QString message){
    QMessageBox *messageBox = new QMessageBox(this);
    messageBox->setText(message);
    if(status==0)
        messageBox->setWindowTitle(tr("Jason information"));
    if(status==1)
        messageBox->setWindowTitle(tr("Jason warning"));
    if(status==2)
        messageBox->setWindowTitle(tr("Jason error"));
    messageBox->show();
}

void Kaidan::displayLogs(QString stdOut, QString stdErr){
    QDialog *outputWindow = new QDialog(this);
    QGridLayout *outputLayout = new QGridLayout(this);
    QTextEdit *errEdit = new QTextEdit(this);
    QTextEdit *outEdit = new QTextEdit(this);
    errEdit->setReadOnly(true);
    outEdit->setReadOnly(true);
    errEdit->setText(stdErr);
    outEdit->setText(stdOut);
    outputLayout->addWidget(outEdit,1,1);
    outputLayout->addWidget(errEdit,1,2);
    outputWindow->setLayout(outputLayout);
    outputWindow->show();
}

void Kaidan::updateProgressText(QString newText){
    if(!newText.isEmpty())
        ui->infoLabel->setText(newText);
}

void Kaidan::updateProgressIcon(QString newIconPath){
    QFile *imageFile = new QFile(newIconPath);
    if(!imageFile->exists())
        return;
    QImage newIcon(newIconPath);
    qreal iconScale = ui->iconLabel->height();
    ui->iconLabel->setPixmap(QPixmap::fromImage(newIcon.scaled(iconScale,iconScale)));
}

void Kaidan::setIconLabelSize(int size){
    int oldSize = ui->iconLabel->height();
    ui->iconLabel->setMinimumSize(size,size);
    ui->iconLabel->setMaximumSize(size,size);
    setMinimumHeight(height()-oldSize+size);
    setMaximumHeight(height()-oldSize+size);
}

void Kaidan::updateWindowTitle(QString newTitle){
    if(!newTitle.isEmpty())
        setWindowTitle(newTitle);
}

void Kaidan::updateMainProgressBarLimits(qint64 min, qint64 max){
    ui->progressBar->setMinimum(min);
    ui->progressBar->setMaximum(max);
}

void Kaidan::updateMainProgressBarValue(int val){
    ui->progressBar->setValue(val);
}

void Kaidan::updateSecondaryProgressBarValue(int val){
    ui->progressBar_2->setValue(val);
}

void Kaidan::updateSecondProgressBarLimits(qint64 min, qint64 max){
    ui->progressBar_2->setMinimum(min);
    ui->progressBar_2->setMaximum(max);
}

void Kaidan::modifySecondPBarVisibility(bool doShow){
    ui->progressBar_2->setHidden(!doShow); //Inverted statement because setHidden does the opposite
}

void Kaidan::handleFinished(){
    updateMainProgressBarLimits(0,100);
    updateMainProgressBarValue(100);
    ui->progressBar->setTextVisible(true);
    modifySecondPBarVisibility(false);
    updateProgressText(tr("Everything done."));
}

void Kaidan::handleFailed(){
    updateMainProgressBarLimits(0,100);
    updateMainProgressBarValue(100);
    ui->progressBar->setTextVisible(true);
    modifySecondPBarVisibility(false);
}
