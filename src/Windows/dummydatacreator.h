#ifndef DUMMYDATACREATOR_H
#define DUMMYDATACREATOR_H

#include "data_engine/data_engine.h"
#include <QDialog>
#include <QSpinBox>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTreeWidget>
namespace Ui {
    class DummyDataCreator;
}

class NoEditDelegate : public QStyledItemDelegate {
    public:
    NoEditDelegate(QObject *parent = 0)
        : QStyledItemDelegate(parent) {}
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        return 0;
    }
};

class SpinBoxDelegate : public QStyledItemDelegate {
    public:
    SpinBoxDelegate(QObject *parent = 0)
        : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class ComboBoxDelegate : public QStyledItemDelegate {
    public:
    ComboBoxDelegate(QStringList items, QObject *parent = 0)
        : QStyledItemDelegate(parent)
        , my_items{items} {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    private:
    QStringList my_items;
};

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
