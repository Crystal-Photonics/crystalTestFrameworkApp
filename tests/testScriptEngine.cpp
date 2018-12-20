#include "testScriptEngine.h"
#include "gmock/gmock.h" // Brings in Google Mock.
#include "lua_functions.h"
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

#if defined(Q_OS_WIN)
    QCOMPARE(get_absolute_file_path("dfsdfsd", QString("C:\\absolute\\filename.json")), QString("C:\\absolute\\filename.json"));
    QCOMPARE(get_absolute_file_path("C:/home/scripts/script.lua", QString("relative/filename.json")), QString("C:/home/scripts/relative/filename.json"));
#else
    QCOMPARE(get_absolute_file_path("/home/scripts/script.lua", QString("relative/filename.json")), QString("/home/scripts/relative/filename.json"));
#endif
}

void TestScriptEngine::test_create_name_path() {
    QString abs_prefix = "";
#if defined(Q_OS_WIN)
    abs_prefix = "c:";
#endif
    QCOMPARE(create_path(QString("/absolute/filename.json")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/"));
    QCOMPARE(create_path(QString("/absolute/filename")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/filename/"));
    QCOMPARE(create_path(QString("/absolute/filename/")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/filename/"));
}

void TestScriptEngine::test_serachpath() {
    QString git_path = search_in_search_path("", "git");
    qDebug() << get_search_paths("");
    qDebug() << git_path;
    QVERIFY(git_path != "");
}
