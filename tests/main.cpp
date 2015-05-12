#include <mainwindow.h>

#include <QString>
#include <QtTest>

class TestClass : public QObject
{
    Q_OBJECT

public:
    TestClass();

private Q_SLOTS:
    void twoPlusTwoEqualsFour();
    void threePlusThreeEqualsSix();
};

TestClass::TestClass()
{
}

void TestClass::twoPlusTwoEqualsFour()
{

}

void TestClass::threePlusThreeEqualsSix()
{

}

QTEST_APPLESS_MAIN(TestClass)

#include "main.moc"
