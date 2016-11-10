#include "testscriptengine.h"
#include <QString>
#include <QVariant>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include "gmock/gmock.h"  // Brings in Google Mock.

#define USETESTS 1
TestScriptEngine::TestScriptEngine( QObject *parent) : QObject(parent)
{
}

TestScriptEngine::~TestScriptEngine()
{
}


void TestScriptEngine::initTestCase(){
}

void TestScriptEngine::cleanupTestCase(){

}

