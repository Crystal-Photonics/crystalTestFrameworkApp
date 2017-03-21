#ifndef PROTOCOL_H
#define PROTOCOL_H

//this class should not exist, it is just a worse std::variant, but unfortunately it is 2016

#include <QString>

struct Protocol {
	QString type;
    QString device_name;
    Protocol(QString type)
        : type(type) {}
	bool operator==(const QString &&type) const {
        return (this->type == type) && (this->device_name == device_name);
	}
	virtual ~Protocol() = default;
};

#endif // PROTOCOL_H
