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


private slots:
    void initTestCase();
    void testPythonUnittest();
    void runScriptGettingStarted();
    void getFilesInDirectory();

    void testPythonStdOutToFile();
    void testPythonStdErrToFile();

    void testPythonArgv();
};


DECLARE_TEST(TestScriptEngine)


#endif
