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


#ifndef LUABIND_CLASS_REGISTRY_HPP_INCLUDED
#define LUABIND_CLASS_REGISTRY_HPP_INCLUDED

#include <luabind/config.hpp>
#include <string>

namespace luabind { namespace detail
{
	class class_rep;

	std::string stack_content_by_name(lua_State* L, int start_index);

	struct class_registry
	{
		class_registry()
			: m_cpp_instance_metatable(LUA_NOREF)
			, m_cpp_class_metatable(LUA_NOREF)
			, m_lua_instance_metatable(LUA_NOREF)
			, m_lua_class_metatable(LUA_NOREF)
		{}

		static inline class_registry* get_registry(lua_State* L)
		{

#ifdef LUABIND_NOT_THREADSAFE

			// if we don't have to be thread safe, we can keep a
			// chache of the class_registry pointer without the
			// need of a mutex
			static lua_State* cache_key = 0;
			static class_registry* registry_cache = 0;
			if (cache_key == L) return registry_cache;

#endif

			lua_pushstring(L, "__luabind_classes");
			lua_gettable(L, LUA_REGISTRYINDEX);
			class_registry* p = static_cast<class_registry*>(lua_touserdata(L, -1));
			lua_pop(L, 1);

#ifdef LUABIND_NOT_THREADSAFE

			cache_key = L;
			registry_cache = p;

#endif

			// luabind::open() is required
			//assert(p);

			return p;
		}

		std::map<LUABIND_TYPE_INFO, class_rep*> classes;

		// this is a lua reference that points to the lua table
		// that is to be used as meta table for all C++ class 
		// instances. It is a kind of v-table.
		int m_cpp_instance_metatable;

		// this is a lua reference to the metatable to be used
		// for all classes defined in C++.
		int m_cpp_class_metatable;

		// this is a lua reference that points to the lua table
		// that is to be used as meta table for all lua class
		// instances. It is a kind of v-table.
		int m_lua_instance_metatable;

		// this is a lua reference to the metatable to be used
		// for all classes defined in lua
		int m_lua_class_metatable;

	};

}}

#endif // LUABIND_CLASS_REGISTRY_HPP_INCLUDED

