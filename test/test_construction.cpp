#include "test.h"

namespace
{
	LUABIND_ANONYMOUS_FIX int feedback = 0;
	LUABIND_ANONYMOUS_FIX int feedback2 = 0;

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

	struct C {};

	struct A_
	{
		virtual void doSomething() = 0;
	};

	struct B_ : public A_
	{
		void doSomething() {}
	};

	struct C_ : public B_
	{
		void doMore() {}
	};

	
} // anonymous namespace


bool test_construction()
{
	using namespace luabind;

	lua_State* L = lua_open();
	lua_closer c(L);
	int top = lua_gettop(L);

	open(L);

	module(L)
	[
		class_<A>("A")
			.def("f", &f)
			.def(constructor<int>())
			.def(constructor<const A&>())
			.def(constructor<>()),

		class_<B>("B")
			.def(constructor<int>())
			.def(constructor<>()),

		class_<C>("C")

	];

	luabind::class_<A_>(L, "A_");

	luabind::class_<B_, A_>(L, "B_")
		.def(luabind::constructor<>())
		.def("doSomething", &B_::doSomething);

	luabind::class_<C_, B_>(L, "C_")
		.def(luabind::constructor<>())
		.def("doMore", &C_::doMore);

		
	if (dostring2(L, "a = C()") == 0) return false;
	lua_pop(L, 1); // pop error message

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

	if (top != lua_gettop(L)) return false;

	c.release();
	if (feedback != -1) return false;
	if (feedback2 != -1) return false;

	return true;
}
