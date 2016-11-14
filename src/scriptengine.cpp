#include "scriptengine.h"
#include "sol.hpp"

#include <QDebug>
#include <QDir>
#include <QMessageBox>

ScriptEngine::ScriptEngine(QObject *parent)
	: QObject(parent) {}

ScriptEngine::~ScriptEngine() {}
