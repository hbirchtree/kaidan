#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QDebug>

#include <QAuthenticator>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QList>
#include <QIODevice>

class Downloader : public QObject
{
    Q_OBJECT
public:
    explicit Downloader(QObject *parent = 0);
    ~Downloader();
    QNetworkAccessManager netman;

public slots:
    void downloadUrl(QString url,QString outputFile);
    void startDownloads();

private:
    QList<QStringList> queuedForDownload;
    QNetworkReply *netRep;
    QNetworkRequest *netReq;
    QStringList *itemList;

    QList<QStringList> queuedFiles;

private slots:
    void addToQueue(QUrl url, QString outputFile);
    void dlFinished(QNetworkReply *netReply);

    //Signal emitters
    void networkSessionConnected();
    void authenticationRequired(QNetworkReply *netReply,QAuthenticator *auth);
    void dlSslError(QNetworkReply *netReply,QList<QSslError> sslError);
    void dlProgress(qint64 curr,qint64 total);
    void dlError(QNetworkReply::NetworkError error);

signals:
    void finishedDownload();
    void failedDownload();
    void reportDownloadProgress(int);
    void networkConnected();
    void netmanSslError(QNetworkReply*,QList<QSslError>);
    void requestAuthentication(QNetworkReply*,QAuthenticator*);
    void netmanGeneralError(QNetworkReply::NetworkError);
};

#endif // DOWNLOADER_H
