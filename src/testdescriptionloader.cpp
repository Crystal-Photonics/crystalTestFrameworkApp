#include "testdescriptionloader.h"
#include "console.h"
#include "mainwindow.h"
#include "scriptengine.h"

#include <QDebug>
#include <QPlainTextEdit>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

TestDescriptionLoader::TestDescriptionLoader(QTreeWidget *test_list, const QString &file_path)
	: console(std::make_unique<QPlainTextEdit>())
	, file_path(file_path) {
	console->setReadOnly(true);
	name = QString{file_path.data() + file_path.lastIndexOf('/') + 1};
	if (name.endsWith(".lua")) {
		name.chop(4);
	}
	ui_entry = std::make_unique<QTreeWidgetItem>(test_list, QStringList{} << name);
	ui_entry->setData(0, Qt::UserRole, Utility::make_qvariant(this));
	test_list->addTopLevelItem(ui_entry.get());
	reload();
}

TestDescriptionLoader::TestDescriptionLoader(TestDescriptionLoader &&other)
	: console(std::move(other.console))
	, ui_entry(std::move(other.ui_entry))
	, name(std::move(other.name))
	, file_path(std::move(other.file_path))
	, protocols(std::move(other.protocols)) {
	ui_entry->setData(0, Qt::UserRole, Utility::make_qvariant(this));
}

TestDescriptionLoader::~TestDescriptionLoader() {}

const std::vector<QString> &TestDescriptionLoader::get_protocols() const {
	return protocols;
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
	try {
		ScriptEngine script{nullptr};
		script.load_script(file_path);
		auto prots = script.get_string_list("protocols");
		protocols.clear();
		std::copy(prots.begin(), prots.end(), std::back_inserter(protocols));
		ui_entry->setText(1, prots.join(", "));
	} catch (const std::runtime_error &e) {
		Console::error(console) << "Failed loading protocols: " << e.what();
	}
}
