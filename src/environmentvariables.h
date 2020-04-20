#ifndef ENVIRONMENTVARIABLES_H
#define ENVIRONMENTVARIABLES_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <sol.hpp>

class EnvironmentVariables {
    public:
    EnvironmentVariables(QString filename);
    void load_to_lua(sol::state *lua);

    private:
	QMap<QString, QVariant> variables;
};

#endif // ENVIRONMENTVARIABLES_H
