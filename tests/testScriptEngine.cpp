#include "testscriptengine.h"
#include "pysys.h"
#include <QString>
#include <QVariant>
#include <QTextStream>
#include "gmock/gmock.h"  // Brings in Google Mock.

#if 1

class MockPySys : public pySys {
    public:
      MOCK_METHOD1(out, void(QVariant text));

};
#endif

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
class Something {};

class MockFoo {
 public:
  // A mock method that returns a user-defined type.  Google Mock
  // doesn't know what the default value for this type is.
  MOCK_METHOD0(GetSomething, Something());
};

void TestScriptEngine::mockTest()
{


    try {

        MockFoo mock;

            EXPECT_CALL(mock, GetSomething())
                  .Times(AtLeast(1));
            mock.GetSomething();
#if 0
            QFAIL("GetSomething()'s return type has no default value, so Google Mock should have thrown.");
#endif

    } catch (testing::internal::GoogleTestFailureException& /* unused */) {
#if 0
        QFAIL("Google Test does not try to catch an exception of type "
                "GoogleTestFailureException, which is used for reporting "
                "a failure to other testing frameworks.  Google Mock should "
                "not throw a GoogleTestFailureException as it will kill the "
                "entire test program instead of just the current TEST.");
#endif
    } catch (const std::exception& ex) {
        //EXPECT_THAT(ex.what(), testing::HasSubstr("has no default value"));
    }
}


void TestScriptEngine::runScriptGettingStarted()
{
#if 1

        try
        {
            MockPySys m_pysys;
            EXPECT_CALL(m_pysys, out(QVariant("Hello World!")))
                  .Times(AtLeast(1));

            ScriptEngine scriptEngine("scripts/");
            scriptEngine.setRTSys(&m_pysys);
            scriptEngine.runScript("Test_sys_out.py");
        }
    #if 1
            catch (testing::internal::GoogleTestFailureException e)
            {
        #if 1
                qWarning() << e.what();
                QFAIL("GMock Expectation mismatch");
        #endif
            }
    #endif
#endif
}

void TestScriptEngine::testPythonStdOutToFile(){
    ScriptEngine scriptEngine("scripts/");
    scriptEngine.runScript("Test_stdout.py");
    QFile file("scripts/Test_stdout_stdout.txt");
    QString line;
    if (file.open(QFile::ReadOnly)) {
        QTextStream ts(&file);
        line = ts.readLine();
        file.close();
    }
    QCOMPARE(line,QString("stdout HelloWorld!") );
}
