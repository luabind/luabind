#include "test.h"

namespace
{
	int feedback = 0;
	int feedback2 = 0;

	struct A
	{
		A(int a) { feedback = a; }
		A(const A&) { feedback = 21; }
		A() { feedback = 20; }
		~A() { feedback = -1; }

	};

	void f(A*, const A&) {}

	struct B: public A
	{
		B(int a):A(a) { feedback2 = a; }
		B() { feedback2 = 20; }
		~B() { feedback2 = -1; }
	};

} // anonymous namespace

bool test_construction()
{
	using namespace luabind;

	lua_State* L = lua_open();
	lua_closer c(L);
	int top = lua_gettop(L);

	open(L);

	class_<A>(L, "A")
		.def("f", &f)
		.def(constructor<int>())
		.def(constructor<const A&>())
		.def(constructor<>());

	class_<B>(L, "B")
		.def(constructor<int>())
		.def(constructor<>());

	if (dostring(L, "a = A(4)")) return false;
	if (feedback != 4) return false;

	if (dostring(L, "a2 = A(a)")) return false;
	if (feedback != 21) return false;

	if (dostring(L, "b = B(6)")) return false;
	if (feedback != 6) return false;
	if (feedback2 != 6) return false;

	if (dostring(L, "b2 = B()")) return false;
	if (feedback != 20) return false;
	if (feedback2 != 20) return false;

//	if (dostring(L, "a:f(a)")) return false;

	c.release();
	if (feedback != -1) return false;
	if (feedback2 != -1) return false;

	if (top != lua_gettop(L)) return false;

	return true;
}
