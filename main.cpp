#include "kaidan.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser cParse;
    cParse.setApplicationDescription("Jason launcher");
    cParse.addHelpOption();
    cParse.addPositionalArgument("file",QApplication::translate("init","File to open"));
    QCommandLineOption desktopAction = QCommandLineOption("action",QApplication::translate("init","Action within the Jason document to launch"),"action","");
    QCommandLineOption desktopGen = QCommandLineOption("desktop-file-generate",QApplication::translate("init","Create a desktop file"),"desktop-file-generate","");
    desktopAction.setValueName("action");
    desktopGen.setValueName(QApplication::translate("init","output .desktop file"));
    cParse.addOption(desktopAction);
    cParse.addOption(desktopGen);
    //Actual processing
    cParse.process(a);

    Kaidan w;
    //Required arguments
    QStringList posArgs = cParse.positionalArguments();
    QString filename;
    if (posArgs.length()>=1)
        filename = posArgs[0];
    w.initializeParse(filename);

    return a.exec();
}
