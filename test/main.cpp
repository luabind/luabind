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

#if 0

#include <luabind/runtime_converters.hpp>

namespace
{
	using namespace luabind::rt_converter;

	struct my_type
	{
		my_type()
		{ std::cout << "constructed\n"; }

		my_type(const my_type&)
		{ std::cout << "copied\n"; }

		~my_type()
		{ std::cout << "destructed\n"; }
	};

	void int_from_lua(lua_State* L, int index, rvalue_data* data)
	{
		data->result = new (data->data) int(lua_tonumber(L, index));
	}

	void float_from_lua(lua_State* L, int index, rvalue_data* data)
	{
		data->result = new (data->data) float(lua_tonumber(L, index));
	}

	void string_from_lua(lua_State* L, int index, rvalue_data* data)
	{
		data->result = new (data->data) std::string(lua_tostring(L, index));
	}

	void my_type_from_lua(lua_State* L, int index, rvalue_data* data)
	{
		data->result = new (data->data) my_type;
	}

	void f(int a, float b, const std::vector<float>& s)
	{
		int c = a + b;
	}

	template<class Source, class Target>
	struct implicit_conversion
	{
		typedef typename boost::add_reference<
			typename boost::add_const<Target>::type
		>::type target_t;

		typedef typename boost::add_reference<Source>::type source_t;

		implicit_conversion()
		{
			insert_converter(&convert, typeid(target_t));
		};

		static void convert(lua_State* L, int index, rvalue_data* data)
		{
			typename boost::python::detail::referent_storage<source_t>::type storage;

			rvalue_data tmp;
			tmp.data = storage.bytes;

			rvalue_convert(L, index, &tmp, registered<Source>::converters);

			new (data->data) Target(void_ptr_to_reference(tmp.result, (source_t(*)())0));
			data->result = data->data;

			if (tmp.result == tmp.data)
				boost::python::detail::destroy_referent(tmp.result, (source_t(*)())0);
		}
	};

	struct A
	{ int n; virtual void f() {} };

	struct B : A
	{ float e; virtual void f() {} };

	void test_runtime()
	{
		lua_State* L = lua_open();

		using namespace luabind;
		open(L);

		insert_converter(&int_from_lua, typeid(const int&));
		insert_converter(&float_from_lua, typeid(const float&));
//		insert_converter(&string_from_lua, typeid(const std::string&));
		insert_converter(&my_type_from_lua, typeid(const my_type&));

		implicit_conversion<int, std::vector<float> >();

		module(L)
		[
			def("f", &f, runtime(_1) + runtime(_2) + runtime(_3))
		];

		lua_close(L);
	}
}

#endif

int main()
{
	bool passed = true;

//	test_runtime();

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
	passed &= report_success(test_scope(), "scopes");
	passed &= report_success(test_yield(), "yield");

	if (passed) std::cout << "\n\nall tests passed\n";
	else std::cout << "\n\nsome tests failed\n";

	return 0;
}
