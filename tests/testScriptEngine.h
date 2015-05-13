#ifndef TESTSCRIPTENGINE_H
#define TESTSCRIPTENGINE_H

#include "autotest.h"
#include "scriptengine.h"

class TestScriptEngine: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void toUpper();
};
DECLARE_TEST(TestScriptEngine)

#endif
