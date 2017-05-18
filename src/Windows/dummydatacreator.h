#ifndef DUMMYDATACREATOR_H
#define DUMMYDATACREATOR_H

#include <QDialog>
#include "data_engine/data_engine.h"
#include <QSpinBox>

namespace Ui {
class DummyDataCreator;
}

class DummyDataCreator : public QDialog
{
    Q_OBJECT

public:
    explicit DummyDataCreator(QWidget *parent = 0);
    ~DummyDataCreator();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::DummyDataCreator *ui;
    Data_engine data_engine;
    QList<QSpinBox*> spinboxes;
    QStringList instance_names;
};

#endif // DUMMYDATACREATOR_H
