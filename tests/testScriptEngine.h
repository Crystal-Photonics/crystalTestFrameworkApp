#ifndef TESTSCRIPTENGINE_H
#define TESTSCRIPTENGINE_H

#include "autotest.h"
#include "scriptengine.h"
#include <QObject>

class TestScriptEngine : public QObject {
    Q_OBJECT
    public:
    explicit TestScriptEngine(QObject *parent = 0);
    ~TestScriptEngine();

    private:
    void initTestCase();
    void cleanupTestCase();
    private slots:
    void basicLuaTest();
    void test_file_name_path();
    void test_create_name_path();
    void test_serachpath();
};

DECLARE_TEST(TestScriptEngine)

#endif
