#include "environmentvariables.h"
#include "Windows/mainwindow.h"
#include "qt_util.h"
#include "util.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QStringList>

EnvironmentVariables::EnvironmentVariables(QString filename) {
    if (filename == "") {
        return;
    }
    QFile file;

	if (filename == "") {
		Utility::thread_call(MainWindow::mw, [filename] {
			QMessageBox::warning(MainWindow::mw, "Can't open environment file", "Can't open environment file. Filename is empty.");
		});

		return;
    }
	if (!QFile::exists(filename)) {
		Utility::thread_call(MainWindow::mw, [filename] {
			QMessageBox::warning(MainWindow::mw, "Can't open environment file", "Can't open environment file. File does not exist: " + filename);
		});

		return;
    }

    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		Utility::thread_call(MainWindow::mw,
							 [filename] { QMessageBox::warning(MainWindow::mw, "Can't open environment file", "Can't open environment file: " + filename); });

        return;
    }
    QString json_string = file.readAll();
    file.close();
    QJsonDocument j_doc = QJsonDocument::fromJson(json_string.toUtf8());
    if (j_doc.isNull()) {
		Utility::thread_call(MainWindow::mw, [filename] {
			QMessageBox::warning(MainWindow::mw, "could not parse file with environment variables",
								 "could not parse file with environment variables. Seems the json is broken: " + filename);
		});
        return;
    }
    QJsonObject j_obj = j_doc.object();
    const auto keys = j_obj.keys();

    for (auto key : keys) {
        auto value = j_obj[key];
        if (value.isDouble()) {
            QVariant var{value.toDouble()};
            variables.insert(key, var);
        } else if (value.isString()) {
            QVariant var{value.toString()};
            variables.insert(key, var);
        } else if (value.isBool()) {
            QVariant var{value.toBool()};
            variables.insert(key, var);
        } else {
            assert(0);
        }
    }
}

void EnvironmentVariables::load_to_lua(sol::state *lua) {
    for (auto key : variables.keys()) {
        if (variables[key].type() == QVariant::Bool) {
            (*lua)[key.toStdString()] = variables[key].toBool();
        } else if (variables[key].type() == QVariant::Double) {
            (*lua)[key.toStdString()] = variables[key].toDouble();
        } else if (variables[key].type() == QVariant::String) {
            (*lua)[key.toStdString()] = variables[key].toString().toStdString();
        } else {
            assert(0);
        }
    }
}
