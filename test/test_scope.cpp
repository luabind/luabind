#include "test.h"
#include <luabind/functor.hpp>

namespace
{
	LUABIND_ANONYMOUS_FIX int feedback = 0;

	void f()
	{
		feedback = 123;
	}

	void f_(int a)
	{
		feedback = 124;
	}

	void f__(int a)
	{
		feedback = 125;
	}

	void g()
	{
		feedback = 2;
	}

	void g_(int)
	{
		feedback = 5;
	}

	void h()
	{
		feedback = 1;
	}

	struct test_class
	{
		test_class()
		{
			feedback = 321;
		}
	};
}

class test_class2
{
public:
        test_class2(){};
        void string_string( const std::string& s1, const std::string& s2)
        {
//                std::cerr<<s1<<" "<<s2<<std::endl;
        };
};

bool test_scope()
{
	using namespace luabind;
	{
		lua_State* L = lua_open();
		lua_closer c(L);

		open(L);

module(L)
[
        class_<test_class2>("test_class2")
                .def(constructor<>())
                .def("string_string",&test_class2::string_string)
];

		module(L, "test")
		[
			class_<test_class>("test_class")
				.def(constructor<>())
				[
					def("inner_fun", &f)
				]
				.enum_("vals")
				[
					value("val1", 1),
					value("val2", 2)
				],

			def("f", &f),
			def("f", &f_),

			namespace_("inner")
			[
				def("g", &g),
				def("f", &f__)
			],

			namespace_("inner")
			[
				def("g", &g_)
			]

		];

		module(L, "test")
		[
			namespace_("inner")
			[
				def("h", &h)
			]
		];


		if (dostring(L, "test.f()")) return false;
		if (feedback != 123) return false;
		if (dostring(L, "test.f(3)")) return false;
		if (feedback != 124) return false;
		if (dostring(L, "a = test.test_class()")) return false;
		if (feedback != 321) return false;
		if (dostring(L, "b = test.test_class.val2")) return false;
		if (dostring(L, "test.inner.g()")) return false;
		if (feedback != 2) return false;
		if (dostring(L, "test.inner.g(7)")) return false;
		if (feedback != 5) return false;
		if (dostring(L, "test.inner.f(4)")) return false;
		if (feedback != 125) return false;
		if (dostring(L, "test.inner.h()")) return false;
		if (feedback != 1) return false;
	}

	return true;
}

