#include "combobox.h"
#include "ui_container.h"

#include <QComboBox>
#include <QInputDialog>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

///\cond HIDDEN_SYMBOLS
ComboBox::ComboBox(UI_container *parent)
	: combobox(new QComboBox(parent))
	, label(new QLabel(parent)) {
	QVBoxLayout *layout = new QVBoxLayout;
    label->setVisible(false);
    layout->addWidget(label);
    layout->addWidget(combobox);
	combobox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
	parent->add(layout);
}

ComboBox::~ComboBox() {
	combobox->setEnabled(false);
}
///\endcond
void ComboBox::set_items(sol::table items) {
    combobox->clear();
    for (auto &item : items) {
        combobox->addItem(QString::fromStdString(item.second.as<std::string>()));
    }
}

std::string ComboBox::get_text() const {
    return combobox->currentText().toStdString();
}

void ComboBox::set_index(unsigned int index) {
    if ((0 == index) && (index > combobox->count())) {
        //TODO error
    } else {
        combobox->setCurrentIndex(index - 1);
    }
}

unsigned int ComboBox::get_index() {
    return combobox->currentIndex();
}

void ComboBox::set_caption(const std::string caption) {
    label->setText(QString::fromStdString(caption));
    label->setVisible(label->text().size());
}

std::string ComboBox::get_caption() const {
    label->text().toStdString();
}
