#include "testgooglemock.h"

class MockFooClass : public FooClass {
 public:

  MOCK_METHOD0(PenUp, void());
  MOCK_METHOD0(PenDown, void());
};

void FooClass::PenUp(){

}

void FooClass::PenDown(){

}

void TestGoogleMock::basicMocking(){
    MockFooClass turtle;
    EXPECT_CALL(turtle, PenDown())
            .Times(AtLeast(1));

    turtle.PenDown();
}

#if 1
void TestGoogleMock::catchMockException()
{

#if 1
    try {

        MockFooClass turtle;

            EXPECT_CALL(turtle, PenUp())
                  .Times(AtLeast(1));
           // turtle.GetSomething(2);
#if 0
            QFAIL("GetSomething()'s return type has no default value, so Google Mock should have thrown.");
#endif

    } catch (testing::internal::GoogleTestFailureException& /* unused */) {
#if 1
        QFAIL("Google Test does not try to catch an exception of type "
                "GoogleTestFailureException, which is used for reporting "
                "a failure to other testing frameworks.  Google Mock should "
                "not throw a GoogleTestFailureException as it will kill the "
                "entire test program instead of just the current TEST.");
#endif
    } catch (const std::exception& ex) {
        EXPECT_THAT(ex.what(), testing::HasSubstr("has no default value"));
    }
    #endif
}

#endif
