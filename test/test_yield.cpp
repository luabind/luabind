// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "test.hpp"
#include <luabind/luabind.hpp>

namespace
{
	struct test_class : counted_type<test_class>
	{
		test_class(): n(0) {}

		int f() const
		{
			return const_cast<int&>(n)++;
		}

		int n;
	};

	int f(int a)
	{
		return 9;
	}

	int j(lua_State* L)
	{
		lua_pushnumber(L, 9);
		return lua_yield(L, 1);
	}

	void f() {}
}

#include <iostream>

void test_yield()
{
    COUNTER_GUARD(test_class);

	lua_state L;

	using namespace luabind;

	module(L)
	[
		class_<test_class>("test")
			.def(constructor<>())
			.def("f", &test_class::f, luabind::yield)
	];

/*
	DOSTRING(L, "function g() a = test() for i = 1, 10 do print(a:f()) end end");

	lua_pushstring(L, "j");
	lua_pushcclosure(L, j, 0);
	lua_settable(L, LUA_GLOBALSINDEX);

	lua_State* thread = lua_newthread(L);
	lua_pushstring(thread, "g");
	lua_gettable(thread, LUA_GLOBALSINDEX);

	if (lua_resume(thread, 0))
	{
		std::cout << "error: " << lua_tostring(thread, -1) << '\n';
	}

	for (int i = 0; i < 10; ++i)
	{
		std::cout << "iteration: " << i << ", top: " << lua_gettop(thread) << '\n';

		lua_resume(thread, lua_gettop(thread));
	}
*/
}

