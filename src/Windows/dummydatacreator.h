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
    NoEditDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        (void)parent;
        (void)option;
        (void)index;
        return nullptr;
    }
};

class SpinBoxDelegate : public QStyledItemDelegate {
    public:
    SpinBoxDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class ComboBoxDelegate : public QStyledItemDelegate {
    public:
    ComboBoxDelegate(QStringList items, QObject *parent = nullptr)
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
    explicit DummyDataCreator(QWidget *parent = nullptr);
    ~DummyDataCreator();
    bool get_is_valid_data_engine();

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
    bool is_valid_data_engine = true;
};

#endif // DUMMYDATACREATOR_H
