#include <QCommandLineOption>
#include <QCommandLineParser>
#include "kaidan.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Jason");
    QApplication::setApplicationVersion("1.0");

    //Parse the command line
    QCommandLineParser cParse;
    cParse.setApplicationDescription("Jason launcher");
    cParse.addHelpOption();
    cParse.addPositionalArgument("file",QApplication::translate("init","File to open"));
    //Actual processing
    cParse.process(a);

    //Required arguments
    QStringList posArgs = cParse.positionalArguments();
    QString filename;
    if (posArgs.length()>=1){
        filename = posArgs[0];
    }else
        return 0;

    //Optional arguments
    QString actionToLaunch;
    QString desktopFile;

    Kaidan jasongui;
    jasongui.initializeParse(filename,actionToLaunch,desktopFile,a.arguments()[0]);

    return 0;
}
