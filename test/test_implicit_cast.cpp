#include "test.h"

namespace
{
	LUABIND_ANONYMOUS_FIX int feedback = 0;

	struct A { virtual ~A() {} };
	struct B: public A {};

	struct test_implicit
	{
		void f(A*) { feedback = 1; }
		void f(B*) { feedback = 2; }
	};


	struct char_pointer_convertable
	{
		operator const char*() const { return "foo!"; }
	};

	void func(const char_pointer_convertable& f)
	{
		if (std::string("foo!") == (const char*)f) feedback = 3;
	}

} // anonymous namespace
/*
#include <luabind/pointer_holder.hpp>

void test_instance_holder(lua_State* L)
{
	using namespace luabind;

//	pointer_holder<A, A*> holder(new B);

//	void* p = holder.holds(LUABIND_TYPEID(B));
}
*/
bool test_implicit_cast()
{
	using namespace luabind;

	lua_State* L = lua_open();
	lua_closer c(L);
	int top = lua_gettop(L);

	open(L);

	typedef void (test_implicit::*f1)(A*);
	typedef void (test_implicit::*f2)(B*);

	module(L)
	[
		class_<A>("A")
			.def(constructor<>()),
	
		class_<B, A>("B")
			.def(constructor<>()),
	
		class_<test_implicit>("test")
			.def(constructor<>())
			.def("f", (f1) &test_implicit::f)
			.def("f", (f2) &test_implicit::f),

		class_<char_pointer_convertable>("char_ptr")
			.def(constructor<>()),

		def("func", &func)
	];

//	test_instance_holder(L);

	if (dostring(L, "a = A()")) return false;
	if (dostring(L, "b = B()")) return false;
	if (dostring(L, "t = test()")) return false;

	if (dostring(L, "t:f(a)")) return false;
	if (feedback != 1) return false;

	if (dostring(L, "t:f(b)")) return false;
	if (feedback != 2) return false;

	if (dostring(L,
		"a = char_ptr()\n"
		"func(a)\n")) return false;
	if (feedback != 3) return false;
	
	if (top != lua_gettop(L)) return false;

	return true;
}
