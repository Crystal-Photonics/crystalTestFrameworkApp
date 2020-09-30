#include "testdescriptionloader.h"
#include "Windows/mainwindow.h"
#include "Windows/plaintextedit.h"
#include "config.h"
#include "console.h"
#include "scriptengine.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QSettings>
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
    : name(display_name)
    , file_path(file_path) {
    console = Utility::promised_thread_call(MainWindow::mw, [&] {
        auto link_console = std::make_unique<PlainTextEdit>();
        link_console->setReadOnly(true);
        link_console->setMaximumBlockCount(1000);
        if (name.endsWith(".lua")) {
            name.chop(4);
        }
        ui_entry.reset(add_entry(test_list, display_name.split('/')));
        ui_entry->setData(0, Qt::UserRole, QVariant::fromValue(this));
        return link_console;
    });
    reload();
}

TestDescriptionLoader::TestDescriptionLoader(TestDescriptionLoader &&other)
    : console(std::move(other.console))
    , ui_entry(std::move(other.ui_entry))
    , name(std::move(other.name))
    , file_path(std::move(other.file_path))
    , device_requirements(std::move(other.device_requirements)) {
    Utility::promised_thread_call(MainWindow::mw, [this] { ui_entry->setData(0, Qt::UserRole, QVariant::fromValue(this)); });
}

TestDescriptionLoader &TestDescriptionLoader::operator=(TestDescriptionLoader &&other) {
    console = std::move(other.console);
    ui_entry = std::move(other.ui_entry);
    name = std::move(other.name);
    file_path = std::move(other.file_path);
    device_requirements = std::move(other.device_requirements);
    Utility::promised_thread_call(MainWindow::mw, [this] { ui_entry->setData(0, Qt::UserRole, QVariant::fromValue(this)); });
    return *this;
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
    Utility::promised_thread_call(MainWindow::mw, [&] {
        ui_entry->setText(1, "");
        console->clear();
    });
    Console temp_console{console.get()};
    ScriptEngine script{nullptr, temp_console, nullptr, ""};
    bool warning_occured = false;
    bool error_occured = false;
    try {
        for (const auto &message : MainWindow::validate_script(file_path)) {
            QRegExp regex{R"((.*):(\d+):\d+-\d+:(.*))"};
            if (not regex.exactMatch(message)) {
                qDebug() << "Failed parsing message" << message;
                continue;
            }
            auto message_parts = regex.capturedTexts();
            auto path = std::move(message_parts[1]);
            auto line = std::move(message_parts[2]);
            auto diagnostic = std::move(message_parts[3]);
            if (message.contains("(W")) {
                temp_console.warning() << Console_Link{path + ':' + line, name + ':' + line} << ':' << std::move(diagnostic);
                warning_occured = true;
            } else if (message.contains("(E")) {
                temp_console.error() << Console_Link{path + ':' + line, name + ':' + line} << ':' << std::move(diagnostic);
                error_occured = true;
            }
        }
    } catch (const std::exception &e) {
        Console_handle::error(console.get()) << "Failed validating script: " << Sol_error_message{e.what(), file_path, name};
        Utility::promised_thread_call(MainWindow::mw, [&] { ui_entry->setIcon(3, QIcon{"://src/icons/if_exclamation_16.ico"}); });
        return;
    }

    try {
        script.load_script(file_path.toStdString());
        device_requirements.clear();
        device_requirements = script.get_device_requirement_list();

        QStringList reqs;
        for (auto &device_requirement : device_requirements) {
            reqs << device_requirement.get_description();
        }

        //promised_thread_call guards above thread_calls. May be empty, but cannot be removed.
        Utility::promised_thread_call(MainWindow::mw, [&] {
            ui_entry->setText(1, reqs.join(", "));
            if (error_occured) {
                ui_entry->setIcon(3, QIcon{"://src/icons/if_exclamation_16.ico"});
            } else if (warning_occured) {
                ui_entry->setIcon(3, QIcon{"://src/icons/warning-96.png"});
            } else {
                ui_entry->setIcon(3, QIcon{});
            }
        });
    } catch (const std::runtime_error &e) {
        Console_handle::error(console.get()) << "Failed loading protocols: " << Sol_error_message{e.what(), file_path, name};
        Utility::promised_thread_call(MainWindow::mw, [&] { ui_entry->setIcon(3, QIcon{"://src/icons/if_exclamation_16.ico"}); });
    }
}
