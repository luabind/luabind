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

#include <luabind/out_value_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/copy_policy.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/discard_result_policy.hpp>
#include <luabind/dependency_policy.hpp>
#include <luabind/luabind.hpp>

namespace
{

	struct policies_test_class
	{
		policies_test_class(const char* name): name_(name)
		{ ++count; }
		policies_test_class() { ++count; }
		~policies_test_class()
		{ --count; }

		std::string name_;

		policies_test_class* make(const char* name) const
		{
			return new policies_test_class(name);
		}

		void f(policies_test_class* p) { delete p; }
		const policies_test_class* internal_ref() { return this; }
		policies_test_class* self_ref()
		{ return this; }

		static int count;

//	private:
		policies_test_class(policies_test_class const& c): name_(c.name_)
		{ ++count; }
	};

	int policies_test_class::count = 0;

	policies_test_class global;

	void out_val(float* f) { *f = 3.f; }
	policies_test_class* copy_val() { return &global; }

	struct secret_type {};

	secret_type sec_;
	
	secret_type* secret() { return &sec_; }

} // anonymous namespace

void test_policies()
{
	lua_state L;

	using namespace luabind;

	module(L)
	[
		class_<policies_test_class>("test")
			.def(constructor<>())
			.def("f", &policies_test_class::f, adopt(_1))
			.def("make", &policies_test_class::make, adopt(return_value))
			.def("internal_ref", &policies_test_class::internal_ref, dependency(return_value, self))
			.def("self_ref", &policies_test_class::self_ref, return_reference_to(self)),

		def("out_val", &out_val, pure_out_value(_1)),
		def("copy_val", &copy_val, copy(result)),
		def("secret", &secret, discard_result)
	];

	// test copy
	DOSTRING(L,
		"a = secret()\n"
		"a = copy_val()\n"
		"a = nil\n"
		"collectgarbage()\n");

	BOOST_CHECK(policies_test_class::count == 1);

	// out_value
	DOSTRING(L, "a = out_val()\n");

	// return_reference_to
	DOSTRING(L,
		"a = test()\n"
		"b = a:self_ref()\n"
		"a = nil\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 2);

	DOSTRING(L,
		"b = nil\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 1);

	DOSTRING(L,
		"a = test()\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 2);

	DOSTRING(L,
		"b = a:internal_ref()\n"
		"a = nil\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 2);

	// two gc-cycles because dependency-table won't be collected in the
	// same cycle as the object_rep
	DOSTRING(L,
		"b = nil\n"
		"collectgarbage()\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 1);

	// adopt
	DOSTRING(L,
		"a = test()\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 2);

	DOSTRING(L,
		"b = a:make('tjosan')\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 3);

	DOSTRING(L,
		"a:f(b)\n"
		"a = nil\n"
		"collectgarbage()");

	BOOST_CHECK(policies_test_class::count == 1);

}

