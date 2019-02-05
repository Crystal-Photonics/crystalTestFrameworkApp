#ifndef COMMUNICATION_LOGGER_H
#define COMMUNICATION_LOGGER_H

struct MatchedDevice;
struct QPlainTextEdit;

#include <QObject>
#include <fstream>
#include <string>
#include <vector>

class Communication_logger : QObject {
	Q_OBJECT
	public:
	Communication_logger(QPlainTextEdit *console);
	~Communication_logger();
	void set_file_path(const std::string &file_path);
	void add(MatchedDevice &device);

	private:
	std::ofstream file;
	std::vector<QMetaObject::Connection> connections;
	int device_count = 1;
};

#endif // COMMUNICATION_LOGGER_H
