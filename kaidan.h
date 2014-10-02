#ifndef KAIDAN_H
#define KAIDAN_H

#include <QDialog>

namespace Ui {
class Kaidan;
}

class Kaidan : public QDialog
{
    Q_OBJECT

public:
    explicit Kaidan(QWidget *parent = 0);
    ~Kaidan();

private:
    Ui::Kaidan *ui;
};

#endif // KAIDAN_H
