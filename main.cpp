#include "kaidan.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Kaidan w;
    w.show();

    return a.exec();
}
