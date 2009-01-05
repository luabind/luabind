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

#define LUABIND_BUILDING

#include <luabind/lua_include.hpp>

#include <luabind/detail/stack_utils.hpp>
#include <luabind/detail/conversion_storage.hpp>
#include <luabind/luabind.hpp>
#include <luabind/exception_handler.hpp>
#include <utility>

using namespace luabind::detail;

namespace luabind { namespace detail
{
	LUABIND_API int property_tag(lua_State* L)
	{
		lua_pushstring(L, "luabind: property_tag function can't be called");
		lua_error(L);
		return 0;
	}
}}

luabind::detail::class_rep::class_rep(type_id const& type
	, const char* name
	, lua_State* L
)
	: m_type(type)
	, m_name(name)
	, m_class_type(cpp_class)
	, m_operator_cache(0)
{
	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	lua_rawgeti(L, LUA_REGISTRYINDEX, r->cpp_class());
	lua_setmetatable(L, -2);

	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref.set(L);

	m_instance_metatable = r->cpp_instance();
}

luabind::detail::class_rep::class_rep(lua_State* L, const char* name)
	: m_type(typeid(null_type))
	, m_name(name)
	, m_class_type(lua_class)
	, m_operator_cache(0)
{
	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	lua_rawgeti(L, LUA_REGISTRYINDEX, r->lua_class());
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref.set(L);

	m_instance_metatable = r->lua_instance();
}

luabind::detail::class_rep::~class_rep()
{
}

// leaves object on lua stack
std::pair<void*,void*> 
luabind::detail::class_rep::allocate(lua_State* L) const
{
	const int size = sizeof(object_rep);
	char* mem = static_cast<char*>(lua_newuserdata(L, size));
	return std::pair<void*,void*>(mem, 0);
}

// lua stack: userdata, key
int luabind::detail::class_rep::gettable(lua_State* L)
{
	// if key is nil, return nil
	if (lua_isnil(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)
	const char* key = lua_tostring(L, 2);

#ifndef LUABIND_NO_ERROR_CHECKING
	if (std::strlen(key) != lua_strlen(L, 2))
	{
		lua_pushfstring(L,
			"luabind does not support keys with "
			"embedded nulls: '%s'", lua_tostring(L, 2));
		lua_error(L);
	}
#endif

	// special case to see if this is a null-pointer
	if (key && !std::strcmp(key, "__ok"))
	{
		lua_pushstring(L, "awaiting reimplementation or removal");
		lua_error(L);

		return 1;
	}

	// first, look in the instance's table..
	detail::lua_reference const& tbl = obj->get_lua_table();
	if (tbl.is_valid())
	{
		tbl.get(L);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
		lua_remove(L, -2);
	}
	else
	{
		lua_pushnil(L);
	}

	// .. if it's not found in the instance, look in the class table.
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		obj->crep()->get_table(L);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
		lua_remove(L, -2);
	}

	if (lua_tocfunction(L, -1) == &property_tag)
	{
		// this member is a property, extract the "get" function and call it.
		lua_getupvalue(L, -1, 1);
		lua_pushvalue(L, 1);
		lua_call(L, 1, 1);
	}

	return 1;
}

// called from the metamethod for __newindex
// the object pointer is passed on the lua stack
// lua stack: userdata, key, value
bool luabind::detail::class_rep::settable(lua_State* L)
{
	// if the key is 'nil' fail
	if (lua_isnil(L, 2)) return false;

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)

	const char* key = lua_tostring(L, 2);

#ifndef LUABIND_NO_ERROR_CHECKING
	if (std::strlen(key) != lua_strlen(L, 2))
	{
		lua_pushfstring(L,
			"luabind does not support keys with "
			"embedded nulls: '%s'", lua_tostring(L, 2));
		lua_error(L);
	}
#endif

	// set the attribute to the object's table
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	detail::lua_reference& tbl = obj->get_lua_table();
	if (!tbl.is_valid())
	{
		// this is the first time we are trying to add
		// a member to this instance, create the table.
		lua_newtable(L);
		lua_pushvalue(L, -1);
		tbl.set(L);
		lua_pushnil(L);
	}
	else
	{
		tbl.get(L);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
	}

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		obj->crep()->get_table(L);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
		lua_remove(L, -2);
	}

	if (lua_tocfunction(L, -1) == &property_tag)
	{
		// this member is a property, extract the "set" function and call it.
		lua_getupvalue(L, -1, 2);

		if (lua_isnil(L, -1))
		{
			lua_pushfstring(L, "property '%s' is read only", lua_tostring(L, 2));
			lua_error(L);
		}

		lua_pushvalue(L, 1);
		lua_pushvalue(L, 3);
		lua_call(L, 2, 0);
		return true;
	}

	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_settable(L, 4);
	lua_pop(L, 3);

	return true;
}

int class_rep::gettable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	return obj->crep()->gettable(L);
}

// this is called as __newindex metamethod on every instance of this class
int luabind::detail::class_rep::settable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

	bool success = obj->crep()->settable(L);

#ifndef LUABIND_NO_ERROR_CHECKING

	if (!success)
	{
		// class_rep::settable() will leave
		// error message on the stack in case
		// of failure
		lua_error(L);
	}

#endif

	return 0;
}


int luabind::detail::class_rep::operator_dispatcher(lua_State* L)
{
	for (int i = 0; i < 2; ++i)
	{
		if (is_class_object(L, 1 + i))
		{
            int nargs = lua_gettop(L);

            lua_pushvalue(L, lua_upvalueindex(1));
			lua_gettable(L, 1 + i);

			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}

			lua_insert(L, 1); // move the function to the bottom

            nargs = lua_toboolean(L, lua_upvalueindex(2)) ? 1 : nargs;

            if (lua_toboolean(L, lua_upvalueindex(2))) // remove trailing nil
                lua_remove(L, 3);

            lua_call(L, nargs, 1);
            return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushstring(L, "No such operator defined");
	lua_error(L);

	return 0;
}

// this is called as metamethod __call on the class_rep.
int luabind::detail::class_rep::constructor_dispatcher(lua_State* L)
{
    class_rep* cls = static_cast<class_rep*>(lua_touserdata(L, 1));

    int args = lua_gettop(L);

    lua_newtable(L);
    lua_reference ref;
    ref.set(L);

    void* obj_ptr;
    void* held_storage;

    boost::tie(obj_ptr, held_storage) = cls->allocate(L);
    new(obj_ptr) object_rep(cls, ref);

    lua_rawgeti(L, LUA_REGISTRYINDEX, cls->metatable_ref());
    lua_setmetatable(L, -2);

    if (cls->get_class_type() == class_rep::lua_class
     && !cls->bases().empty())
    {
        lua_pushstring(L, "super");
        lua_pushvalue(L, 1);
        lua_pushvalue(L, -3);
        lua_pushcclosure(L, super_callback, 2);
        lua_settable(L, LUA_GLOBALSINDEX);
    }

    lua_pushvalue(L, -1);
    lua_replace(L, 1);

    cls->get_table(L);
    lua_pushstring(L, "__init");
    lua_gettable(L, -2);

    lua_insert(L, 1);

    lua_pop(L, 1);
    lua_insert(L, 1);

    lua_call(L, args, 0);

    return 1;
}

void luabind::detail::class_rep::add_base_class(const luabind::detail::class_rep::base_info& binfo)
{
	// If you hit this assert you are deriving from a type that is not registered
	// in lua. That is, in the class_<> you are giving a baseclass that isn't registered.
	// Please note that if you don't need to have access to the base class or the
	// conversion from the derived class to the base class, you don't need
	// to tell luabind that it derives.
	assert(binfo.base && "You cannot derive from an unregistered type");

	class_rep* bcrep = binfo.base;

	// import all static constants
	for (std::map<const char*, int, ltstr>::const_iterator i = bcrep->m_static_constants.begin(); 
			i != bcrep->m_static_constants.end(); ++i)
	{
		int& v = m_static_constants[i->first];
		v = i->second;
	}

	// also, save the baseclass info to be used for typecasts
	m_bases.push_back(binfo);
}

namespace
{

  bool super_deprecation_disabled = false;

} // namespace unnamed

LUABIND_API void luabind::disable_super_deprecation()
{
    super_deprecation_disabled = true;
}

int luabind::detail::class_rep::super_callback(lua_State* L)
{
	if (!super_deprecation_disabled)
	{
		lua_pushstring(L,
			"DEPRECATION: 'super' has been deprecated in favor of "
			"directly calling the base class __init() function. "
			"This error can be disabled by calling 'luabind::disable_super_deprecation()'."
		);
		lua_error(L);
	}

	int args = lua_gettop(L);
		
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	class_rep* base = crep->bases()[0].base;

	if (base->bases().empty())
	{
		lua_pushstring(L, "super");
		lua_pushnil(L);
		lua_settable(L, LUA_GLOBALSINDEX);
	}
	else
	{
		lua_pushstring(L, "super");
		lua_pushlightuserdata(L, base);
		lua_pushvalue(L, lua_upvalueindex(2));
		lua_pushcclosure(L, super_callback, 2);
		lua_settable(L, LUA_GLOBALSINDEX);
	}

	base->get_table(L);
	lua_pushstring(L, "__init");
	lua_gettable(L, -2);
	lua_insert(L, 1);
	lua_pop(L, 1);

	lua_pushvalue(L, lua_upvalueindex(2));
	lua_insert(L, 2);

	lua_call(L, args + 1, 0);

	// TODO: instead of clearing the global variable "super"
	// store it temporarily in the registry. maybe we should
	// have some kind of warning if the super global is used?
	lua_pushstring(L, "super");
	lua_pushnil(L);
	lua_settable(L, LUA_GLOBALSINDEX);

	return 0;
}



int luabind::detail::class_rep::lua_settable_dispatcher(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

	// get first table
	crep->get_table(L);

	// copy key, value
	lua_pushvalue(L, -3);
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	// pop table
	lua_pop(L, 1);

	// get default table
	crep->get_default_table(L);
	lua_replace(L, 1);
	lua_rawset(L, -3);

	crep->m_operator_cache = 0; // invalidate cache
	
	return 0;
}

/*
	stack:
	1: class_rep
	2: member name
*/
int luabind::detail::class_rep::static_class_gettable(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

	// look in the static function table
	crep->get_default_table(L);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1)) return 1;
	else lua_pop(L, 2);

	const char* key = lua_tostring(L, 2);

	if (std::strlen(key) != lua_strlen(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	std::map<const char*, int, ltstr>::const_iterator j = crep->m_static_constants.find(key);

	if (j != crep->m_static_constants.end())
	{
		lua_pushnumber(L, j->second);
		return 1;
	}

#ifndef LUABIND_NO_ERROR_CHECKING

	{
		std::string msg = "no static '";
		msg += key;
		msg += "' in class '";
		msg += crep->name();
		msg += "'";
		lua_pushstring(L, msg.c_str());
	}
	lua_error(L);

#endif

	lua_pushnil(L);

	return 1;
}

bool luabind::detail::is_class_rep(lua_State* L, int index)
{
	if (lua_getmetatable(L, index) == 0) return false;

	lua_pushstring(L, "__luabind_classrep");
	lua_gettable(L, -2);
	if (lua_toboolean(L, -1))
	{
		lua_pop(L, 2);
		return true;
	}

	lua_pop(L, 2);
	return false;
}

void luabind::detail::finalize(lua_State* L, class_rep* crep)
{
	if (crep->get_class_type() != class_rep::lua_class) return;

//	lua_pushvalue(L, -1); // copy the object ref
	crep->get_table(L);
	lua_pushstring(L, "__finalize");
	lua_gettable(L, -2);
	lua_remove(L, -2);

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
	}
	else
	{
		lua_pushvalue(L, -2);
		lua_call(L, 1, 0);
	}

	for (std::vector<class_rep::base_info>::const_iterator 
			i = crep->bases().begin(); i != crep->bases().end(); ++i)
	{
		if (i->base) finalize(L, i->base);
	}
}

void luabind::detail::class_rep::cache_operators(lua_State* L)
{
	m_operator_cache = 0x1;

	for (int i = 0; i < number_of_operators; ++i)
	{
		get_table(L);
		lua_pushstring(L, get_operator_name(i));
		lua_rawget(L, -2);

		if (lua_isfunction(L, -1)) m_operator_cache |= 1 << (i + 1);

		lua_pop(L, 2);
	}
}

bool luabind::detail::class_rep::has_operator_in_lua(lua_State* L, int id)
{
	if ((m_operator_cache & 0x1) == 0)
		cache_operators(L);

	const int mask = 1 << (id + 1);

	return (m_operator_cache & mask) != 0;
}
