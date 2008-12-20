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

#ifndef LUABIND_NO_ERROR_CHECKING

	std::string luabind::detail::get_overload_signatures_candidates(
			lua_State* L
			, std::vector<const overload_rep_base*>::iterator start
			, std::vector<const overload_rep_base*>::iterator end
			, std::string name)
	{
		std::string s;
		for (; start != end; ++start)
		{
			s += name;
			(*start)->get_signature(L, s);
			s += "\n";
		}
		return s;
	}

#endif


luabind::detail::class_rep::class_rep(LUABIND_TYPE_INFO type
	, const char* name
	, lua_State* L
	,  void(*destructor)(void*)
	,  void(*const_holder_destructor)(void*)
	, LUABIND_TYPE_INFO holder_type
	, LUABIND_TYPE_INFO const_holder_type
	, void*(*extractor)(void*)
	, const void*(*const_extractor)(void*)
	, void(*const_converter)(void*,void*)
	, void(*construct_holder)(void*,void*)
	, void(*construct_const_holder)(void*,void*)
	, void(*default_construct_holder)(void*)
	, void(*default_construct_const_holder)(void*)
	, void(*adopt_fun)(void*)
	, int holder_size
	, int holder_alignment)

	: m_type(type)
	, m_holder_type(holder_type)
	, m_const_holder_type(const_holder_type)
	, m_extractor(extractor)
	, m_const_extractor(const_extractor)
	, m_const_converter(const_converter)
	, m_construct_holder(construct_holder)
	, m_construct_const_holder(construct_const_holder)
	, m_default_construct_holder(default_construct_holder)
	, m_default_construct_const_holder(default_construct_const_holder)
	, m_adopt_fun(adopt_fun)
	, m_holder_size(holder_size)
	, m_holder_alignment(holder_alignment)
	, m_name(name)
	, m_class_type(cpp_class)
	, m_destructor(destructor)
	, m_const_holder_destructor(const_holder_destructor)
	, m_operator_cache(0)
{
	assert(m_holder_alignment >= 1 && "internal error");

	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->cpp_class());
	lua_setmetatable(L, -2);

	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref.set(L);

	m_instance_metatable = r->cpp_instance();
}

luabind::detail::class_rep::class_rep(lua_State* L, const char* name)
	: m_type(LUABIND_INVALID_TYPE_INFO)
	, m_holder_type(LUABIND_INVALID_TYPE_INFO)
	, m_const_holder_type(LUABIND_INVALID_TYPE_INFO)
	, m_extractor(0)
	, m_const_extractor(0)
	, m_const_converter(0)
	, m_construct_holder(0)
	, m_construct_const_holder(0)
	, m_default_construct_holder(0)
	, m_default_construct_const_holder(0)
	, m_adopt_fun(0)
	, m_holder_size(0)
	, m_holder_alignment(1)
	, m_name(name)
	, m_class_type(lua_class)
	, m_destructor(0)
	, m_const_holder_destructor(0)
	, m_operator_cache(0)
{
	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->lua_class());
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
	const int overlap = sizeof(object_rep)&(m_holder_alignment-1);
	const int padding = overlap==0?0:m_holder_alignment-overlap;
	const int size = sizeof(object_rep) + padding + m_holder_size;

	char* mem = static_cast<char*>(lua_newuserdata(L, size));
	char* ptr = mem + sizeof(object_rep) + padding;

	return std::pair<void*,void*>(mem,ptr);
}

void luabind::detail::class_rep::adopt(bool const_obj, void* obj)
{
	if (m_adopt_fun == 0) return;

	if (m_extractor)
	{
		assert(m_const_extractor);
		if (const_obj)
			m_adopt_fun(const_cast<void*>(m_const_extractor(obj)));
		else
			m_adopt_fun(m_extractor(obj));
	}
	else
	{
		m_adopt_fun(obj);
	}
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
		class_rep* crep = obj->crep();

		void* p = crep->extractor() ? crep->extractor()(obj->ptr())
			: obj->ptr();

		lua_pushboolean(L, p != 0);
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

    int flags = object_rep::lua_class | object_rep::owner;

    boost::tie(obj_ptr, held_storage) = cls->allocate(L);
    (new(obj_ptr) object_rep(cls, flags, ref))->set_object(held_storage);

    detail::getref(L, cls->metatable_ref());
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

	// import all getters from the base
	for (std::map<const char*, callback, ltstr>::const_iterator i = bcrep->m_getters.begin(); 
			i != bcrep->m_getters.end(); ++i)
	{
		callback& m = m_getters[i->first];
		m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
		m.func = i->second.func;

#ifndef LUABIND_NO_ERROR_CHECKING
		m.match = i->second.match;
		m.sig = i->second.sig;
#endif
	}

	// import all setters from the base
	for (std::map<const char*, callback, ltstr>::const_iterator i = bcrep->m_setters.begin(); 
			i != bcrep->m_setters.end(); ++i)
	{
		callback& m = m_setters[i->first];
		m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
		m.func = i->second.func;

#ifndef LUABIND_NO_ERROR_CHECKING
		m.match = i->second.match;
		m.sig = i->second.sig;
#endif
	}

	// import all static constants
	for (std::map<const char*, int, ltstr>::const_iterator i = bcrep->m_static_constants.begin(); 
			i != bcrep->m_static_constants.end(); ++i)
	{
		int& v = m_static_constants[i->first];
		v = i->second;
	}

	// import all operators
	for (int i = 0; i < number_of_operators; ++i)
	{
		for (std::vector<operator_callback>::const_iterator j = bcrep->m_operators[i].begin(); 
				j != bcrep->m_operators[i].end(); ++j)
			m_operators[i].push_back(*j);
	}

	// also, save the baseclass info to be used for typecasts
	m_bases.push_back(binfo);

    if (!m_destructor)
        m_destructor = bcrep->m_destructor;
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
		
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, lua_upvalueindex(2)));
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	class_rep* base = crep->bases()[0].base;

	if (base->bases().empty())
	{
		obj->set_flags(obj->flags() & ~object_rep::call_super);

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

void* luabind::detail::class_rep::convert_to(
	LUABIND_TYPE_INFO target_type
	, const object_rep* obj
	, conversion_storage& storage) const
{
	// TODO: since this is a member function, we don't have to use the accesor functions for
	// the types and the extractor

	assert(obj == 0 || obj->crep() == this);

	int steps = 0;
	int offset = 0;
	if (!(LUABIND_TYPE_INFO_EQUAL(holder_type(), target_type))
		&& !(LUABIND_TYPE_INFO_EQUAL(const_holder_type(), target_type)))
	{
		steps = implicit_cast(this, target_type, offset);
	}

	// should never be called with a type that can't be cast
	assert((steps >= 0) && "internal error, please report");

	if (LUABIND_TYPE_INFO_EQUAL(target_type, holder_type()))
	{
		if (obj == 0)
		{
			// we are trying to convert nil to a holder type
			m_default_construct_holder(&storage.data);
			storage.destructor = m_destructor;
			return &storage.data;
		}
		// if the type we are trying to convert to is the holder_type
		// it means that his crep has a holder_type (since it would have
		// been invalid otherwise, and T cannot be invalid). It also means
		// that we need no conversion, since the holder_type is what the
		// object points to.
		return obj->ptr();
	}

	if (LUABIND_TYPE_INFO_EQUAL(target_type, const_holder_type()))
	{
		if (obj == 0)
		{
			// we are trying to convert nil to a const holder type
			m_default_construct_const_holder(&storage.data);
			storage.destructor = m_const_holder_destructor;
			return &storage.data;
		}

		if (obj->flags() & object_rep::constant)
		{
			// we are holding a constant
			return obj->ptr();
		}
		else
		{
			// we are holding a non-constant, we need to convert it
			// to a const_holder.
			m_const_converter(obj->ptr(), &storage.data);
			storage.destructor = m_const_holder_destructor;
			return &storage.data;
		}
	}

	void* raw_pointer;

	if (has_holder())
	{
		assert(obj);
		// this means that we have a holder type where the
		// raw-pointer needs to be extracted
		raw_pointer = extractor()(obj->ptr());
	}
	else
	{
		if (obj == 0) raw_pointer = 0;
		else raw_pointer = obj->ptr();
	}

	return static_cast<char*>(raw_pointer) + offset;
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

const class_rep::property_map& luabind::detail::class_rep::properties() const
{
	return m_getters;
}

