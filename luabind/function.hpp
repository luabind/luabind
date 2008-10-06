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


#ifndef LUABIND_FUNCTION_HPP_INCLUDED
#define LUABIND_FUNCTION_HPP_INCLUDED

#include <luabind/prefix.hpp>
#include <luabind/config.hpp>

#include <boost/config.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/apply_wrap.hpp>

#include <luabind/detail/call_function.hpp>
#include <luabind/detail/get_overload_signature.hpp>
#include <luabind/detail/compute_score.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/detail/overload_rep_base.hpp>
#include <luabind/detail/deduce_signature.hpp>

#include <luabind/scope.hpp>

namespace luabind
{
	namespace detail
	{

		namespace mpl = boost::mpl;
	  
		namespace free_functions
		{

			struct overload_rep: public overload_rep_base
			{
				template <class F, class Policies>
				overload_rep(F fn, Policies const& policies)
				  : fun(reinterpret_cast<void(*)()>(fn))
				{
					m_arity = compute_arity(deduce_signature(fn), policies);
				}

				typedef int(*call_ptr)(lua_State*, void(*)());

				inline void set_fun(call_ptr f) { call_fun = f; }
				inline int call(lua_State* L, void(*f)()) const { return call_fun(L, f); }

				// this is the actual function pointer to be called when this overload is invoked
				void (*fun)();

//TODO:		private: 

				call_ptr call_fun;
			};

    		struct LUABIND_API function_rep
			{
				function_rep(const char* name): m_name(name) {}
				void add_overload(const free_functions::overload_rep& o);

				const std::vector<overload_rep>& overloads() const throw() { return m_overloads; }

				const char* name() const { return m_name; }

			private:
				const char* m_name;

				// this have to be write protected, since each time an overload is
				// added it has to be checked for existence. add_overload() should
				// be used.
				std::vector<free_functions::overload_rep> m_overloads;
			};

			template<class F, class Policies>
			struct function_callback_s
			{
				static inline int apply(lua_State* L, void(*fun)())
				{
					F fn = reinterpret_cast<F>(fun);
					return invoke(L, fn, deduce_signature(fn), Policies());
				}
			};

			template<class F, class Policies>
			struct match_function_callback_s
			{
				static inline int apply(lua_State* L)
				{
					return compute_score(L, deduce_signature((F)0), Policies());
				}
			};

			LUABIND_API int function_dispatcher(lua_State* L);
		}
	}

	// deprecated
	template<class F, class Policies>
	void function(lua_State* L, const char* name, F f, const Policies& p)
	{
		module(L) [ def(name, f, p) ];
	}

	// deprecated
	template<class F>
	void function(lua_State* L, const char* name, F f)
	{
		luabind::function(L, name, f, detail::null_type());
	}

	namespace detail
	{
		template<class F, class Policies>
		struct function_commiter : detail::registration
		{
			function_commiter(const char* n, F f, const Policies& p)
				: m_name(n)
				, fun(f)
				, policies(p)
			{}

			virtual void register_(lua_State* L) const
			{
				detail::free_functions::overload_rep o(fun, Policies());

				o.set_match_fun(&detail::free_functions::match_function_callback_s<F, Policies>::apply);
				o.set_fun(&detail::free_functions::function_callback_s<F, Policies>::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
				o.set_sig_fun(&detail::get_free_function_signature<F>::apply);
#endif

				lua_pushstring(L, m_name);
				lua_gettable(L, -2);

				detail::free_functions::function_rep* rep = 0;
				if (lua_iscfunction(L, -1))
				{
					if (lua_getupvalue(L, -1, 2) != 0)
					{
						// check the magic number that identifies luabind's functions
						if (lua_touserdata(L, -1) == (void*)0x1337)
						{
							if (lua_getupvalue(L, -2, 1) != 0)
							{
								rep = static_cast<detail::free_functions::function_rep*>(lua_touserdata(L, -1));
								lua_pop(L, 1);
							}
						}
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1);

				if (rep == 0)
				{
					lua_pushstring(L, m_name);
					// create a new function_rep
					rep = static_cast<detail::free_functions::function_rep*>(lua_newuserdata(L, sizeof(detail::free_functions::function_rep)));
					new(rep) detail::free_functions::function_rep(m_name);

                    // STORE IN REGISTRY
                    lua_pushvalue(L, -1);
                    detail::ref(L);

					detail::class_registry* r = detail::class_registry::get_registry(L);
					assert(r && "you must call luabind::open() prior to any function registrations");
					detail::getref(L, r->lua_function());
					int ret = lua_setmetatable(L, -2);
					(void)ret;
					assert(ret != 0);

					// this is just a magic number to identify functions that luabind created
					lua_pushlightuserdata(L, (void*)0x1337);

					lua_pushcclosure(L, &free_functions::function_dispatcher, 2);
					lua_settable(L, -3);
				}

				rep->add_overload(o);
			}

			char const* m_name;
			F fun;
			Policies policies;
		};
	}

	template<class F, class Policies>
	scope def(const char* name, F f, const Policies& policies)
	{
		return scope(std::auto_ptr<detail::registration>(
			new detail::function_commiter<F,Policies>(name, f, policies)));
	}

	template<class F>
	scope def(const char* name, F f)
	{
		return scope(std::auto_ptr<detail::registration>(
			new detail::function_commiter<F,detail::null_type>(
				name, f, detail::null_type())));
	}

} // namespace luabind

#endif // LUABIND_FUNCTION_HPP_INCLUDED

