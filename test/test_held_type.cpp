#include <boost/shared_ptr.hpp>


namespace luabind
{
	namespace detail
	{
		template<class T>
		T* get_pointer(boost::shared_ptr<T>& p) { return p.get(); }
	}
}

#include "test.h"

namespace
{

	LUABIND_ANONYMOUS_FIX int feedback = 0;

	struct held_type_test
	{
		held_type_test() : n(4), val(4) { feedback = 3; }
		~held_type_test() { feedback = 1; }

		void f(int)
		{
			feedback = n;
		}

		int n;
		int val;
	};

	void tester(boost::shared_ptr<held_type_test> t)
	{
		if (t->val == 4) feedback = 2;
	}

	struct base
	{
	};

	struct derived : base
	{
		void f() {}
	};

	void f(boost::shared_ptr<base>) {}

} // anonymous namespace


bool test_held_type()
{
	// This feature is not finished yet

	using namespace luabind;

	boost::shared_ptr<held_type_test> ptr(new held_type_test());
	
	{
		lua_State* L = lua_open();
		lua_closer c(L);
		int top = lua_gettop(L);

		open(L);

		function(L, "tester", &tester);

		class_<held_type_test, boost::shared_ptr<held_type_test> >("held_type_test")
			.def(constructor<>())
			.def("f", &held_type_test::f)
			.commit(L)
			;

		namespace_(L, "held_test")
		[
			class_<base, boost::shared_ptr<base> >("base"),

			class_<derived, base, boost::shared_ptr<derived> >("derived")
				.def("f", &derived::f),

			def("f", &f)
		];

		object g = get_globals(L);
		g["test"] = ptr;
		if (dostring(L, "tester(test)")) return false;
		if (feedback != 2) return false;
		if (dostring(L, "a = held_type_test()"))
		if (feedback != 3) return false;
		if (dostring(L, "test:f(4)")) return false;
		if (feedback != 4) return false;

		g["a"] = boost::shared_ptr<base>(new derived());

		if (dostring(L, "held_test.f(a)")) return false;
		
		if (top != lua_gettop(L)) return false;

	}

//	if (feedback != 3) return false;

	ptr.reset();
	
	if (feedback != 1) return false;

	return true;
}

