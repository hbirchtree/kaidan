#ifndef FILESYSTEMACTIONS_H
#define FILESYSTEMACTIONS_H

#include <QObject>
#include <QHash>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QString>
#include <QDebug>

class FilesystemActions : public QObject
{
    Q_OBJECT

    enum FileOperation {
        FILE_APPEND,FILE_COPY,FILE_REMOVE,
        DIR_CREATE,DIR_COPY,DIR_REMOVE
    };

public:
    FilesystemActions(QString sourceFile = QString(),QString targetFile = QString(),bool overwrite = false,bool append = false);
    ~FilesystemActions();
    int installPayload();
private:
    QString sourceFile;
    QString targetFile;
    bool overwriteFile = false;
    bool appendFile = false;

    int copyDirectory(QString oldName, QString newName, bool caseSense, bool overwriteFile, bool appendFile);
    int applyPayloadFile(QString sourceFilename, QString targetFilename, QFileInfo *entryInfo, bool overwriteFile, bool appendFile);

    void logFileAction(QFileInfo *target,FileOperation detail){
        QFile logFile("kaidan-file-actions.log");
        if(!logFile.open(QIODevice::WriteOnly|QIODevice::Append))
            return;
        QTextStream out(&logFile);
        out << fileOperations.value(detail) << "{" << target->absoluteFilePath() << "}{" << " -> " << target->permissions() << "}\n";
        logFile.close();
    }

    QHash<FileOperation,QString> fileOperations;

signals:
    void updateProgressText(QString);
};

#endif // FILESYSTEMACTIONS_H
