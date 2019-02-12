#include "testScriptEngine.h"
#include "LuaUI/lineedit.h"
#include "gmock/gmock.h" // Brings in Google Mock.
#include "lua_functions.h"
#include "sol.hpp"

#define USETESTS 1
TestScriptEngine::TestScriptEngine(QObject *parent)
    : QObject(parent) {}

#define QVERIFY_EXCEPTION_THROWN_error_number(expression, error_number)                                                                                        \
    do {                                                                                                                                                       \
        QT_TRY {                                                                                                                                               \
            QT_TRY {                                                                                                                                           \
                expression;                                                                                                                                    \
                QTest::qFail(                                                                                                                                  \
                    "Expected exception of type DataEngineError"                                                                                               \
                    " to be thrown"                                                                                                                            \
                    " but no exception caught",                                                                                                                \
                    __FILE__, __LINE__);                                                                                                                       \
                return;                                                                                                                                        \
            }                                                                                                                                                  \
            QT_CATCH(const DataEngineError &e) {                                                                                                               \
                QCOMPARE(e.get_error_number(), error_number);                                                                                                  \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        QT_CATCH(const std::exception &e) {                                                                                                                    \
            QByteArray msg = QByteArray() + "Expected exception of type DataEngineError to be thrown but std::exception caught with message: " + e.what();     \
            QTest::qFail(msg.constData(), __FILE__, __LINE__);                                                                                                 \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
        QT_CATCH(...) {                                                                                                                                        \
            QTest::qFail(                                                                                                                                      \
                "Expected exception of type DataEngineError"                                                                                                   \
                " to be thrown"                                                                                                                                \
                " but unknown exception caught",                                                                                                               \
                __FILE__, __LINE__);                                                                                                                           \
            return;                                                                                                                                            \
        }                                                                                                                                                      \
    } while (0)

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
    abs_prefix = QDir().currentPath().split(':')[0] + ':'; //get c:
    abs_prefix = abs_prefix.toLower();
#endif
    QCOMPARE(create_path(QString("/absolute/filename.json")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/"));
    QCOMPARE(create_path(QString("/absolute/filename")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/filename/"));
    QCOMPARE(create_path(QString("/absolute/filename/")).toLower(), QDir::toNativeSeparators(abs_prefix + "/absolute/filename/"));
}

void TestScriptEngine::test_pattern_check() {
#if 1
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("asdasd"), false);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern(""), false);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("18/42"), true);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("17/42"), false);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("33/17"), false);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("1733"), false);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("18/42a"), true);
    QCOMPARE(PatternCheck("YY/WW?").is_input_matching_to_pattern("18/42asdasd"), false);
    QCOMPARE(PatternCheck("").is_input_matching_to_pattern("18/42asdasd"), true);
#endif
    QVERIFY_EXCEPTION_THROWN(PatternCheck("werwerwer").is_input_matching_to_pattern("18/42asdasd"), std::runtime_error);

    QCOMPARE(PatternCheck("YY/WW?-").is_input_matching_to_pattern("-"), true);
    QCOMPARE(PatternCheck("YY/WW?-").is_input_matching_to_pattern("18/42"), true);
    QCOMPARE(PatternCheck("YY/WW?-").is_input_matching_to_pattern("17/42a"), false);
    QCOMPARE(PatternCheck("YY/WW?-").is_input_matching_to_pattern("18/42a"), true);
    QCOMPARE(PatternCheck("YY/WW?-").is_input_matching_to_pattern("18/42-"), false);
}

void TestScriptEngine::test_serachpath() {
    QString git_path = search_in_search_path("", "git");
    // qDebug() << get_search_paths("");
    //  qDebug() << git_path;
    QVERIFY(git_path != "");
}
