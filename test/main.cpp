#include "test.h"

extern "C"
{
	#include "lauxlib.h"
	#include "lualib.h"
	#include "lua.h"
}

#include <iostream>
#include <algorithm>

#include <luabind/luabind.hpp>
#include <luabind/scope.hpp>

bool dostring(lua_State* L, const char* str)
{
	if (luaL_loadbuffer(L, str, std::strlen(str), str) || lua_pcall(L, 0, 0, 0))
	{
		const char* a = lua_tostring(L, -1);
		std::cout << a << "\n";
		lua_pop(L, 1);
		return true;
	}
	return false;
}

int dostring2(lua_State* L, const char* str)
{
	if (luaL_loadbuffer(L, str, std::strlen(str), str) || lua_pcall(L, 0, 0, 0))
	{
		return 1;
	}
	return 0;
}

bool report_success(bool result, const char* name)
{
	std::cout << name;
#ifdef BOOST_MSVC
	if (result) std::cout << ": passed\n";
	else std::cout << ": failed\n";
#else
	if (result) std::cout << ": \033[32mpassed\033[0m\n";
	else std::cout << ": \033[31mfailed\033[0m\n";
#endif
	return result;
}

template<class R, class T>
void test_constness(R(T::*)())
{
	std::cout << "non const\n";
}

template<class R, class T>
void test_constness(R(T::*)() const)
{
	std::cout << "const\n";
}

struct TestConst
{
	void f() const {}
	void g() {}
};

void f(int) {}

int main()
{
	{
		using namespace luabind;

		lua_State* L = lua_open();

		open(L);

		namespace_(L, "dwarf")
		[
			def("f", &f),
			
			namespace_("inner")
			[
				def("test", &f)
			]
		];

		namespace_(L, "dwarf")
		[
			def("g", &f)
		];

		dostring(L, "dwarf.f(4)");
		dostring(L, "dwarf.g(10)");
		dostring(L, "dwarf.inner.test(2)");

		lua_close(L);
	}

	test_constness(&TestConst::f);
	test_constness(&TestConst::g);

	bool passed = true;

	passed &= report_success(test_construction(), "construction");
	passed &= report_success(test_attributes(), "attributes");
	passed &= report_success(test_operators(), "operators");
	passed &= report_success(test_implicit_cast(), "implicit cast");
	passed &= report_success(test_const(), "const");
#ifndef LUABIND_NO_EXCEPTIONS
	passed &= report_success(test_exceptions(), "exceptions");
#else
	std::cout << "exceptions: skipped \n";
#endif
	passed &= report_success(test_null_pointer(), "null pointer");
	passed &= report_success(test_policies(), "policies");
	passed &= report_success(test_lua_classes(), "lua classes");
	passed &= report_success(test_free_functions(), "free functions");
	passed &= report_success(test_object(), "object");
	passed &= report_success(test_held_type(), "held type");
	passed &= report_success(test_iterator(), "iterator");

	if (passed) std::cout << "\n\nall tests passed\n";
	else std::cout << "\n\nsome tests failed\n";

	return 0;
}

