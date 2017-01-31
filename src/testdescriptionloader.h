#ifndef TESTDESCRIPTIONLOADER_H
#define TESTDESCRIPTIONLOADER_H

#include <QString>
#include <memory>
#include <vector>

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;

class TestDescriptionLoader {
	public:
	TestDescriptionLoader(QTreeWidget *test_list, const QString &file_path);
	TestDescriptionLoader(TestDescriptionLoader &&other);
	~TestDescriptionLoader();
	const std::vector<QString> &get_protocols() const;
	std::unique_ptr<QPlainTextEdit> console;
	std::unique_ptr<QTreeWidgetItem> ui_entry;
	const QString &get_name() const;
	const QString &get_filepath() const;
	void reload();
	void launch_editor();

	private:
	void load_description();
	QString name;
	QString file_path;
	std::vector<QString> protocols;
};

#endif // TESTDESCRIPTIONLOADER_H
