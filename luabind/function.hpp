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


#if !BOOST_PP_IS_ITERATING

#ifndef LUABIND_FUNCTIONS_HPP_INCLUDED
#define LUABIND_FUNCTIONS_HPP_INCLUDED

#include <luabind/config.hpp>

#include <boost/config.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp> 
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/cat.hpp>

#include <luabind/detail/signature_match.hpp>
#include <luabind/detail/call_function.hpp>
#include <luabind/detail/get_overload_signature.hpp>
#include <luabind/detail/overload_rep_base.hpp>

#include <luabind/scope.hpp>

namespace luabind
{
	namespace detail
	{

		namespace free_functions
		{

			struct overload_rep: public overload_rep_base
			{

#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 1))
#include BOOST_PP_ITERATE()

				inline bool operator==(const overload_rep& o) const
				{
					if (o.m_arity != m_arity) return false;
					if (o.m_num_args != m_num_args) return false;
					for (int i = 0; i < m_num_args; ++i)
						if (!(LUABIND_TYPE_INFO_EQUAL(m_params[i], o.m_params[i]))) return false;
					return true;
				}

				typedef int(*call_ptr)(lua_State*, void(*)());

				inline void set_fun(call_ptr f) { call_fun = f; }
				inline int call(lua_State* L, void(*f)()) const { return call_fun(L, f); }

				// this is the actual function pointer to be called when this overload is invoked
				void (*fun)();

			private:

				int m_num_args;
				call_ptr call_fun;

				// the number of parameters this overload takes
				// these are used to detect when a function by a derived class.
//				int m_arity;
				// the types of the parameter it takes
				LUABIND_TYPE_INFO m_params[LUABIND_MAX_ARITY];
			};




			struct function_rep
			{
				void add_overload(const free_functions::overload_rep& o)
				{
					std::vector<free_functions::overload_rep>::iterator i = std::find(m_overloads.begin(), m_overloads.end(), o);

					// if the overload already exists, overwrite the existing function
					if (i != m_overloads.end())
					{
						*i = o;
					}
					else
					{
						m_overloads.push_back(o);
					}
				}

				const std::vector<overload_rep>& overloads() const throw() { return m_overloads; }

#ifdef LUABIND_DONT_COPY_STRINGS
				const char* name;
#else
				std::string name;
#endif

			private:
				// this have to be write protected, since each time an overload is
				// added it has to be checked for existence. add_overload() should
				// be used.
				std::vector<free_functions::overload_rep> m_overloads;
			};



		// returns generates functions that calls function pointers
/*
#define LUABIND_DECL(z, n, text) typedef typename detail::get_policy<n+1,Policies>::type BOOST_PP_CAT(a##n,_policy); \
		typedef typename BOOST_PP_CAT(a##n,_policy)::head BOOST_PP_CAT(a##n,_param_converter_intermediate); \
		typedef typename BOOST_PP_CAT(a##n,_param_converter_intermediate)::template generate_converter<A##n, lua_to_cpp>::type BOOST_PP_CAT(p##n,_conv);

#define LUABIND_PARAMS(z,n,text) BOOST_PP_CAT(p##n,_conv)::apply(L, LUABIND_DECORATE_TYPE(A##n), n + 1)
*/

#define LUABIND_DECL(z, n, text) typedef typename find_conversion_policy<n + 1, Policies>::type BOOST_PP_CAT(converter_policy,n); \
		typename BOOST_PP_CAT(converter_policy,n)::template generate_converter<A##n, lua_to_cpp>::type BOOST_PP_CAT(c,n);

#define LUABIND_ADD_INDEX(z,n,text) + BOOST_PP_CAT(converter_policy,n)::has_arg
#define LUABIND_INDEX_MAP(z,n,text) 1 BOOST_PP_REPEAT(n, LUABIND_ADD_INDEX, _)
#define LUABIND_PARAMS(z,n,text) BOOST_PP_CAT(c,n).apply(L, LUABIND_DECORATE_TYPE(A##n), LUABIND_INDEX_MAP(_,n,_))
#define LUABIND_POSTCALL(z,n,text) BOOST_PP_CAT(c,n).converter_postcall(L, LUABIND_DECORATE_TYPE(A##n), LUABIND_INDEX_MAP(_,n,_));

			template<class T>
			struct returns
			{
				#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 2))
				#include BOOST_PP_ITERATE()
			};

			template<>
			struct returns<void>
			{
				#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 3))
				#include BOOST_PP_ITERATE()
			};

			#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 4))
			#include BOOST_PP_ITERATE()


#undef LUABIND_PARAMS
#undef LUABIND_DECL
#undef LUABIND_POSTCALL
#undef LUABIND_ADD_INDEX
#undef LUABIND_INDEX_MAP


			#define BOOST_PP_ITERATION_PARAMS_1 (4, (0, LUABIND_MAX_ARITY, <luabind/function.hpp>, 5))
			#include BOOST_PP_ITERATE()


			

			struct function_registry
			{
				std::map<const char*, free_functions::function_rep, ltstr> m_functions;
#ifndef LUABIND_DONT_COPY_STRINGS
				std::vector<char*> m_strings;

				~function_registry()
				{
					for (std::vector<char*>::iterator i = m_strings.begin(); i != m_strings.end(); ++i)
						delete[] *i;
				}
#endif
			};

			template<class F, class Policies>
			struct function_callback_s
			{
				static inline int apply(lua_State* L, void(*fun)())
				{
					return free_functions::call(reinterpret_cast<F>(fun), L, static_cast<const Policies*>(0));
				}
			};

			template<class F, class Policies>
			struct match_function_callback_s
			{
				static inline int apply(lua_State* L)
				{
					F fptr = 0;
					return free_functions::match(fptr, L, static_cast<Policies*>(0));
				}

				static int callback(lua_State* L)
				{
					F fptr = 0;
					return free_functions::match(fptr, L, static_cast<Policies*>(0));
				}
			};

#ifdef LUABIND_NO_HEADERS_ONLY
			int function_dispatcher(lua_State* L);
#else
//			inline int function_dispatcher(lua_State* L)
			{
				function_rep* rep = static_cast<function_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	
				bool ambiguous = false;
				int min_match = std::numeric_limits<int>::max();
				int match_index = -1;
				bool ret;

#ifdef LUABIND_NO_ERROR_CHECKING
				
				if (rep->overloads().size() == 1)
				{
					match_index = 0;
				}
				else
				{

#endif
					int num_params = lua_gettop(L);
					ret = find_best_match(L, &rep->overloads().front(), rep->overloads().size(), sizeof(free_functions::overload_rep), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING
				}

#else

				if (!ret)
				{
					// this bock is needed to make sure the std::string is destructed
					{
						std::string msg = "no match for function call '";
						msg += rep->name;
						msg += "' with the parameters (";
						msg += stack_content_by_name(L, 1);
						msg += ")\ncandidates are:\n";

						msg += get_overload_signatures(L, rep->overloads().begin(), rep->overloads().end(), rep->name);

						lua_pushstring(L, msg.c_str());
					}
					lua_error(L);
				}

				if (ambiguous)
				{
					// this bock is needed to make sure the std::string is destructed
					{
						std::string msg = "call of overloaded function '";
						msg += rep->name;
						msg += "(";
						msg += stack_content_by_name(L, 1);
						msg += ") is ambiguous\nnone of the overloads have a best conversion:";

						std::vector<const overload_rep_base*> candidates;
						find_exact_match(L, &rep->overloads().front(), rep->overloads().size(), sizeof(free_functions::overload_rep), min_match, num_params, candidates);

						msg += get_overload_signatures_candidates(L, candidates.begin(), candidates.end(), rep->name);

						lua_pushstring(L, msg.c_str());
					}
					lua_error(L);
				}
#endif
				const overload_rep& ov_rep = rep->overloads()[match_index];
				return ov_rep.call(L, ov_rep.fun);
			}
#endif
		}
	}
	
	template<class F, class Policies>
	void function(lua_State* L, const char* name, F f, const Policies&)
	{
		detail::free_functions::function_registry* registry = 0;
		lua_pushstring(L, "__lua_free_functions");
		lua_gettable(L, LUA_REGISTRYINDEX);

		registry = static_cast<detail::free_functions::function_registry*>(lua_touserdata(L, -1));
		lua_pop(L, 1);

		// if you hit this assert you have not called luabind::open() for
		// this lua_State. See the documentation for more information.
		assert(registry != 0);

#ifdef LUABIND_DONT_COPY_STRINGS
		detail::free_functions::function_rep& rep = registry->m_functions[name];
#else
		registry->m_strings.push_back(detail::dup_string(name));
		detail::free_functions::function_rep& rep = registry->m_functions[registry->m_strings.back()];
#endif

		detail::free_functions::overload_rep o(f, static_cast<Policies*>(0));

		o.set_match_fun(&detail::free_functions::match_function_callback_s<F, Policies>::apply);
		o.set_fun(&detail::free_functions::function_callback_s<F, Policies>::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
		o.set_sig_fun(&detail::get_free_function_signature<F>::apply);
#endif

		rep.add_overload(o);
		rep.name = name;

		lua_pushstring(L, name);
		lua_pushlightuserdata(L, &rep);
		lua_pushcclosure(L, detail::free_functions::function_dispatcher, 1);
		lua_settable(L, LUA_GLOBALSINDEX);
	}

	template<class F>
	void function(lua_State* L, const char* name, F f)
	{
		luabind::function(L, name, f, detail::null_type());
	}

	namespace detail
	{
		template<class F, class Policies>
		struct function_commiter : detail::scoped_object
		{
			function_commiter(const char* n, F f, const Policies& p)
				: name(n)
				, fun(f)
				, policies(p)
			{}

			virtual detail::scoped_object* clone() const
			{
				return new function_commiter(*this);
			}

			virtual void commit(lua_State* L) const
			{
				detail::free_functions::function_registry* registry = 0;
				lua_pushstring(L, "__lua_free_functions");
				lua_gettable(L, LUA_REGISTRYINDEX);

				registry = static_cast<detail::free_functions::function_registry*>(lua_touserdata(L, -1));
				lua_pop(L, 1);

				// if you hit this assert you have not called luabind::open() for
				// this lua_State. See the documentation for more information.
				assert(registry != 0);

#ifdef LUABIND_DONT_COPY_STRINGS
				detail::free_functions::function_rep& rep = registry->m_functions[name.c_str()];
#else
				registry->m_strings.push_back(detail::dup_string(name.c_str()));
				detail::free_functions::function_rep& rep = registry->m_functions[registry->m_strings.back()];
#endif

				detail::free_functions::overload_rep o(fun, static_cast<Policies*>(0));

				o.set_match_fun(&detail::free_functions::match_function_callback_s<F, Policies>::apply);
				o.set_fun(&detail::free_functions::function_callback_s<F, Policies>::apply);

#ifndef LUABIND_NO_ERROR_CHECKING
				o.set_sig_fun(&detail::get_free_function_signature<F>::apply);
#endif

				rep.add_overload(o);
				rep.name = name.c_str();

				detail::getref(L, scope_stack::top(L));
				lua_pushstring(L, name.c_str());
				lua_pushlightuserdata(L, &rep);
				lua_pushcclosure(L, free_functions::function_dispatcher, 1);
				lua_settable(L, -3);
			}

			std::string name;
			F fun;
			Policies policies;
		};
	}

	template<class F, class Policies>
	detail::function_commiter<F,Policies>
	def(const char* name, F f, const Policies& policies)
	{
		return detail::function_commiter<F,Policies>(name, f, policies);
	}

	template<class F>
	detail::function_commiter<F, detail::null_type>
	def(const char* name, F f)
	{
		return detail::function_commiter<F,detail::null_type>(name, f, detail::null_type());
	}

} // namespace luabind


#endif // LUABIND_FUNCTION_HPP_INCLUDED

#elif BOOST_PP_ITERATION_FLAGS() == 1

// overloaded template funtion that initializes the parameter list
// called m_params and the m_arity member.

#define LUABIND_INIT_PARAM(z, n, _) m_params[n] = LUABIND_TYPEID(A##n);
#define LUABIND_POLICY_DECL(z,n,text) typedef typename find_conversion_policy<n + 1, Policies>::type BOOST_PP_CAT(p,n);
#define LUABIND_ARITY(z,n,text) + BOOST_PP_CAT(p,n)::has_arg

template<class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A), class Policies>
overload_rep(R(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), Policies*)
	: fun(reinterpret_cast<void(*)()>(f))
	, m_num_args(BOOST_PP_ITERATION())
{
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_INIT_PARAM, _)
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POLICY_DECL, _)

	m_arity = 0 BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_ARITY, _);
}

#undef LUABIND_INIT_PARAM
#undef LUABIND_POLICY_DECL
#undef LUABIND_ARITY

#elif BOOST_PP_ITERATION_FLAGS() == 2

template<class Policies BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
static int call(T(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies*)
{
/*	typedef typename get_policy<0, Policies>::type ret_policy;
	typedef typename ret_policy::head return_value_converter_intermediate;
	typedef typename return_value_converter_intermediate::template generate_converter<T, cpp_to_lua>::type ret_conv;*/

	int nargs = lua_gettop(L);

//	typedef typename get_policy_list<0, Policies>::type policy_list_ret;
//	typedef typename find_converter_policy<policy_list_ret>::type converter_policy_ret;
	typedef typename find_conversion_policy<0, Policies>::type converter_policy_ret;
	typename converter_policy_ret::template generate_converter<T, cpp_to_lua>::type converter_ret;
	
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
		converter_ret.apply(L, f
				(
				 BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, _)
				));
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, _)

	int nret = lua_gettop(L) - nargs;

	const int indices[] =
	{
		-1		/* self */,
		nargs + nret	/* result*/
		BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, _)
	};

	policy_list_postcall<Policies>::apply(L, indices);

	return nret;
}



#elif BOOST_PP_ITERATION_FLAGS() == 3

template<class Policies BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
static int call(void(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies*)
{
	int nargs = lua_gettop(L);

	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_DECL, _)
		f
		(
		 BOOST_PP_ENUM(BOOST_PP_ITERATION(), LUABIND_PARAMS, _)
		);
	BOOST_PP_REPEAT(BOOST_PP_ITERATION(), LUABIND_POSTCALL, _)

	int nret = lua_gettop(L) - nargs;

	const int indices[] =
	{
		-1		/* self */,
		nargs + nret	/* result*/
		BOOST_PP_ENUM_TRAILING(BOOST_PP_ITERATION(), LUABIND_INDEX_MAP, _)
	};

	policy_list_postcall<Policies>::apply(L, indices);

	return nret;
}


#elif BOOST_PP_ITERATION_FLAGS() == 4

template<class Policies, class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
int call(R(*f)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies* policies)
{
	return free_functions::returns<R>::call(f, L, policies);
}

#elif BOOST_PP_ITERATION_FLAGS() == 5

	template<class Policies, class R BOOST_PP_COMMA_IF(BOOST_PP_ITERATION()) BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), class A)>
	static int match(R(*)(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)), lua_State* L, const Policies* policies)
	{
		//if (lua_gettop(L) != BOOST_PP_ITERATION()) return -1;
		typedef constructor<BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), A)> ParameterTypes;
		return match_params(L, 1, static_cast<ParameterTypes*>(0), policies);
	}



#endif

