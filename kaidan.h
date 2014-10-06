#ifndef KAIDAN_H
#define KAIDAN_H

#include <QDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QGridLayout>
#include <QPixmap>
#include <QImageReader>
#include <QFile>
#include <QFileDialog>

namespace Ui {
class Kaidan;
}

class Kaidan : public QDialog
{
    Q_OBJECT

public:
    explicit Kaidan(QWidget *parent = 0);
    ~Kaidan();

    void initializeParse(QString startDocument);

private slots:

private:
    Ui::Kaidan *ui;


    //displayLogs
    QDialog outputWindow;
    QGridLayout *outputLayout;
    QTextEdit *errEdit,*outEdit;

    //displayMessage
    QMessageBox *messageBox;

    //updateProgressIcon
    QFile *imageFile;
    QPixmap *newLabelIcon;

public slots:
    //Interface modificators
    void updateProgressText(QString newText);
    void updateProgressIcon(QString newIconPath);
    void updateWindowTitle(QString newTitle);
    void updateMainProgressBarLimits(qint64 min,qint64 max);
    void updateMainProgressBarValue(int val);
    void modifySecondPBarVisibility(bool doShow);
    void updateSecondProgressBarLimits(qint64 min,qint64 max);
    void updateSecondaryProgressBarValue(int val);
    void setIconLabelSize(int size);

    //Receivers
    void displayMessage(int status,QString message);
    void displayLogs(QString stdOut,QString stdErr);

    //Core functionality
    void handleFailed();
    void handleFinished();

};

#endif // KAIDAN_H
