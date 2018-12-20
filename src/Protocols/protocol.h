#ifndef PROTOCOL_H
#define PROTOCOL_H

//this class should not exist, it is just a worse std::variant, but unfortunately it is 2016

#include <QString>

struct Protocol {
    QString type;
    Protocol(QString type)
        : type(type) {}
    bool operator==(const QString &&type) const {
		return this->type == type;
    }
    virtual ~Protocol() = default;
};

#endif // PROTOCOL_H
