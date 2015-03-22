#include <QCoreApplication>
#include "downloader.h"

Downloader::Downloader(QObject *parent){
    connect(&netman,SIGNAL(networkSessionConnected()),this,SLOT(networkSessionConnected()));
    connect(&netman,SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),this,SLOT(authenticationRequired(QNetworkReply*,QAuthenticator*)));
    connect(&netman,SIGNAL(finished(QNetworkReply*)),SLOT(dlFinished(QNetworkReply*)));
    connect(&netman,SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),this,SLOT(dlSslError(QNetworkReply*,QList<QSslError>)));
}

Downloader::~Downloader(){
    deleteLater();
}

void Downloader::downloadUrl(QString url, QString outputFile){
    QStringList thisDownload;
    thisDownload.append(url);
    thisDownload.append(outputFile);
    queuedForDownload.append(thisDownload);
}

void Downloader::startDownloads(){
    foreach(QStringList download,queuedForDownload){
        addToQueue(QUrl(download.at(0)),download.at(1));
    }
}

void Downloader::addToQueue(QUrl url, QString outputFile){
    QNetworkRequest *netReq = new QNetworkRequest(url);
    QNetworkReply *netReply = netman.get(*netReq);
    connect(netReply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(dlProgress(qint64,qint64)));
    connect(netReply,SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(dlError(QNetworkReply::NetworkError)));
    QStringList *itemList = new QStringList;
    itemList->append(url.toString());
    itemList->append(outputFile);
    queuedFiles.append(*itemList);
    qDebug() << "downloader_addQueue";
}

void Downloader::dlFinished(QNetworkReply *netReply){
    QString outputFilename;
    foreach(QStringList nDl, queuedFiles){
        if(nDl[0] == netReply->url().toString()){
            outputFilename = nDl[1];
            queuedFiles.removeOne(nDl);
        }
    }
    QFile outputFile;
    QString testName = outputFilename;
    int inc = 0;
    while(outputFile.fileName().isEmpty()){
        if(testName.isEmpty())
            testName="Downloaded";
        outputFile.setFileName(testName);
        if(outputFile.exists()){
            inc++;
            testName.append("."+inc);
        }else
            outputFile.setFileName(testName);
    }
    if(!outputFile.open(QIODevice::WriteOnly))
        return;
    if(netReply->isReadable())
        outputFile.write(netReply->readAll());
    outputFile.close();
    netReply->close();
    emit finishedDownload();
    qDebug() << "downloader_finished";
}

void Downloader::dlProgress(qint64 curr, qint64 total){
    emit reportDownloadProgress(curr*100/total);
}

void Downloader::dlError(QNetworkReply::NetworkError error){
    emit netmanGeneralError(error);
    emit failedDownload();
}

void Downloader::networkSessionConnected(){
    emit networkConnected();
}

void Downloader::dlSslError(QNetworkReply *netReply, QList<QSslError> sslError){
    emit netmanSslError(netReply,sslError);
    emit failedDownload();
}

void Downloader::authenticationRequired(QNetworkReply *netReply, QAuthenticator *auth){
    emit requestAuthentication(netReply,auth);
}
