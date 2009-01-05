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

#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/class_rep.hpp>

namespace luabind { namespace detail
{

	// dest is a function that is called to delete the c++ object this struct holds
	object_rep::object_rep(instance_holder* instance, class_rep* crep)
		: m_instance(instance)
		, m_classrep(crep)
		, m_dependency_cnt(1)
	{}

	object_rep::object_rep(class_rep* crep, detail::lua_reference const& table_ref)
		: m_instance(0)
		, m_classrep(crep)
		, m_lua_table_ref(table_ref)
		, m_dependency_cnt(1)
	{}

	object_rep::~object_rep()
	{
        if (!m_instance)
            return;
        m_instance->~instance_holder();
        deallocate(m_instance);
	}

	void object_rep::add_dependency(lua_State* L, int index)
	{
		if (!m_dependency_ref.is_valid())
		{
			lua_newtable(L);
			m_dependency_ref.set(L);
		}

		m_dependency_ref.get(L);
		lua_pushvalue(L, index);
		lua_rawseti(L, -2, m_dependency_cnt);
		lua_pop(L, 1);
		++m_dependency_cnt;
	}

	int object_rep::garbage_collector(lua_State* L)
	{
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, -1));

		finalize(L, obj->crep());

		obj->~object_rep();
		return 0;
	}

}}

