#ifndef LUABIND_SCOPE_HPP_INCLUDED
#define LUABIND_SCOPE_HPP_INCLUDED

#include <vector>
#include <string>
#include <stack>

#include <luabind/detail/ref.hpp>

namespace luabind
{
	namespace detail
	{
		struct scoped_sequence;

		struct scoped_object
		{
			scoped_object()
			{
			}

			virtual ~scoped_object() {}

			virtual void commit(lua_State*) const = 0;
			virtual scoped_object* clone() const = 0;

			scoped_sequence operator,(const scoped_object& rhs) const;
		};

		struct scoped_sequence : scoped_object
		{
			scoped_sequence(scoped_object* a, scoped_object* b)
			{
				this->objects.push_back(a);
				this->objects.push_back(b);
			}

			scoped_sequence() {}

			virtual ~scoped_sequence()
			{
					for (std::vector<scoped_object*>
							::const_iterator i = this->objects.begin()
												; i != this->objects.end()
												; ++i)
					{
						scoped_object* ptr = *i;
						delete ptr;
					}
			}

			scoped_sequence& operator,(const scoped_object& rhs)
			{
				objects.push_back(rhs.clone());
				return *this;
			}

			virtual scoped_object* clone() const
			{
				scoped_sequence* copy = new scoped_sequence();
				copy->objects.swap(this->objects);
				return copy;
			}

			virtual void commit(lua_State* L) const
			{
				for (std::vector<scoped_object*>
							::const_iterator i = this->objects.begin()
												; i != this->objects.end()
												; ++i)
				{
					scoped_object* ptr = *i;
					ptr->commit(L);
				}
			}

			mutable std::vector<scoped_object*> objects;
		};

		inline scoped_sequence scoped_object::operator,(const scoped_object& rhs) const
		{
			return scoped_sequence(this->clone(), rhs.clone());
		}
	}
/*
	def(..),
	def(..)

	function_obj1, function_obj2
	function_obj1.push_back(function_obj2.doclone());
*/
	struct scope_stack
	{
		std::stack<int> scopes;

		static int gc(lua_State* L)
		{
			scope_stack* scopes = static_cast<scope_stack*>(
				lua_touserdata(L, -1)
			);

			assert(scopes->scopes.size() == 1);

			int top = scopes->scopes.top();
			scopes->scopes.pop();

			detail::unref(L, top);

			scopes->~scope_stack();

			return 0;
		}

		static int top(lua_State* L)
		{
			lua_pushstring(L, "__luabind_scope_stack");
			lua_gettable(L, LUA_REGISTRYINDEX);

			scope_stack* scopes = static_cast<scope_stack*>(
				lua_touserdata(L, -1)
			);

			lua_pop(L, 1);

			return scopes->scopes.top();
		}

		static void push(lua_State* L)
		{
			lua_pushstring(L, "__luabind_scope_stack");
			lua_gettable(L, LUA_REGISTRYINDEX);

			scope_stack* scopes = static_cast<scope_stack*>(
				lua_touserdata(L, -1)
			);

			lua_pop(L, 1);

			return scopes->scopes.push(detail::ref(L));
		}

		static void pop(lua_State* L)
		{
			lua_pushstring(L, "__luabind_scope_stack");
			lua_gettable(L, LUA_REGISTRYINDEX);

			scope_stack* scopes = static_cast<scope_stack*>(
				lua_touserdata(L, -1)
			);

			lua_pop(L, 1);

			int n = scopes->scopes.top();
			scopes->scopes.pop();
			detail::unref(L, n);
		}
	};

	class scope : public detail::scoped_object
	{
	public:

		static void init(lua_State* L)
		{
			lua_pushstring(L, "__luabind_scope_stack");
			lua_gettable(L, LUA_REGISTRYINDEX);

			scope_stack* scopes = static_cast<scope_stack*>(
				lua_touserdata(L, -1)
			);

			lua_pop(L, 1);

			if (scopes == 0)
			{
				scopes = static_cast<scope_stack*>(
					lua_newuserdata(L, sizeof(scope_stack))
				);

				lua_pushstring(L, "__luabind_scope_stack");
				lua_pushvalue(L, -2);
				lua_settable(L, LUA_REGISTRYINDEX);

				new (scopes) scope_stack();

				lua_pushvalue(L, LUA_GLOBALSINDEX);
				scopes->scopes.push(detail::ref(L));

				lua_newtable(L);
				lua_pushstring(L, "__gc");
				lua_pushcclosure(L, scope_stack::gc, 0);
				lua_settable(L, -3);
				lua_setmetatable(L, -2);

				lua_pop(L, 1);
			}
		}

		scope(lua_State* L, const char* name)
			: m_state(L)
			, m_name(name)
		{
			init(L);
		}

		scope(const char* name)
			: m_state(0)
			, m_name(name)
		{
		}

		virtual ~scope()
		{
			for (std::vector<scoped_object*>
					::const_iterator i = m_children.begin()
										; i != m_children.end()
										; ++i)
			{
				scoped_object* ptr = *i;
				delete ptr;
			}
		}

		scope& operator[](const detail::scoped_object& x)
		{
			m_children.push_back(x.clone());

			if (m_state)
			{
				this->commit(m_state);

				for (std::vector<scoped_object*>
						::const_iterator i = m_children.begin()
											; i != m_children.end()
											; ++i)
				{
					scoped_object* ptr = *i;
					delete ptr;
				}

				m_children.clear();
			}

			return *this;
		}

		virtual detail::scoped_object* clone() const
		{
			std::vector<detail::scoped_object*> tmp;
			tmp.swap(this->m_children);
			scope* copy = new scope(*this);
			copy->m_children.swap(tmp);
			return copy;
		}

		virtual void commit(lua_State* L) const
		{
			init(L);

			detail::getref(L, scope_stack::top(L)); // get current scope
			lua_pushstring(L, m_name.c_str());
			lua_gettable(L, -2);
			lua_remove(L, -2); // remove scope

			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);

				lua_newtable(L);
				detail::getref(L, scope_stack::top(L));
				lua_pushstring(L, m_name.c_str());
				lua_pushvalue(L, -3);
				lua_settable(L, -3);
				lua_pop(L, 1);
			}

			scope_stack::push(L);

			for (std::vector<detail::scoped_object*>
						::const_iterator i = m_children.begin()
	 										; i != m_children.end()
											; ++i)
			{
				scoped_object* ptr = *i;
				ptr->commit(L);
			}

			scope_stack::pop(L);
		}

	private:

		mutable std::vector<scoped_object*> m_children;
		lua_State* m_state;
		std::string m_name;
	};

	typedef scope namespace_;
}
/*
scope dwarf("dwarf");

dwarf
[
	def(..),
	def(..),

	class<vec3>("vec3")
		.def(constructor<float,float,float>())
		.def(self + self)
		.def(self - self)
		.def(self == self)
		,

	def("getDirection", &get_direction),
	def("getPosition", &get_position)
];

dwarf
[
	def(..)
];

dwarf.commit(L);

*/
#endif

