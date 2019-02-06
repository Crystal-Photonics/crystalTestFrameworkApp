#ifndef COMMUNICATION_LOGGER_H
#define COMMUNICATION_LOGGER_H

struct MatchedDevice;
struct QPlainTextEdit;

#include <QObject>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Communication_logger : QObject {
	Q_OBJECT
	public:
	Communication_logger(QPlainTextEdit *console);
	~Communication_logger();
	void add(const MatchedDevice &device);
	void set_log_file(const std::string &filepath);

	private:
	std::ostringstream log;
	std::ofstream logfile;
	std::ostream *log_target = &log;
	std::vector<QMetaObject::Connection> connections;
	int device_count = 1;
};

#endif // COMMUNICATION_LOGGER_H
