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
	void reload();
	void launch_editor();

	private:
	void load_description();
	QString name;
	QString file_path;
	std::vector<QString> protocols;
};

#if 0
struct Test {
	Test(QTreeWidget *test_list, const QString &file_path);
	~Test();
	Test(const Test &) = delete;
	Test(Test &&other);
	Test &operator=(const Test &) = delete;
	Test &operator=(Test &&other);
	void swap(Test &other);
	void reset_ui();

	QTreeWidget *parent = nullptr;
	QTreeWidgetItem *ui_item = nullptr;
	QPlainTextEdit *console = nullptr;
	QSplitter *test_console_widget = nullptr;
	ScriptEngine script;
	std::vector<QString> protocols;
	QString name;
	QString file_path;
	bool operator==(QTreeWidgetItem *item);
	std::unique_ptr<Worker> worker;
	std::unique_ptr<QThread> worker_thread;
};
#endif

#endif // TESTDESCRIPTIONLOADER_H
