#include "testScriptEngine.h"
#include "gmock/gmock.h"  // Brings in Google Mock.
#include "sol.hpp"

#define USETESTS 1
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

void TestScriptEngine::basicLuaTest()
{
	sol::state lua;
	int x = 0;
	lua.set_function("beep", [&x]{ ++x; });
	lua.script("beep()");
	ASSERT_EQ(x, 1);

	sol::function beep = lua["beep"];
	beep();
	ASSERT_EQ(x, 2);
}
