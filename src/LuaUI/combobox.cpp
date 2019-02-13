#include "combobox.h"
#include "Windows/mainwindow.h"
#include "ui_container.h"
#include <QComboBox>
#include <QInputDialog>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
ComboBox::ComboBox(UI_container *parent, const sol::table &items)
    : UI_widget{parent}
    , combobox(new QComboBox(parent))
    , label(new QLabel(parent)) {
    QVBoxLayout *layout = new QVBoxLayout;
    label->setText(" ");
    QStringList sl;
    for (const auto &item : items) {
        sl.append(QString::fromStdString(item.second.as<std::string>()));
    }

    layout->addWidget(label);
    layout->addWidget(combobox);
    layout->addStretch(1);
    combobox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    parent->add(layout, this);
    //set_items(items);
    combobox->addItems(sl);
    parent->scroll_to_bottom();
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

ComboBox::~ComboBox() {
    combobox->setEnabled(false);
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
}

///\endcond
void ComboBox::set_items(const sol::table &items) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    combobox->clear();
    for (const auto &item : items) {
        combobox->addItem(QString::fromStdString(item.second.as<std::string>()));
    }
}

std::string ComboBox::get_text() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return combobox->currentText().toStdString();
}

void ComboBox::set_index(unsigned int index) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    if ((0 == index) && (index > (unsigned int)combobox->count())) {
        throw std::runtime_error(QObject::tr("ComboBox:set_index() Index out of range. setting to: %1 but only 1 <= index <= %2 is allowed")
                                     .arg(index)
                                     .arg(combobox->count())
                                     .toStdString());
    } else {
        combobox->setCurrentIndex(index - 1);
    }
}

unsigned int ComboBox::get_index() {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    //return == 0 means: "no item is selected"
    return combobox->currentIndex() + 1;
}

void ComboBox::set_visible(bool visible) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    combobox->setVisible(visible);
    label->setVisible(visible);
}

void ComboBox::set_caption(const std::string caption) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    label->setText(QString::fromStdString(caption));
    if (caption == "") {
        label->setText(" ");
    }
    //  label->setVisible(label->text().size());
}

void ComboBox::set_name(const std::string name) {
    name_m = QString::fromStdString(name);
}

std::string ComboBox::get_caption() const {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    return label->text().toStdString();
}

void ComboBox::set_editable(bool editable) {
    assert(MainWindow::gui_thread == QThread::currentThread()); //event_queue_run_ must not be started by the GUI-thread because it would freeze the GUI
    combobox->setEditable(editable);
}
