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


#ifndef LUABIND_POLICY_HPP_INCLUDED
#define LUABIND_POLICY_HPP_INCLUDED

#include <luabind/config.hpp>

#include <typeinfo>
#include <string>
#include <memory>

#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/bind/arg.hpp>
#include <boost/limits.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/version.hpp>

#include <luabind/detail/class_registry.hpp>
#include <luabind/detail/primitives.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/typetraits.hpp>
#include <luabind/detail/class_cache.hpp>
#include <luabind/detail/debug.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/conversion_storage.hpp>
#include <luabind/detail/has_get_pointer.hpp>

#include <boost/type_traits/add_reference.hpp>

#include <luabind/detail/decorate_type.hpp>
#include <luabind/weak_ref.hpp>
#include <luabind/back_reference_fwd.hpp>

#include <luabind/value_wrapper.hpp>
#include <luabind/from_stack.hpp>
#include <luabind/typeid.hpp>

namespace luabind
{
	namespace detail
	{
		struct conversion_policy_base {};
	}

	template<int N, bool HasArg = true>
	struct conversion_policy : detail::conversion_policy_base
	{
		BOOST_STATIC_CONSTANT(int, index = N);
		BOOST_STATIC_CONSTANT(bool, has_arg = HasArg);
	};

	class index_map
	{
	public:
		index_map(const int* m): m_map(m) {}

		int operator[](int index) const
		{
			return m_map[index];
		}

	private:
		const int* m_map;
	};

	namespace detail
	{

		LUABIND_API int implicit_cast(const class_rep* crep, type_id const&, int& pointer_offset);
	}

//	 template<class T> class functor;
	 class weak_ref;
}

namespace luabind { namespace detail
{
	template<class H, class T>
	struct policy_cons
	{
		typedef H head;
		typedef T tail;

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator,(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator+(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}

		template<class U>
		policy_cons<U, policy_cons<H,T> > operator|(policy_cons<U,detail::null_type>)
		{
			return policy_cons<U, policy_cons<H,T> >();
		}
	};

	struct indirection_layer
	{
		template<class T>
		indirection_layer(const T&);
	};

	yes_t is_policy_cons_test(const null_type&);
	template<class H, class T>
	yes_t is_policy_cons_test(const policy_cons<H,T>&);
	no_t is_policy_cons_test(...);

	template<class T>
	struct is_policy_cons
	{
		static const T& t;

		BOOST_STATIC_CONSTANT(bool, value = 
			sizeof(is_policy_cons_test(t)) == sizeof(yes_t));

		typedef boost::mpl::bool_<value> type;
	};	

	template<bool>
	struct is_string_literal
	{
		static no_t helper(indirection_layer);
		static yes_t helper(const char*);
	};

	template<>
	struct is_string_literal<false>
	{
		static no_t helper(indirection_layer);
	};
	

    namespace mpl = boost::mpl;

// ********** pointer converter ***********

    template <class P>
    void install_instance(P ptr, object_rep& instance)
    {
        typedef pointer_holder<P> holder_type;

        void* storage = instance.allocate(sizeof(holder_type));

        try
        {
            new (storage) holder_type(ptr, instance.crep());
        }
        catch (...)
        {
            instance.deallocate(storage);
            throw;
        }

        instance.set_instance((holder_type*)storage);
    }

	struct pointer_converter
	{
		typedef pointer_converter type;
        typedef mpl::false_ is_native;

        pointer_converter()
          : result(0)
        {}

        void* result;

        int const consumed_args(...)
        {
            return 1;
        }

		template<class T>
		void apply(lua_State* L, T* ptr)
		{
			if (ptr == 0) 
			{
				lua_pushnil(L);
				return;
			}

			if (luabind::get_back_reference(L, ptr))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

            object_rep* instance = push_new_instance(L, crep);
            install_instance(ptr, *instance);
		}

		conversion_storage storage;

		template<class T>
		T* apply(lua_State* L, by_pointer<T>, int index)
		{
            return static_cast<T*>(result);
		}

		template<class T>
		int match(lua_State* L, by_pointer<T>, int index)
		{
			if (lua_isnil(L, index)) return 0;
			object_rep* obj = get_instance(L, index);
			if (obj == 0) return -1;

            if (obj->is_const())
                return -1;

            std::pair<void*, int> s = obj->get_instance(typeid(T));
            result = s.first;
            return s.second;
		}

		template<class T>
		void converter_postcall(lua_State*, by_pointer<T>, int) 
		{}
	};

// ******* value converter *******

	struct value_converter
	{
		typedef value_converter type;
        typedef mpl::false_ is_native;

        template <class T>
        void install(T& x, object_rep& instance, mpl::false_)
        {
            std::auto_ptr<T> ptr(new T(x));
            install_instance(ptr, instance);
        }

        template <class T>
        void install(T& x, object_rep& instance, mpl::true_)
        {
            install_instance(x, instance);
        }

		template <class T>
		class_rep* get_class(T const*, lua_State* L, mpl::false_)
		{
			return get_class_rep<T>(L);
		}

		template <class T>
		class_rep* get_class(T const* x, lua_State* L, mpl::true_)
		{
			return get_class(get_pointer(*x), L, mpl::false_());
		}

        int const consumed_args(...)
        {
            return 1;
        }

		template<class T>
		void apply(lua_State* L, T x)
		{
			if (luabind::get_back_reference(L, x))
				return;

			class_rep* crep = get_class(&x, L, has_get_pointer<T>());

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

            object_rep* instance = push_new_instance(L, crep);
            install(x, *instance, has_get_pointer<T>());
		}

        conversion_storage storage;

		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
			// preconditions:
			//	lua_isuserdata(L, index);
			// getmetatable().__lua_class is true

			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, index));
			assert((obj != 0) && "internal error, please report"); // internal error

			return *static_cast<T*>(obj->get_instance(typeid(T)).first);
		}

		template<class T>
		static int match(lua_State* L, by_value<T>, int index)
		{
			// special case if we get nil in, try to match the holder type
			if (lua_isnil(L, index))
				return -1;

			object_rep* obj = get_instance(L, index);
			if (obj == 0) return -1;

			return obj->get_instance(typeid(T)).second;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

// ******* const pointer converter *******

    struct const_pointer_converter : pointer_converter
	{
		typedef const_pointer_converter type;

		template<class T>
		void apply(lua_State* L, const T* ptr)
		{
			if (ptr == 0) 
			{
				lua_pushnil(L);
				return;
			}

			if (luabind::get_back_reference(L, ptr))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

            object_rep* instance = push_new_instance(L, crep);
            install_instance(ptr, *instance);
		}

		template<class T>
		T const* apply(lua_State* L, by_const_pointer<T>, int index)
		{
			return pointer_converter::apply(L, by_pointer<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_const_pointer<T>, int index)
		{
			if (lua_isnil(L, index)) return 0;
			object_rep* obj = get_instance(L, index);
			if (obj == 0) return -1; // if the type is not one of our own registered types, classify it as a non-match
			int score = obj->get_instance(typeid(T)).second;

            if (score >= 0 && !obj->is_const())
                score += 10;

            return score;
		}

		template<class T>
		void converter_postcall(lua_State* L, by_const_pointer<T>, int index) 
		{
			pointer_converter::converter_postcall(L, by_pointer<T>(), index);
		}
	};

// ******* reference converter *******

    struct ref_converter : pointer_converter
	{
		typedef ref_converter type;
        typedef mpl::false_ is_native;

        int const consumed_args(...)
        {
            return 1;
        }

		template<class T>
		void apply(lua_State* L, T& ref)
		{
			if (luabind::get_back_reference(L, ref))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

            object_rep* instance = push_new_instance(L, crep);
            install_instance(&ref, *instance);
		}

		template<class T>
		T& apply(lua_State* L, by_reference<T>, int index)
		{
			assert(!lua_isnil(L, index));
            return *pointer_converter::apply(L, by_pointer<T>(), index);
		}

		template<class T>
		int match(lua_State* L, by_reference<T>, int index)
		{
			if (lua_isnil(L, index)) return -1;
			return pointer_converter::match(L, by_pointer<T>(), index);
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

// ******** const reference converter *********

	struct const_ref_converter
	{
		typedef const_ref_converter type;
        typedef mpl::false_ is_native;

        int const consumed_args(...)
        {
            return 1;
        }

        const_ref_converter()
          : result(0)
        {}

        void* result;

		template<class T>
		void apply(lua_State* L, T const& ref)
		{
			if (luabind::get_back_reference(L, ref))
				return;

			class_rep* crep = get_class_rep<T>(L);

			// if you get caught in this assert you are
			// trying to use an unregistered type
			assert(crep && "you are trying to use an unregistered type");

			// create the struct to hold the object
            object_rep* instance = push_new_instance(L, crep);
            install_instance(&ref, *instance);
		}

		conversion_storage storage;

		template<class T>
		T const& apply(lua_State* L, by_const_reference<T>, int index)
		{
			return *static_cast<T*>(result);
		}

		template<class T>
		int match(lua_State* L, by_const_reference<T>, int index)
		{
			object_rep* obj = get_instance(L, index);
			if (obj == 0) return -1; // if the type is not one of our own registered types, classify it as a non-match

            std::pair<void*, int> s = obj->get_instance(typeid(T));
            if (s.second >= 0 && !obj->is_const())
                s.second += 10;
            result = s.first;
            return s.second;
		}

		template<class T>
		void converter_postcall(lua_State* L, by_const_reference<T>, int index) 
		{
		}
	};

	// ****** enum converter ********

	struct enum_converter
	{
		typedef enum_converter type;
        typedef mpl::true_ is_native;

        int const consumed_args(...)
        {
            return 1;
        }
		
		void apply(lua_State* L, int val)
		{
			lua_pushnumber(L, val);
		}

		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
			return static_cast<T>(static_cast<int>(lua_tonumber(L, index)));
		}
		
		template<class T>
		static int match(lua_State* L, by_value<T>, int index)
		{
			if (lua_isnumber(L, index)) return 0; else return -1;
		}

		template<class T>
		T apply(lua_State* L, by_const_reference<T>, int index)
		{
			return static_cast<T>(static_cast<int>(lua_tonumber(L, index)));
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<T>, int index)
		{
			if (lua_isnumber(L, index)) return 0; else return -1;
		}

		template<class T>
		void converter_postcall(lua_State*, T, int) {}
	};

	template <class U>
	struct value_wrapper_converter
	{
		typedef value_wrapper_converter<U> type;
		typedef mpl::true_ is_native;

        int const consumed_args(...)
        {
            return 1;
        }

		template<class T>
		T apply(lua_State* L, by_const_reference<T>, int index)
		{
			return T(from_stack(L, index));
		}

		template<class T>
		T apply(lua_State* L, by_value<T>, int index)
		{
			return apply(L, by_const_reference<T>(), index);
		}

		template<class T>
		static int match(lua_State* L, by_const_reference<T>, int index)
		{
			return value_wrapper_traits<T>::check(L, index) 
                ? (std::numeric_limits<int>::max)() / LUABIND_MAX_ARITY
                : -1;
		}

		template<class T>
		static int match(lua_State* L, by_value<T>, int index)
		{
			return match(L, by_const_reference<T>(), index);
		}

		void converter_postcall(...) {}

		template<class T>
		void apply(lua_State* interpreter, T const& value_wrapper)
		{
			value_wrapper_traits<T>::unwrap(interpreter, value_wrapper);
		}
    };

    template <class T>
    struct default_converter_generator
      : mpl::eval_if<
            is_value_wrapper_arg<T>
          , value_wrapper_converter<T>
          , mpl::eval_if<
                boost::is_enum<typename boost::remove_reference<T>::type>
              , enum_converter
              , mpl::eval_if<
                    is_nonconst_pointer<T>
                  , pointer_converter
                  , mpl::eval_if<
                        is_const_pointer<T>
                      , const_pointer_converter
                      , mpl::eval_if<
                            is_nonconst_reference<T>
                          , ref_converter
                          , mpl::eval_if<
                                is_const_reference<T>
                              , const_ref_converter
                              , value_converter
                            >
                        >
                    >
                >
            >
        >
    {};

} // namespace detail

// *********** default_policy *****************

template <class T>
struct default_converter
  : detail::default_converter_generator<T>::type
{};

template <class T, class Derived = default_converter<T> >
struct native_converter_base
{
    typedef boost::mpl::true_ is_native;

    int const consumed_args(...)
    {
        return 1;
    }

    template <class U>
    void converter_postcall(lua_State*, U const&, int)
    {}

    int match(lua_State* L, detail::by_value<T>, int index)
    {
        return derived().compute_score(L, index);
    }

    int match(lua_State* L, detail::by_value<T const>, int index)
    {
        return derived().compute_score(L, index);
    }

    int match(lua_State* L, detail::by_const_reference<T>, int index)
    {
        return derived().compute_score(L, index);
    }

    T apply(lua_State* L, detail::by_value<T>, int index)
    {
        return derived().from(L, index);
    }

    T apply(lua_State* L, detail::by_value<T const>, int index)
    {
        return derived().from(L, index);
    }

    T apply(lua_State* L, detail::by_const_reference<T>, int index)
    {
        return derived().from(L, index);
    }

    void apply(lua_State* L, T const& value)
    {
        derived().to(L, value);
    }

    Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }
};

# define LUABIND_NUMBER_CONVERTER(type) \
    template <> \
struct default_converter<type> \
  : native_converter_base<type> \
{ \
    lua_Number result; \
 \
    int compute_score(lua_State* L, int index) \
    { \
        result = lua_tonumber(L, index); \
        return (result != 0 || lua_isnumber(L, index)) ? 0 : -1; \
    }; \
    \
    type from(lua_State* L, int index) \
    { \
        return static_cast<type>(result); \
    } \
    \
    void to(lua_State* L, type const& value) \
    { \
        lua_pushnumber(L, static_cast<lua_Number>(value)); \
    } \
}; \
\
template <> \
struct default_converter<type const> \
  : default_converter<type> \
{}; \
\
template <> \
struct default_converter<type const&> \
  : default_converter<type> \
{};

LUABIND_NUMBER_CONVERTER(char)
LUABIND_NUMBER_CONVERTER(signed char)
LUABIND_NUMBER_CONVERTER(unsigned char)
LUABIND_NUMBER_CONVERTER(signed short)
LUABIND_NUMBER_CONVERTER(unsigned short)
LUABIND_NUMBER_CONVERTER(signed int)
LUABIND_NUMBER_CONVERTER(unsigned int)
LUABIND_NUMBER_CONVERTER(signed long)
LUABIND_NUMBER_CONVERTER(unsigned long)
LUABIND_NUMBER_CONVERTER(float)
LUABIND_NUMBER_CONVERTER(double)
LUABIND_NUMBER_CONVERTER(long double)

# undef LUABIND_NUMBER_CONVERTER

template <>
struct default_converter<bool>
  : native_converter_base<bool>
{
    static int compute_score(lua_State* L, int index)
    {
        return lua_type(L, index) == LUA_TBOOLEAN ? 0 : -1;
    }

    bool from(lua_State* L, int index)
    {
        return lua_toboolean(L, index) == 1;
    }

    void to(lua_State* L, bool value)
    {
        lua_pushboolean(L, value);
    }
};

template <>
struct default_converter<bool const>
  : default_converter<bool>
{};

template <>
struct default_converter<bool const&>
  : default_converter<bool>
{};

template <>
struct default_converter<std::string>
  : native_converter_base<std::string>
{
    static int compute_score(lua_State* L, int index)
    {
        return lua_type(L, index) == LUA_TSTRING ? 0 : -1;
    }

    std::string from(lua_State* L, int index)
    {
        return std::string(lua_tostring(L, index), lua_strlen(L, index));
    }

    void to(lua_State* L, std::string const& value)
    {
        lua_pushlstring(L, value.data(), value.size());
    }
};

template <>
struct default_converter<std::string const>
  : default_converter<std::string>
{};

template <>
struct default_converter<std::string const&>
  : default_converter<std::string>
{};

template <>
struct default_converter<char const*>
{
    typedef boost::mpl::true_ is_native;

    int const consumed_args(...)
    {
        return 1;
    }

    template <class U>
    static int match(lua_State* L, U, int index)
    {
        return lua_type(L, index) == LUA_TSTRING ? 0 : -1;
    }

    template <class U>
    char const* apply(lua_State* L, U, int index)
    {
        return lua_tostring(L, index);
    }

    void apply(lua_State* L, char const* str)
    {
        lua_pushstring(L, str);
    }

    template <class U>
    void converter_postcall(lua_State*, U, int)
    {}
};

template <>
struct default_converter<const char* const>
  : default_converter<char const*>
{};

template <>
struct default_converter<char*>
  : default_converter<char const*>
{};

template <std::size_t N>
struct default_converter<char const[N]>
  : default_converter<char const*>
{};

template <std::size_t N>
struct default_converter<char[N]>
  : default_converter<char const*>
{};

template <>
struct default_converter<lua_State*>
{
    int const consumed_args(...)
    {
        return 0;
    }

    template <class U>
    lua_State* apply(lua_State* L, U, int)
    {
        return L;
    }

    template <class U>
    static int match(lua_State*, U, int)
    {
        return 0;
    }

    template <class U>
    void converter_postcall(lua_State*, U, int) {}
};

namespace detail
{

	struct default_policy : converter_policy_tag
	{
		BOOST_STATIC_CONSTANT(bool, has_arg = true);

		template<class T>
		static void precall(lua_State*, T, int) {}

		template<class T, class Direction>
		struct apply
		{
            typedef default_converter<T> type;
        };
	};

    template<class T>
    struct is_primitive
      : default_converter<T>::is_native
    {};

// ============== new policy system =================

	template<int, class> struct find_conversion_policy;

	template<bool IsConverter = false>
	struct find_conversion_impl
	{
		template<int N, class Policies>
		struct apply
		{
			typedef typename find_conversion_policy<N, typename Policies::tail>::type type;
		};
	};

	template<>
	struct find_conversion_impl<true>
	{
		template<int N, class Policies>
		struct apply
		{
			typedef typename Policies::head head;
			typedef typename Policies::tail tail;

			BOOST_STATIC_CONSTANT(bool, found = (N == head::index));

			typedef typename
				boost::mpl::if_c<found
					, head
					, typename find_conversion_policy<N, tail>::type
				>::type type;
		};
	};

	template<class Policies>
	struct find_conversion_impl2
	{
		template<int N>
		struct apply
			: find_conversion_impl<
				boost::is_base_and_derived<conversion_policy_base, typename Policies::head>::value
			>::template apply<N, Policies>
		{
		};
	};

	template<>
	struct find_conversion_impl2<detail::null_type>
	{
		template<int N>
		struct apply
		{
			typedef default_policy type;
		};
	};

	template<int N, class Policies>
	struct find_conversion_policy : find_conversion_impl2<Policies>::template apply<N>
	{
	};

	template<class List>
	struct policy_list_postcall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, const index_map& i)
		{
			head::postcall(L, i);
			policy_list_postcall<tail>::apply(L, i);
		}
	};

	template<>
	struct policy_list_postcall<detail::null_type>
	{
		static void apply(lua_State*, const index_map&) {}
	};

// ==================================================

// ************** precall and postcall on policy_cons *********************


	template<class List>
	struct policy_precall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, int index)
		{
			head::precall(L, index);
			policy_precall<tail>::apply(L, index);
		}
	};

	template<>
	struct policy_precall<detail::null_type>
	{
		static void apply(lua_State*, int) {}
	};

	template<class List>
	struct policy_postcall
	{
		typedef typename List::head head;
		typedef typename List::tail tail;

		static void apply(lua_State* L, int index)
		{
			head::postcall(L, index);
			policy_postcall<tail>::apply(L, index);
		}
	};

	template<>
	struct policy_postcall<detail::null_type>
	{
		static void apply(lua_State*, int) {}
	};

}} // namespace luabind::detail


namespace luabind { namespace
{
#if defined(__GNUC__) && \
  (__GNUC__ * 100 + __GNUC_MINOR__ <= 400 || BOOST_VERSION <= 103401)
  static inline boost::arg<0> return_value()
  {
	  return boost::arg<0>();
  }

  static inline boost::arg<0> result()
  {
	  return boost::arg<0>();
  }
# define LUABIND_PLACEHOLDER_ARG(N) boost::arg<N>(*)()
#elif defined(BOOST_MSVC) || defined(__MWERKS__)
  static boost::arg<0> return_value;
  static boost::arg<0> result;
# define LUABIND_PLACEHOLDER_ARG(N) boost::arg<N>
#else
  boost::arg<0> return_value;
  boost::arg<0> result;
# define LUABIND_PLACEHOLDER_ARG(N) boost::arg<N>
#endif
}}

#endif // LUABIND_POLICY_HPP_INCLUDED

