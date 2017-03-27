#ifndef TESTDESCRIPTIONLOADER_H
#define TESTDESCRIPTIONLOADER_H

#include <QString>
#include <memory>
#include <vector>
#include "scriptengine.h"

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;

class TestDescriptionLoader {
	public:
	TestDescriptionLoader(QTreeWidget *test_list, const QString &file_path, const QString &display_name);
	TestDescriptionLoader(TestDescriptionLoader &&other);
	~TestDescriptionLoader();

	const std::vector<DeviceRequirements> &get_device_requirements() const;
	const QString &get_name() const;
	const QString &get_filepath() const;
	void reload();
	void launch_editor();

	std::unique_ptr<QPlainTextEdit> console;
	std::unique_ptr<QTreeWidgetItem> ui_entry;

	private:
	void load_description();
	QString name;
	QString file_path;
    std::vector<DeviceRequirements> device_requirements;
};

#endif // TESTDESCRIPTIONLOADER_H
