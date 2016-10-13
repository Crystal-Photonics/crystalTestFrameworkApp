#ifndef TESTGOOGLEMOCK_H
#define TESTGOOGLEMOCK_H
#include <QObject>
#include "autotest.h"

class FooClass {
  virtual void PenUp() = 0;
  virtual void PenDown() = 0;

};

class TestGoogleMock : public QObject
{
    Q_OBJECT
public:
private slots:
    void basicMocking();
    //void catchMockException();
};

DECLARE_TEST(TestGoogleMock)

#endif // TESTGOOGLEMOCK_H
