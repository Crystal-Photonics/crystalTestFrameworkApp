#include "testScriptEngine.h"
#include "lua_functions.h"
#include "gmock/gmock.h" // Brings in Google Mock.
#include "sol.hpp"

#define USETESTS 1
TestScriptEngine::TestScriptEngine(QObject *parent)
    : QObject(parent) {}

TestScriptEngine::~TestScriptEngine() {}

void TestScriptEngine::initTestCase() {}

void TestScriptEngine::cleanupTestCase() {}

void TestScriptEngine::basicLuaTest() {
    sol::state lua;
    int x = 0;
    lua.set_function("beep", [&x] { ++x; });
    lua.script("beep()");
    ASSERT_EQ(x, 1);

    sol::function beep = lua["beep"];
    beep();
    ASSERT_EQ(x, 2);
}

void TestScriptEngine::test_file_name_path() {
    QCOMPARE(get_absolute_file_path("dfsdfsd", QString("/absolute/filename.json")), QString("/absolute/filename.json"));
    QCOMPARE(get_absolute_file_path("dfsdfsd", QString("C:\\absolute\\filename.json")), QString("C:\\absolute\\filename.json"));
    QCOMPARE(get_absolute_file_path("C:/home/scripts/script.lua", QString("relative/filename.json")), QString("C:/home/scripts/relative/filename.json"));
}

void TestScriptEngine::test_create_name_path() {
    QCOMPARE(create_path(QString("/absolute/filename.json")).toLower(), QDir::toNativeSeparators("c:/absolute/"));
    QCOMPARE(create_path(QString("/absolute/filename")).toLower(), QDir::toNativeSeparators("c:/absolute/filename/"));
    QCOMPARE(create_path(QString("/absolute/filename/")).toLower(), QDir::toNativeSeparators("c:/absolute/filename/"));

}
