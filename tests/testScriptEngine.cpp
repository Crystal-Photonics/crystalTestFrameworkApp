#include "testscriptengine.h"
#include "pysys.h"
#include <QString>
#include <QVariant>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include "gmock/gmock.h"  // Brings in Google Mock.
//#include "gtest/gtest.h"
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
        #if 1
    ScriptEngine scriptEngine("scripts/");
    scriptEngine.runScript("Test_stdout.py");
    QFile file("scripts/Test_stdout_stdout.txt");
    QString line;
    if (file.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        line = ts.readLine();
        file.close();
    }
    QCOMPARE(line,QString("stdout HelloWorld!") );
#endif
}

void TestScriptEngine::testPythonStdErrToFile(){
    #if 1
    ScriptEngine scriptEngine("scripts/");
    scriptEngine.runScript("Test_stderr.py");
    QFile file("scripts/Test_stderr_stderr.txt");
    QString line;
    if (file.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        while(!ts.atEnd()){
            line = ts.readLine();
            if (line.contains("SyntaxError:"))
                break;
        }

        file.close();
    }

    QCOMPARE(line,QString("SyntaxError: invalid syntax") );
#endif
}


void TestScriptEngine::testPythonUnittest(){
#if 1
    ScriptEngine scriptEngine("scripts/");
    scriptEngine.runScript("Test_UnitTest.py");
	QFile fi("scripts/Test_UnitTest_stderr.txt");
	fi.open(fi.ReadOnly);
	//QCOMPARE(fi.readAll(), QByteArray());
#endif
}

void TestScriptEngine::testPythonArgv(){
   #if 0
    QString scriptDir = "scripts/";
    QString scriptFileName = "Test_Argv.py";
    ScriptEngine scriptEngine(scriptDir);
    scriptEngine.runScript(scriptFileName);

    QFileInfo fi("scripts/Test_Argv_stderr.txt");
    QCOMPARE(fi.size(),0 );

    QDir dir(scriptDir);
    QString fullScriptFileName = dir.absoluteFilePath(scriptFileName);
    QFile file("scripts/Test_Argv_stdout.txt");
    QString line;
    if (file.open(QFile::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&file);
        line = ts.readLine();
        file.close();
    }
    QCOMPARE(line,"[\'"+fullScriptFileName+"\']");
#endif
}
