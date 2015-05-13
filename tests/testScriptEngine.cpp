#include "testscriptengine.h"
#include <QString>

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

void TestScriptEngine::getFilesInDirectory()
{
   ScriptEngine scriptEngine("scripts/directoryTest");
   QList<QString> filelist = scriptEngine.getFilesInDirectory();
   QCOMPARE(filelist.count(),4);
   QVERIFY2(filelist.indexOf("GettingStarted_1.py") > -1, "GettingStarted_1.py not found in List");
}
