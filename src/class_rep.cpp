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

#include <luabind/luabind.hpp>
#include <luabind/detail/class_rep.hpp>

using namespace luabind::detail;

class_rep::class_rep(LUABIND_TYPE_INFO t, 
       		         const char* name, 
			            lua_State* L, 
			            void(*destructor)(void*), 
			            LUABIND_TYPE_INFO held_t, 
			            void*(*extractor)(void*))
	: m_type(t)
	, m_held_type(held_t)
	, m_extract_underlying_fun(extractor)
	, m_name(name)
	, m_class_type(cpp_class)
	, m_destructor(destructor)
{
	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->cpp_class());
	lua_setmetatable(L, -2);

	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref = detail::ref(L); // pop one of them

	m_instance_metatable = r->cpp_instance();
}

class_rep::class_rep(lua_State* L, const char* name)
	: m_type(LUABIND_TYPEID(int))
	, m_held_type(0)
	, m_extract_underlying_fun(0)
	, m_name(name)
	, m_class_type(lua_class)
{
	// TODO: don't we need to copy the name?
	lua_newtable(L);
	m_table_ref = detail::ref(L);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->lua_class());
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref = detail::ref(L); // pop one of them

	m_instance_metatable = r->lua_instance();
}

class_rep::~class_rep()
{
#ifndef LUABIND_DONT_COPY_STRINGS
	for (std::vector<char*>::iterator i = m_strings.begin(); 
			i != m_strings.end(); ++i)
	{
			delete[] *i;
	}
#endif
}

int class_rep::gettable(lua_State* L)
{
	if (lua_isnil(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)
	const char* key = lua_tostring(L, 2);
	std::map<const char*, method_rep, ltstr>::iterator i = m_methods.find(key);

	if (i != m_methods.end())
	{
		// the name is a method, return it
		lua_pushlightuserdata(L, &i->second);
		lua_pushcclosure(L, function_dispatcher, 1);
		return 1;
	}

	std::map<const char*, callback, ltstr>::iterator j = m_getters.find(key);
	if (j != m_getters.end())
	{
		// the name is a data member
		return j->second.func(L, j->second.pointer_offset);
	}

	lua_pushnil(L);
	return 1;
}

// called from the metamethod for __newindex
// the object pointer is passed on the lua stack
bool class_rep::settable(lua_State* L)
{
	if (lua_isnil(L, 2))
	{
		return false;
	}

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)
	const char* key = lua_tostring(L, 2);
	std::map<const char*, callback, ltstr>::iterator j = m_setters.find(key);
	if (j != m_setters.end())
	{
		// the name is a data member
		j->second.func(L, j->second.pointer_offset);
		return true;
	}

	return false; // false means that we don't have a member with the given name
}

static int class_rep::gettable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	return obj->crep()->gettable(L);
}

// this is called as __newindex metamethod on every instance of this class
static int class_rep::settable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

	bool success = obj->crep()->settable(L);

#ifndef LUABIND_NO_ERROR_CHECKING

	if (!success)
	{
		// this block is needed to make sure the std::string is destructed before
		// lua_error() is called
#ifdef BOOST_MSVC
		{
			// msvc has a bug which deletes the string twice, that's
			// why we have to create it on the heap
			std::string* msg = new std::string("cannot set attribute '");
			*msg += obj->crep()->m_name;
			*msg += ".";
			*msg += lua_tostring(L, -2);
			*msg += "'";
			lua_pushstring(L, msg->c_str());
			delete msg;
		}
#else
		{
			std::string msg = "cannot set attribute '";
			msg += obj->crep()->m_name;
			msg += ".";
			msg += lua_tostring(L, -2);
			msg += "'";
			lua_pushstring(L, msg.c_str());
		}
#endif
		lua_error(L);
	}

#endif

	return 0;
}


static int class_rep::operator_dispatcher(lua_State* L)
{
	int id = static_cast<int>(lua_tonumber(L, lua_upvalueindex(1)));

	int operand_id = 0;

	object_rep* operand[2];
	for (int i = 0; i < 2; ++i)
		operand[i] = detail::is_class_object(L, i + 1);

	if (operand[0] && operand[1])
		if (LUABIND_TYPE_INFO_EQUAL(operand[0]->crep()->type(), operand[1]->crep()->type())) operand[1] = 0;

	std::vector<operator_callback>* overloads[2];
	for (int i = 0; i < 2; ++i)
		if (operand[i]) overloads[i] = &operand[i]->crep()->m_operators[id]; else overloads[i] = 0;

	std::size_t num_overloads[2];
	for (int i = 0; i < 2; ++i)
		if (overloads[i]) num_overloads[i] = overloads[i]->size(); else num_overloads[i] = 0;

	bool ambiguous = false;
	int match_index = -1;
	int min_match = std::numeric_limits<int>::max();

#ifdef LUABIND_NO_ERROR_CHECKING

	if (num_overloads[0] == 1 && num_overloads[1] == 0)
	{
		operand_id = 0;
		match_index = 0;
	}
	else if (num_overloads[0] == 0 && num_overloads[1] == 1)
	{
		operand_id = 1;
		match_index = 0;
	}
	else
	{

#endif

		int num_params = lua_gettop(L);
		if (overloads[0])
		{
			if (find_best_match(L, overloads[0]->begin(), overloads[0]->end(), ambiguous, min_match, match_index, num_params))
				operand_id = 0;
		}

		// have look at the right operand.
		// if the right operand is a class and
		// not the same class as this, we have to
		// try to match it's operators too	

		if (overloads[1])
		{
			if(find_best_match(L, overloads[1]->begin(), overloads[1]->end(), ambiguous, min_match, match_index, num_params))
				operand_id = 1;
		}

#ifdef LUABIND_NO_ERROR_CHECKING

	}

#else

	if (match_index == -1)
	{
		// this block is needed to make sure the std::string is destructed before
		// lua_error() is called
		{
			std::string msg = "no operator ";
			msg += get_operator_symbol(id);
			msg += " matched the arguments (";
			msg += stack_content_by_name(L, 1);
			msg += ")\ncandidates are:\n";

			if (overloads[0])
				msg += get_overload_signatures(L, overloads[0]->begin(), overloads[0]->end(), get_operator_symbol(id));

			if (overloads[1])
				msg += get_overload_signatures(L, overloads[1]->begin(), overloads[1]->end(), get_operator_symbol(id));
					
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}
	else if (ambiguous)
	{
		// this block is needed to make sure the std::string is destructed before
		// lua_error() is called
		{
			std::string msg = "call of overloaded operator ";
			msg += get_operator_symbol(id);
			msg += " (";
			msg += stack_content_by_name(L, 1);
			msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

			std::vector<operator_callback> candidates;
			if (overloads[0])
				find_exact_match(L, overloads[0]->begin(), overloads[0]->end(), min_match, num_params, candidates);

			if (overloads[1])
				find_exact_match(L, overloads[1]->begin(), overloads[1]->end(), min_match, num_params, candidates);

			msg += get_overload_signatures(L, candidates.begin(), candidates.end(), get_operator_symbol(id));


			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

	return (*overloads[operand_id])[match_index].call(L);
}

// this is called as metamethod __call on the class_rep.
static int class_rep::constructor_dispatcher(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));
	construct_rep* rep = &crep->m_constructor;

	bool ambiguous = false;
	int match_index = -1;
	int min_match = std::numeric_limits<int>::max();
	bool found;

#ifdef LUABIND_NO_ERROR_CHECKING

	if (rep->overloads.size() == 1)
	{
		match_index = 0;
	}
	else
	{

#endif

		int num_params = lua_gettop(L) - 1;
		found = find_best_match(L, rep->overloads.begin(), rep->overloads.end(), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING
	}

#else

	if (!found)
	{
		// this block is needed to make sure the std::string is destructed before
		// lua_error() is called
		{
			std::string msg = "no constructor of '";
			msg += crep->name();
			msg += "' matched the arguments (";
			msg += stack_content_by_name(L, 2);
			msg += ")\n candidates are:\n";

			msg += get_overload_signatures(L, rep->overloads.begin(), rep->overloads.end(), crep->name());

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}
	else if (ambiguous)
	{
		// this block is needed to make sure the std::string is destructed before
		// lua_error() is called
		{
			std::string msg = "call of overloaded constructor '";
			msg += crep->m_name;
			msg +=  "(";
			msg += stack_content_by_name(L, 2);
			msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

			std::vector<construct_rep::overload_t> candidates;
			find_exact_match(L, rep->overloads.begin(), rep->overloads.end(), min_match, num_params, candidates);
			msg += get_overload_signatures(L, candidates.begin(), candidates.end(), crep->name());
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

	try
	{

#endif

		void* object_ptr = rep->overloads[match_index].construct(L);

		void* obj_rep = lua_newuserdata(L, sizeof(object_rep));
		new(obj_rep) object_rep(object_ptr, crep, object_rep::owner, crep->destructor());

		detail::getref(L, crep->m_instance_metatable);
		lua_setmetatable(L, -2);
		return 1;

#ifndef LUABIND_NO_EXCEPTIONS

	}

	catch(const std::exception& e)
	{
		lua_pushstring(L, e.what());
	}
	catch(const char* s)
	{
		lua_pushstring(L, s);
	}
	catch(...)
	{
		{
			std::string msg = crep->name();
			msg += "() threw an exception";
			lua_pushstring(L, msg.c_str());
		}
	}

	// we can only reach this line if an exception was thrown
	lua_error(L);
	return 0; // will never be reached

#endif

}

static int class_rep::implicit_cast(const class_rep* from, const class_rep* to, int& pointer_offset)
{
	int offset = 0;
	if (LUABIND_TYPE_INFO_EQUAL(from->type(), to->type())) return 0;

	for (std::vector<class_rep::base_info>::const_iterator i = from->bases().begin(); i != from->bases().end(); ++i)
	{
		int steps = implicit_cast(i->base, to, offset);
		pointer_offset = offset + i->pointer_offset;
		if (steps >= 0) return steps + 2;
	}
	return -1;
}

// the functions dispatcher assumes the following:
// there is one upvalue that points to the method_rep that this dispatcher is to call
// the first parameter on the lua stack is an object_rep that points to the object the
// call is being made on
static int class_rep::function_dispatcher(lua_State* L)
{
	method_rep* rep = static_cast<method_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	object_rep* obj = reinterpret_cast<object_rep*>(lua_touserdata(L, 1));

#ifndef LUABIND_NO_ERROR_CHECKING

	if (is_class_object(L, 1) == 0)
	{
		{
			std::string msg = "No self reference given as first parameter to member function '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "'. Have you used '.' instead of ':'?";

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

	int p;
	if (implicit_cast(obj->crep(), rep->crep, p) < 0)
	{
		{
			std::string msg = "invalid self reference given to '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "'";
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

	bool ambiguous = false;
	int match_index = -1;
	int min_match = std::numeric_limits<int>::max();
	bool found;

#ifdef LUABIND_NO_ERROR_CHECKING
	if (rep->overloads().size() == 1)
	{
		match_index = 0;
	}
	else
	{
#endif

		int num_params = lua_gettop(L) - 1;
		found = find_best_match(L, rep->overloads().begin(), rep->overloads().end(), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

	}

#else

	if (!found)
	{
		{
			std::string msg = "no overload of  '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "' matched the arguments (";
			msg += stack_content_by_name(L, 2);
			msg += ")\ncandidates are:\n";

			std::string function_name;
			function_name += rep->crep->name();
			function_name += ":";
			function_name += rep->name;

			msg += get_overload_signatures(L, rep->overloads().begin(), rep->overloads().end(), function_name);

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}
	else if (ambiguous)
	{
		{
			std::string msg = "call of overloaded  '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "(";
			msg += stack_content_by_name(L, 2);
			msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

			std::vector<overload_rep> candidates;
			find_exact_match(L, rep->overloads().begin(), rep->overloads().end(), min_match, num_params, candidates);

			std::string function_name;
			function_name += rep->crep->name();
			function_name += ":";
			function_name += rep->name;

			msg += get_overload_signatures(L, candidates.begin(), candidates.end(), function_name);

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

	try
	{

#endif

		const overload_rep& o = rep->overloads()[match_index];
		return o.call(L, *obj);

#ifndef LUABIND_NO_EXCEPTIONS

	}
	catch(const std::exception& e)
	{
		lua_pushstring(L, e.what());
	}
	catch (const char* s)
	{
		lua_pushstring(L, s);
	}
	catch(...)
	{
		std::string msg = rep->crep->name();
		msg += ":";
		msg += rep->name;
		msg += "() threw an exception";
		lua_pushstring(L, msg.c_str());
	}
	// we can only reach this line if an exception was thrown
	lua_error(L);
	return 0; // will never be reached

#endif
			
}

