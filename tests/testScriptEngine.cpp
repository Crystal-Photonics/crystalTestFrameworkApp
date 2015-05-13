#include "testscriptengine.h"


void TestScriptEngine::initTestCase(){

}

void TestScriptEngine::cleanupTestCase(){

}

void TestScriptEngine::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}
