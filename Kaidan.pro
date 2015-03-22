#-------------------------------------------------
#
# Project created by QtCreator 2014-10-02T20:41:57
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Kaidan
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    desktoptools.cpp \
    executer.cpp \
    jasonparser.cpp \
    jsonparser.cpp \
    jsonstaticfuncs.cpp \
    kaidan.cpp \
    main.cpp \
    kaidan-tools/downloader.cpp \
    kaidan-tools/filesystemactions.cpp \
    kaidan-tools/kaidanprocedure.cpp \
    modules/executionunit.cpp \
    modules/variablehandler.cpp \
    modules/environmentcontainer.cpp \
    modules/systemcontainer.cpp

HEADERS  += \
    desktoptools.h \
    executer.h \
    jasonparser.h \
    jsonparser.h \
    jsonstaticfuncs.h \
    kaidan.h \
    kaidan-tools/downloader.h \
    kaidan-tools/filesystemactions.h \
    kaidan-tools/kaidanprocedure.h \
    modules/executionunit.h \
    modules/uiglue.h \
    modules/variablehandler.h \
    kaidan-tools/kaidanstep.h \
    modules/environmentcontainer.h \
    modules/systemcontainer.h

FORMS    += kaidan.ui
