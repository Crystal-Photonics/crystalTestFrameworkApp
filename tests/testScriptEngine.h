#ifndef TESTSCRIPTENGINE_H
#define TESTSCRIPTENGINE_H

#include "autotest.h"
#include "scriptengine.h"
#include <QObject>

class FooFooClass {
  virtual void PenUp() = 0;
  virtual void PenDown() = 0;
};

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
    void testPythonStdErrToFile();

    void testPythonArgv();

private slots:
    void initTestCase();
    void testPythonUnittest();
};


DECLARE_TEST(TestScriptEngine)


#endif
