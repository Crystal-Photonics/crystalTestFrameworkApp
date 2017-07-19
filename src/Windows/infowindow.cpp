#include "infowindow.h"
#include "ui_infowindow.h"
#include "vc.h"
InfoWindow::InfoWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InfoWindow)
{
    ui->setupUi(this);
    ui->lbl_git_date->setText("Git Date: "+QString(GITDATE));
    ui->lbl_git_hash->setText("Git Hash: 0x"+QString::number(GITHASH,16));
    ui->lbl_build_date->setText(QString("Build date: ")+QString(__DATE__) +" "+ QString(__TIME__));
}

InfoWindow::~InfoWindow()
{
    delete ui;
}
