#ifndef TESTDESCRIPTIONLOADER_H
#define TESTDESCRIPTIONLOADER_H

#include "scriptengine.h"

#include <QString>
#include <memory>
#include <vector>

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;

class TestDescriptionLoader {
	public:
	TestDescriptionLoader(QTreeWidget *test_list, const QString &file_path, const QString &display_name);
	TestDescriptionLoader(TestDescriptionLoader &&other);
	TestDescriptionLoader &operator=(TestDescriptionLoader &&other);
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

Q_DECLARE_METATYPE(TestDescriptionLoader *);

#endif // TESTDESCRIPTIONLOADER_H
