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


#ifndef LUABIND_PROPERTY_HPP_INCLUDED
#define LUABIND_PROPERTY_HPP_INCLUDED

#include <luabind/config.hpp>

namespace luabind { namespace detail
{
	class object_rep;

	template<class R, class T, class Policies>
	int get(R(T::*f)() const, T* obj, lua_State* L, Policies* policies)  { return returns<R>::call(f, obj, L, policies); }

	template<class R, class T, class Policies>
	int get(R(*f)(const T*), T* obj, lua_State* L, Policies* policies) { return returns<R>::call(f, obj, L, policies); }

	template<class R, class T, class Policies>
	int get(R(*f)(const T&), T* obj, lua_State* L, Policies* policies) { return returns<R>::call(f, obj, L, policies); }

	template<class T, class F, class Policies>
	struct get_caller : Policies
	{
		get_caller() {}
		get_caller(const Policies& p): Policies(p) {}

		int operator()(lua_State* L, int pointer_offset, F f)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)

			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			return get(f, reinterpret_cast<T*>(static_cast<char*>(obj->ptr()) + pointer_offset), L, static_cast<Policies*>(this));
		}
	};

	template<class T, class A1, class Policies>
	int set(void(T::*f)(A1), T* obj, lua_State* L, Policies* policies)  { return returns<void>::call(f, obj, L, policies); }

	template<class R, class T, class A1, class Policies>
	int set(void(*f)(T*, A1), T* obj, lua_State* L, Policies* policies) { return returns<void>::call(f, obj, L, policies); }

	template<class T, class A1, class Policies>
	int set(void(*f)(T&, A1), T* obj, lua_State* L, Policies* policies) { return returns<void>::call(f, obj, L, policies); }

	template<class T, class F, class Policies>
	struct set_caller : Policies
	{
		int operator()(lua_State* L, int pointer_offset, F f)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			// 3. value

			// and since call() expects it's first
			// parameter on index 2 we need to
			// remove the key-parameter (parameter 2).
			object_rep* obj = reinterpret_cast<object_rep*>(lua_touserdata(L, 1));
			lua_remove(L, 2);
			return set(f, reinterpret_cast<T*>(static_cast<char*>(obj->ptr()) + pointer_offset), L, static_cast<Policies*>(this));
		}
	};

	// TODO: add support for policies
	template<class T, class D>
	struct auto_set
	{
		int operator()(lua_State* L, int pointer_offset, D T::*member)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			// 3. value
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			T* ptr =  reinterpret_cast<T*>(static_cast<char*>(obj->ptr()) + pointer_offset);
			typename default_policy::template generate_converter<D, lua_to_cpp>::type converter;
			ptr->*member = converter.apply(L, LUABIND_DECORATE_TYPE(D), 3);
			return 0;
		}
	};

	// TODO: add support for policies
	template<class T, class D>
	struct auto_get
	{
		int operator()(lua_State* L, int pointer_offset, D T::*member)
		{
			// parameters on the lua stack:
			// 1. object_rep
			// 2. key (property name)
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
			T* ptr =  reinterpret_cast<T*>(static_cast<char*>(obj->ptr()) + pointer_offset);
			typename default_policy::template generate_converter<D, cpp_to_lua>::type converter;
			converter.apply(L, ptr->*member);
			return 1;
		}
	};

}}

#endif // LUABIND_PROPERTY_HPP_INCLUDED

