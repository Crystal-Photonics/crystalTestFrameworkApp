#ifndef DUMMYDATACREATOR_H
#define DUMMYDATACREATOR_H

#include "data_engine/data_engine.h"
#include <QDialog>
#include <QSpinBox>
#include <QTreeWidget>

namespace Ui {
    class DummyDataCreator;
}

class DummyDataCreator : public QDialog {
    Q_OBJECT

    public:
    explicit DummyDataCreator(QWidget *parent = 0);
    ~DummyDataCreator();

    private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_btn_image_header_clicked();

    void on_btn_image_footer_clicked();

    private:
    void load_gui_from_json();
    void save_gui_to_json();
    QString template_config_filename{};
    Ui::DummyDataCreator *ui;
    Data_engine data_engine;
    QList<QSpinBox *> spinboxes;
    QStringList instance_names;
};

#endif // DUMMYDATACREATOR_H
