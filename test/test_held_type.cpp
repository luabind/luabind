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
		held_type_test() { feedback = 3; }
		~held_type_test() { feedback = 1; }
	};

	void tester(held_type_test* t)
	{
		feedback = 2;
	}

} // anonymous namespace


bool test_held_type()
{
	// This feature is not finished yet

	using namespace luabind;

	{
		lua_State* L = lua_open();
		lua_closer c(L);
		int top = lua_gettop(L);

		open(L);

		function(L, "tester", &tester);

		class_<held_type_test, boost::shared_ptr<held_type_test> >("held_type_test")
			.def(constructor<>())
			.commit(L)
			;

		object g = get_globals(L);
		g["test"] = boost::shared_ptr<held_type_test>(new held_type_test());
		if (dostring(L, "tester(test)")) return false;
		if (feedback != 2) return false;
		if (dostring(L, "a = held_type_test()"))
		if (feedback != 3) return false;
		if (top != lua_gettop(L)) return false;

	}
	if (feedback != 1) return false;

	return true;
}

