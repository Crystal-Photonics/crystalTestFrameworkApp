#include "testdescriptionloader.h"
#include "Windows/mainwindow.h"
#include "console.h"
#include "scriptengine.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

//TODO: find a way to combine these 2 functions
static QTreeWidgetItem *add_entry(QTreeWidgetItem *item, QStringList &list) {
    assert(list.isEmpty() == false);
    for (int i = 0; i < item->childCount(); i++) {
        if (item->child(i)->text(0) == list.front()) {
            list.pop_front();
            return add_entry(item->child(i), list);
        }
    }
    auto child = new QTreeWidgetItem(QStringList{} << list.front());
    list.pop_front();
    item->addChild(child);
    if (list.isEmpty()) {
        return child;
    }
    return add_entry(child, list);
}

static QTreeWidgetItem *add_entry(QTreeWidget *item, QStringList &&list) {
    assert(list.isEmpty() == false);
    for (int i = 0; i < item->topLevelItemCount(); i++) {
        if (item->topLevelItem(i)->text(0) == list.front()) {
            list.pop_front();
            return add_entry(item->topLevelItem(i), list);
        }
    }
    auto child = new QTreeWidgetItem(item, QStringList{} << list.front());
    list.pop_front();
    item->addTopLevelItem(child);
    if (list.isEmpty()) {
        return child;
    }
    return add_entry(child, list);
}

TestDescriptionLoader::TestDescriptionLoader(QTreeWidget *test_list, const QString &file_path, const QString &display_name)
    : console(std::make_unique<QPlainTextEdit>())
    , name(display_name)
    , file_path(file_path) {
    console->setReadOnly(true);
    console->setMaximumBlockCount(1000);
    if (name.endsWith(".lua")) {
        name.chop(4);
    }
    ui_entry.reset(add_entry(test_list, display_name.split('/')));
    ui_entry->setData(0, Qt::UserRole, Utility::make_qvariant(this));
    reload();
}

TestDescriptionLoader::TestDescriptionLoader(TestDescriptionLoader &&other)
    : console(std::move(other.console))
    , ui_entry(std::move(other.ui_entry))
    , name(std::move(other.name))
    , file_path(std::move(other.file_path))
    , device_requirements(std::move(other.device_requirements)) {
    ui_entry->setData(0, Qt::UserRole, Utility::make_qvariant(this));
}

TestDescriptionLoader::~TestDescriptionLoader() {}

const std::vector<DeviceRequirements> &TestDescriptionLoader::get_device_requirements() const {
    return device_requirements;
}

const QString &TestDescriptionLoader::get_name() const {
    return name;
}

const QString &TestDescriptionLoader::get_filepath() const {
    return file_path;
}

void TestDescriptionLoader::reload() {
    load_description();
}

void TestDescriptionLoader::launch_editor() {
    ScriptEngine::launch_editor(file_path);
}

void TestDescriptionLoader::load_description() {
    ui_entry->setText(1, "");
    ScriptEngine script{nullptr, nullptr, nullptr, nullptr};
    try {
        //   qDebug() << "load_description  start";
        script.load_script(file_path.toStdString());
#if 1
        device_requirements.clear();
        device_requirements = script.get_device_requirement_list("device_requirements");

        QStringList reqs;
        for (auto &device_requirement : device_requirements) {
            reqs << device_requirement.get_description();
        }

        ui_entry->setText(1, reqs.join(", "));
        ui_entry->setIcon(3, QIcon{});
#endif
        //   qDebug() << "load_description  ende";
    } catch (const std::runtime_error &e) {
        ui_entry->setIcon(3, QIcon{"://src/icons/if_exclamation_16.ico"});
        Console::error(console) << "Failed loading protocols: " << e.what();
        //    qDebug() << "load_description  exception";
    }
}
