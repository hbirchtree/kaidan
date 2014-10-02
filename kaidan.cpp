#include "kaidan.h"
#include "ui_kaidan.h"

Kaidan::Kaidan(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Kaidan)
{
    ui->setupUi(this);
}

Kaidan::~Kaidan()
{
    delete ui;
}
