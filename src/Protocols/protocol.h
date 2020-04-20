#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QString>

struct Protocol {
    QString type;
    Protocol(QString type)
        : type(type) {}
    bool operator==(const QString &&type) const {
        return this->type == type;
    }
    virtual QString get_manual() const {
        return {};
    }
    virtual ~Protocol() = default;
};

#endif // PROTOCOL_H
