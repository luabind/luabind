#include "test.h"
#include <luabind/functor.hpp>

namespace
{
	LUABIND_ANONYMOUS_FIX int feedback = 0;

	void f()
	{
		feedback = 123;
	}

	void g()
	{
		feedback = 2;
	}

	struct test_class
	{
		test_class()
		{
			feedback = 321;
		}
	};
}

bool test_scope()
{
	using namespace luabind;
	{
		lua_State* L = lua_open();
		lua_closer c(L);

		open(L);

		module(L, "test")
		[
			class_<test_class>("test_class")
				.def(constructor<>())
				.enum_("vals")
				[
					value("val1", 1),
					value("val2", 2)
				],

			def("f", &f),

			namespace_("inner")
			[
				def("g", &g)
			]

		];

		if (dostring(L, "test.f()")) return false;
		if (feedback != 123) return false;
		if (dostring(L, "a = test.test_class()")) return false;
		if (feedback != 321) return false;
		if (dostring(L, "b = test.test_class.val2")) return false;
		if (dostring(L, "test.inner.g()")) return false;
		if (feedback != 2) return false;
	}

	return true;
}

