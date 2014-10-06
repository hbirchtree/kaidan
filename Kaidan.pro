#-------------------------------------------------
#
# Project created by QtCreator 2014-10-02T20:41:57
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Kaidan
TEMPLATE = app


SOURCES += main.cpp\
        kaidan.cpp \
    jasonparser.cpp \
    downloader.cpp

HEADERS  += kaidan.h \
    jasonparser.h \
    downloader.h

FORMS    += kaidan.ui
