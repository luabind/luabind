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
	struct operator_tester : counted_type<operator_tester>
	{
		int operator+(int a) const
		{
			return 1 + a;
		}

		float operator-() const
		{
			return 4.6f;
		}

		float operator()() const
		{
			return 3.5f;
		}

		float operator()(int a) const
		{
			return 3.5f + a;
		}

		float operator()(int a)
		{
			return 2.5f + a;
		}

	};

	int operator+(int a, const operator_tester&)
	{
		return 2 + a;
	}

	struct operator_tester2 : counted_type<operator_tester2>
	{
	};

	float operator+(const operator_tester&, const operator_tester2&)
	{
		return 7.3f;
	}

	struct operator_tester3: operator_tester, counted_type<operator_tester3> {};

	const operator_tester* make_const_test()
	{
		static operator_tester o;
		return &o;
	}

	std::ostream& operator<<(std::ostream& os, const operator_tester&)
	{
		os << "operator_tester"; return os;
	}
	
	operator_tester* clone(const operator_tester* p)
	{ return new operator_tester(*p); }

} // anonymous namespace

void test_operators()
{
    COUNTER_GUARD(operator_tester);
    COUNTER_GUARD(operator_tester2);
    COUNTER_GUARD(operator_tester3);

	lua_state L;

	using namespace luabind;

	module(L)
	[
		class_<operator_tester>("operator_tester")
			.def(constructor<>())
			.def(tostring(const_self))
			.def(self + int())
			.def(other<int>() + self)
			.def(-self)
			.def(self())
			.def(const_self(int()))
			.def(self(int())),
//			.def("clone", &clone, adopt(return_value)),

		class_<operator_tester2>("operator_tester2")
			.def(constructor<>())
			.def(other<const operator_tester&>() + self),

		class_<operator_tester3, bases<operator_tester> >("operator_tester3")
			.def(constructor<>()),
	
		def("make_const_test", &make_const_test)
	];
	
	DOSTRING(L, "test = operator_tester()");
	DOSTRING(L, "test2 = operator_tester2()");
	DOSTRING(L, "test3 = operator_tester3()");
	DOSTRING(L, "const_test = make_const_test()");

	DOSTRING(L, "assert(test() == 3.5)");
	DOSTRING(L, "assert(test(5) == 2.5 + 5)");
	DOSTRING(L, "assert(const_test(7) == 3.5 + 7)");

	DOSTRING(L, "assert(-test == 4.6)");
	DOSTRING(L, "assert(test + test2 == 7.3)");
	DOSTRING(L, "assert(2 + test == 2 + 2)");
	DOSTRING(L, "assert(test + 2 == 1 + 2)");
	DOSTRING(L, "assert(test3 + 6 == 1 + 6)");
	DOSTRING(L, "assert(test3 + test2 == 7.3)");
	DOSTRING(L, "assert(tostring(test) == 'operator_tester')");

	const char* prog =
		"class 'my_class'\n"
		"function my_class:__add(lhs)\n"
		"	return my_class(self.val + lhs.val)\n"
		"end\n"
		"function my_class:__init(a)\n"
		"	self.val = a\n"
		"end\n"
		"function my_class:__sub(v)\n"
		"	if (type(self) == 'number') then\n"
		"		return my_class(self - v.val)\n"
		"	elseif (type(v) == 'number') then\n"
		"		return my_class(self.val - v)\n"
		"	else\n"
		"		return my_class(self.val - v.val)\n"
		"	end\n"
		"end\n"
		"a = my_class(3)\n"
		"b = my_class(7)\n"
		"c = a + b\n"
		"d = a - 2\n"
		"d = 10 - d\n"
		"d = d - b\n";

	DOSTRING(L, prog);
	DOSTRING(L, "assert(c == 10)");
	DOSTRING(L, "assert(d == 2)");
}
