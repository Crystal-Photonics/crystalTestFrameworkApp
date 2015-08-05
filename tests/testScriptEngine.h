#ifndef TESTSCRIPTENGINE_H
#define TESTSCRIPTENGINE_H

#include "autotest.h"
#include "scriptengine.h"
#include <QObject>

class TestScriptEngine: public QObject
{
    Q_OBJECT
public:
    explicit TestScriptEngine( QObject *parent = 0);
    ~TestScriptEngine();

private:

    void cleanupTestCase();
    void getFilesInDirectory();
    void runScriptGettingStarted();
    void testPythonStdOutToFile();
    void mockTest();
    void testPythonStdErrToFile();

    void testPythonArgv();

private slots:
    void initTestCase();
    void testPythonUnittest();
};
DECLARE_TEST(TestScriptEngine)

#endif
