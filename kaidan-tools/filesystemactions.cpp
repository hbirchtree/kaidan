#include "filesystemactions.h"

FilesystemActions::FilesystemActions(QString sourceFile,QString targetFile,bool overwrite,bool append)
{
    this->sourceFile = sourceFile;
    this->targetFile = targetFile;
    this->overwriteFile = overwrite;
    this->appendFile = append;

    fileOperations.insert(FILE_APPEND,"APP");
    fileOperations.insert(FILE_REMOVE,"RM");
    fileOperations.insert(FILE_COPY,"CP");
    fileOperations.insert(DIR_CREATE,"MKDIR");
    fileOperations.insert(DIR_REMOVE,"RMDIR");
    fileOperations.insert(DIR_COPY,"CPR");
}

FilesystemActions::~FilesystemActions()
{

}


int FilesystemActions::applyPayloadFile(QString sourceFilename, QString targetFilename, QFileInfo *entryInfo, bool overwriteFile, bool appendFile){
    QFile *fileOperator = new QFile(sourceFilename);
    QFile *destFileOperator = new QFile(targetFilename);
    QFileInfo *destEntryInfo = new QFileInfo(targetFilename);
    QDir targetDir(destEntryInfo->path());
    if(!targetDir.exists())
        targetDir.mkpath(destEntryInfo->path());
    if(appendFile&&destFileOperator->exists()){
        if(destFileOperator->open(QIODevice::WriteOnly|QIODevice::Append|QIODevice::Text)&&fileOperator->open(QIODevice::ReadOnly)){
            if(destFileOperator->write(fileOperator->readAll())==-1){
                updateProgressText(tr("Failure upon installing payload: Could not append to file"));
                return 1;
            }else{
                logFileAction(destEntryInfo,FILE_APPEND);
                destFileOperator->close();
            }
        }else{
            updateProgressText(tr("Failure upon installing payload: Could not open file to append"));
            return 1;
        }
        return 0;
    }
    if(overwriteFile&&destFileOperator->exists()){
        if(!destFileOperator->remove()){
            updateProgressText(tr("Failure upon installing payload: Could not remove existing file"));
            return 1;
        }else
            logFileAction(destEntryInfo,FILE_REMOVE);
    }
    if(!fileOperator->copy(targetFile)){
        updateProgressText(tr("Failure upon installing payload: Will not overwrite existing file."));
        return 1;
    }else
        logFileAction(destEntryInfo,FILE_COPY);
    delete fileOperator;
    delete destFileOperator;
    delete destEntryInfo;
    return 0;
}

int FilesystemActions::installPayload(){
    if((sourceFile.isEmpty())||(targetFile.isEmpty())){
        updateProgressText(tr("Failed to install payload; no filenames provided."));
        return 1;
    }
    QFileInfo sourceFileInfo(sourceFile);
    if(sourceFileInfo.isFile()){
        QFileInfo *entryInfo = new QFileInfo(sourceFile);
        applyPayloadFile(sourceFile,targetFile,entryInfo,overwriteFile,appendFile);
        delete entryInfo;
    }else if(sourceFileInfo.isDir()){
        QDir sourceDir(sourceFile);
        QDir targetDir(targetFile);
        QFileInfo *targetDirInfo = new QFileInfo(targetFile);
        if(!sourceDir.exists()){
            updateProgressText(tr("Payload referenced in JSON does not exist."));
            return 1;
        }
        if(!targetDir.exists())
            if(targetDir.mkpath(targetFile))
                logFileAction(targetDirInfo,DIR_CREATE);
        foreach(QString entry,sourceDir.entryList(QDir::Files|QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot)){
            QString entryFN = sourceFile+QString("/")+entry;
            QString entryDFN = targetFile+QString("/")+entry;
            QFileInfo *entryInfo = new QFileInfo(entryFN);
            QFileInfo *entryDInfo = new QFileInfo(entryDFN);
            if(entryInfo->isDir()){
                if(copyDirectory(entryFN,entryDFN,false,overwriteFile,appendFile)!=0){
                    return 1;
                }else
                    logFileAction(entryDInfo,DIR_COPY);
            }
            if(entryInfo->isFile()){
                applyPayloadFile(entryFN,entryDFN,entryInfo,overwriteFile,appendFile);
            }
            delete entryInfo;
            delete entryDInfo;
        }
        delete targetDirInfo;
    }
}

int FilesystemActions::copyDirectory(QString oldName, QString newName, bool caseSense, bool overwriteFile, bool appendFile){
    QDir sourceDir(oldName);
    QDir targetDir(newName);
    QFileInfo *targetDirInfo = new QFileInfo(newName);
    if(!targetDir.exists())
        if(targetDir.mkdir(newName))
            logFileAction(targetDirInfo,DIR_CREATE);
    foreach(QString entry,sourceDir.entryList(QDir::Files|QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot)){
        QString entryFN = oldName+QString("/")+entry;
        QString entryDFN = newName+QString("/")+entry;
        QFileInfo *entryInfo = new QFileInfo(entryFN);
        QFileInfo *entryDInfo = new QFileInfo(entryDFN);
        if(entryInfo->isDir()){
            if(copyDirectory(entryFN,entryDFN,caseSense,overwriteFile,appendFile)!=0){
                updateProgressText(tr("Failure upon installing payload: Failed to copy directory."));
                return 1;
            }else
                logFileAction(entryDInfo,DIR_COPY);
        }
        if(entryInfo->isFile()){
            applyPayloadFile(entryFN,entryDFN,entryInfo,overwriteFile,appendFile);
        }
        delete entryInfo;
        delete entryDInfo;
    }
    delete targetDirInfo;
    return 0;
}
