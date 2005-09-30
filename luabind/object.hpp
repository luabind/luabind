// Copyright (c) 2005 Daniel Wallin and Arvid Norberg

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

#ifndef LUABIND_OBJECT_050419_HPP
#define LUABIND_OBJECT_050419_HPP

#include <boost/implicit_cast.hpp> // detail::push()
#include <boost/ref.hpp> // detail::push()
#include <boost/mpl/bool.hpp> // value_wrapper_traits specializations
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

#include <luabind/value_wrapper.hpp>
#include <luabind/detail/pcall.hpp>
#include <luabind/handle.hpp>
#include <luabind/from_stack.hpp>
#include <luabind/detail/policy.hpp>
#include <luabind/detail/stack_utils.hpp>
#include <luabind/detail/convert_to_lua.hpp> // REFACTOR

#include <boost/iterator/iterator_facade.hpp> // iterator
#include <boost/python/detail/is_xxx.hpp>

#include <boost/preprocessor/iteration/iterate.hpp>

namespace luabind {

namespace detail 
{

  template<class T, class ConverterGenerator>
  void push_aux(lua_State* interpreter, T& value, ConverterGenerator*)
  {
      typedef typename boost::mpl::if_<
          boost::is_reference_wrapper<T>
        , typename boost::unwrap_reference<T>::type&
        , T
      >::type unwrapped_type;

      typename ConverterGenerator::template generate_converter<
          unwrapped_type
        , cpp_to_lua
      >::type cv;

      cv.apply(
          interpreter
        , boost::implicit_cast<typename boost::unwrap_reference<T>::type&>(value)
      );
  }

  template<class T, class Policies>
  void push(lua_State* interpreter, T& value, Policies const&)
  {
      typedef typename find_conversion_policy<
          0
        , Policies
      >::type converter_policy;

      push_aux(interpreter, value, (converter_policy*)0);
  }

  template<class T>
  void push(lua_State* interpreter, T& value)
  {
      push(interpreter, value, null_type());
  }

} // namespace detail

namespace adl
{

  template<class T, class U, class V>
  lua_State* binary_interpreter(T const& x, U const&, boost::mpl::true_, V)
  {
      return value_wrapper_traits<T>::interpreter(x);
  }

  template<class T, class U>
  lua_State* binary_interpreter(T const&, U const& x, boost::mpl::false_, boost::mpl::true_)
  {
      return value_wrapper_traits<U>::interpreter(x);
  }

  template<class T, class U>
  lua_State* binary_interpreter(T const& x, U const& y)
  {
      return binary_interpreter(
          x
        , y
        , is_value_wrapper<T>()
        , is_value_wrapper<U>()
      );
  }

#define LUABIND_BINARY_OP_DEF(op, fn) \
  template<class LHS, class RHS> \
  bool operator op(LHS const& lhs, RHS const& rhs) \
  { \
      lua_State* L = binary_interpreter(lhs, rhs); \
\
      assert(L); \
\
      detail::stack_pop pop1(L, 1); \
      detail::push(L, lhs); \
      detail::stack_pop pop2(L, 1); \
      detail::push(L, rhs); \
\
      return fn(L, -1, -2) != 0; \
  }

  LUABIND_BINARY_OP_DEF(==, lua_equal)
  LUABIND_BINARY_OP_DEF(<, lua_lessthan)

#undef LUABIND_BINARY_OP_DEF

  template<class LHS, class RHS>
  bool operator>(LHS const& lhs, RHS const& rhs)
  {
      return !(lhs < rhs || lhs == rhs);
  }

  template<class LHS, class RHS>
  bool operator<=(LHS const& lhs, RHS const& rhs)
  {
      return lhs < rhs || lhs == rhs;
  }

  template<class LHS, class RHS>
  bool operator>=(LHS const& lhs, RHS const& rhs)
  {
      return !(lhs < rhs);
  }

  template<class LHS, class RHS>
  bool operator!=(LHS const& lhs, RHS const& rhs)
  {
      return !(lhs < rhs);
  }

  template<class ValueWrapper, class Arguments>
  struct call_proxy;

  template<class Next>
  class index_proxy;

  class object;
  
  template<class Derived>
  class object_interface
  {
  public:
      ~object_interface() {}

      call_proxy<Derived, boost::tuples::tuple<> > operator()();

      template<class A0>
      call_proxy<
          Derived
        , boost::tuples::tuple<A0 const*>
      > operator()(A0 const& a0)
      {
          typedef boost::tuples::tuple<A0 const*> arguments;

          return call_proxy<Derived, arguments>(
              derived()
            , arguments(&a0)
          );
      }

      template<class A0, class A1>
      call_proxy<
          Derived
        , boost::tuples::tuple<A0 const*, A1 const*>
      > operator()(A0 const& a0, A1 const& a1)
      {
          typedef boost::tuples::tuple<A0 const*, A1 const*> arguments;

          return call_proxy<object, arguments>(
              derived()
            , arguments(&a0, &a1)
          );
      }

      // The rest of the overloads are PP-generated.
      #define BOOST_PP_ITERATION_PARAMS_1 (3, \
          (3, LUABIND_MAX_ARITY, <luabind/detail/object_call.hpp>))
      #include BOOST_PP_ITERATE()

  private:
      Derived& derived()
      {
          return *static_cast<Derived*>(this);
      }

      Derived const& derived() const
      {
          return *static_cast<Derived const*>(this);
      }
  };

#ifdef LUABIND_USE_VALUE_WRAPPER_TAG
  struct iterator_proxy_tag;
#endif
  
  template<class AccessPolicy>
  class iterator_proxy
    : public object_interface<iterator_proxy<AccessPolicy> >
  {
  public:
#ifdef LUABIND_USE_VALUE_WRAPPER_TAG
      typedef iterator_proxy_tag value_wrapper_tag;
#endif

      iterator_proxy(lua_State* interpreter, handle const& table, handle const& key)
        : m_interpreter(interpreter)
        , m_table_index(lua_gettop(interpreter) + 1)
        , m_key_index(m_table_index + 1)
      {
          table.push(m_interpreter);
          key.push(m_interpreter);
      }

      iterator_proxy(iterator_proxy const& other)
        : m_interpreter(other.m_interpreter)
        , m_table_index(other.m_table_index)
        , m_key_index(other.m_key_index)
      {
          other.m_interpreter = 0;
      }

      ~iterator_proxy()
      {
          if (m_interpreter)
              lua_pop(m_interpreter, 2);
      }

      template<class T>
      iterator_proxy& operator=(T const& value)
      {
          lua_pushvalue(m_interpreter, m_key_index);
          detail::push(m_interpreter, value);
          AccessPolicy::set(m_interpreter, m_table_index);
          return *this;
      }

      template<class Key>
      index_proxy<iterator_proxy<AccessPolicy> > operator[](Key const& key)
      {
          return index_proxy<iterator_proxy<AccessPolicy> >(
              *this, m_interpreter, key
          );
      }

      // This is non-const to prevent conversion on lvalues.
      operator object();

      lua_State* interpreter() const
      {
          return m_interpreter;
      }

      // TODO: Why is it non-const?
      void push(lua_State* interpreter)
      {
          assert(interpreter == m_interpreter);
          lua_pushvalue(m_interpreter, m_key_index);
          AccessPolicy::get(m_interpreter, m_table_index);
      }

  private:
      mutable lua_State* m_interpreter;
      int m_table_index;
      int m_key_index;
  };

} // namespace adl

namespace detail
{
  struct basic_access
  {
      static void set(lua_State* interpreter, int table)
      {
          lua_settable(interpreter, table);
      }

      static void get(lua_State* interpreter, int table)
      {
          lua_gettable(interpreter, table);
      }
  };

  struct raw_access
  {
      static void set(lua_State* interpreter, int table)
      {
          lua_rawset(interpreter, table);
      }

      static void get(lua_State* interpreter, int table)
      {
          lua_rawget(interpreter, table);
      }
  };

  template<class AccessPolicy>
  class basic_iterator 
    : public boost::iterator_facade<
        basic_iterator<AccessPolicy>
      , adl::iterator_proxy<AccessPolicy>
      , boost::single_pass_traversal_tag
      , adl::iterator_proxy<AccessPolicy>
    >
  {
  public:
      basic_iterator()
        : m_interpreter(0)
      {}

      template<class ValueWrapper>
      explicit basic_iterator(ValueWrapper const& value_wrapper)
        : m_interpreter(
              value_wrapper_traits<ValueWrapper>::interpreter(value_wrapper)
          )
      {
          detail::stack_pop pop(m_interpreter, 1);
          value_wrapper_traits<ValueWrapper>::unwrap(m_interpreter, value_wrapper);

          lua_pushnil(m_interpreter);
          if (lua_next(m_interpreter, -2) != 0)
          {
              detail::stack_pop pop(m_interpreter, 2);
              handle(m_interpreter, -2).swap(m_key);
          }
          else
          {
              m_interpreter = 0;
              return;
          }

          handle(m_interpreter, -1).swap(m_table);
      }

      adl::object key() const;

  private:
      friend class boost::iterator_core_access;

      void increment()
      {
          m_table.push(m_interpreter);
          m_key.push(m_interpreter);

          detail::stack_pop pop(m_interpreter, 1);

          if (lua_next(m_interpreter, -2) != 0)
          {
              m_key.replace(m_interpreter, -2);
              lua_pop(m_interpreter, 2);
          }
          else
          {
              m_interpreter = 0;
              handle().swap(m_table);
              handle().swap(m_key);
          }
      }

      bool equal(basic_iterator const& other) const
      {
          if (m_interpreter == 0 && other.m_interpreter == 0)
              return true;

          if (m_interpreter != other.m_interpreter)
              return false;

          detail::stack_pop pop(m_interpreter, 2);
          m_key.push(m_interpreter);
          other.m_key.push(m_interpreter);
          return lua_equal(m_interpreter, -2, -1) != 0;
      }

      adl::iterator_proxy<AccessPolicy> dereference() const 
      {
          return adl::iterator_proxy<AccessPolicy>(m_interpreter, m_table, m_key);
      }

      lua_State* m_interpreter;
      handle m_table;
      handle m_key;
  };

#if defined(__GNUC__) || BOOST_WORKAROUND(BOOST_MSVC, == 1300)
  // Needed because of GCC's and VC7's strange ADL

# define LUABIND_OPERATOR_ADL_WKND(op) \
  inline bool operator op( \
      basic_iterator<basic_access> const& x \
    , basic_iterator<basic_access> const& y) \
  { \
      return boost::operator op(x, y); \
  } \
 \
  inline bool operator op( \
      basic_iterator<raw_access> const& x \
    , basic_iterator<raw_access> const& y) \
  { \
      return boost::operator op(x, y); \
  }

  LUABIND_OPERATOR_ADL_WKND(==)
  LUABIND_OPERATOR_ADL_WKND(!=)

# undef LUABIND_OPERATOR_ADL_WKND

#endif
  
} // namespace detail

namespace adl
{
 
#ifdef LUABIND_USE_VALUE_WRAPPER_TAG
  struct index_proxy_tag;    
#endif

  template<class Next>
  class index_proxy
    : public object_interface<index_proxy<Next> >
  {
  public:
#ifdef LUABIND_USE_VALUE_WRAPPER_TAG
      typedef index_proxy_tag value_wrapper_tag;
#endif

      typedef index_proxy<Next> this_type;

      template<class Key>
      index_proxy(Next const& next, lua_State* interpreter, Key const& key)
        : m_interpreter(interpreter)
        , m_key_index(lua_gettop(interpreter) + 1)
        , m_next(next)
      {
          detail::push(m_interpreter, key);
      }

      index_proxy(index_proxy const& other)
        : m_interpreter(other.m_interpreter)
        , m_key_index(other.m_key_index)
        , m_next(other.m_next)
      {
          other.m_interpreter = 0;
      }

      ~index_proxy()
      {
          if (m_interpreter)
              lua_pop(m_interpreter, 1);
      }

      // This is non-const to prevent conversion on lvalues.
      operator object();

      template<class T>
      this_type& operator=(T const& value)
      {
          value_wrapper_traits<Next>::unwrap(m_interpreter, m_next);

          detail::stack_pop pop(m_interpreter, 1);

          lua_pushvalue(m_interpreter, m_key_index);
          detail::push(m_interpreter, value);
          lua_settable(m_interpreter, -3);
          return *this;
      }

      template<class T>
      index_proxy<this_type> operator[](T const& key)
      {
          return index_proxy<this_type>(*this, m_interpreter, key);
      }

      void push(lua_State* interpreter);

      lua_State* interpreter() const
      {
          return m_interpreter;
      }

  private:
      this_type& operator=(index_proxy<Next> const&);

      mutable lua_State* m_interpreter;
      int m_key_index;

      Next const& m_next;
  };

} // namespace adl

typedef detail::basic_iterator<detail::basic_access> iterator;
typedef detail::basic_iterator<detail::raw_access> raw_iterator;

#ifndef LUABIND_USE_VALUE_WRAPPER_TAG
template<class T>
struct value_wrapper_traits<adl::index_proxy<T> >
#else
template<>
struct value_wrapper_traits<adl::index_proxy_tag>
#endif
{
    typedef boost::mpl::true_ is_specialized;

    template<class Next>
    static lua_State* interpreter(adl::index_proxy<Next> const& proxy)
    {
        return proxy.interpreter();
    }

    template<class Next>
    static void unwrap(lua_State* interpreter, adl::index_proxy<Next> const& proxy)
    {
        const_cast<adl::index_proxy<Next>&>(proxy).push(interpreter);
    }
};

#ifndef LUABIND_USE_VALUE_WRAPPER_TAG
template<class AccessPolicy>
struct value_wrapper_traits<adl::iterator_proxy<AccessPolicy> >
#else
template<>
struct value_wrapper_traits<adl::iterator_proxy_tag>
#endif
{
    typedef boost::mpl::true_ is_specialized;

    template<class Proxy>
    static lua_State* interpreter(Proxy const& p)
    {
        return p.interpreter();
    }

    template<class Proxy>
    static void unwrap(lua_State* interpreter, Proxy const& p)
    {
        // TODO: Why const_cast?
        const_cast<Proxy&>(p).push(interpreter);
    }
};

namespace adl
{
  // An object holds a reference to a Lua value residing
  // in the registry.
  class object : public object_interface<object>
  {
      struct safe_bool_type {};
  public:
      object()
      {}

      explicit object(from_stack const& stack_reference)
        : m_handle(stack_reference.interpreter, stack_reference.index)
      {
      }

      template<class ValueWrapper>
      explicit object(ValueWrapper const& value_wrapper)
      {
          lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
              value_wrapper
          );

          value_wrapper_traits<ValueWrapper>::unwrap(interpreter, value_wrapper);
          detail::stack_pop pop(interpreter, 1);

          handle(interpreter, -1).swap(m_handle);
      }

      template<class T>
      object(lua_State* interpreter, T const& value)
      {
          detail::push(interpreter, value);
          detail::stack_pop pop(interpreter, 1);
          handle(interpreter, -1).swap(m_handle);
      }

      template<class T, class Policies>
      object(lua_State* interpreter, T const& value, Policies const&)
      {
          detail::push(interpreter, value, Policies());
          detail::stack_pop pop(interpreter, 1);
          handle(interpreter, -1).swap(m_handle);
      }

      void push(lua_State* interpreter) const;
      lua_State* interpreter() const;
      bool is_valid() const;
      operator safe_bool_type*() const;

      template<class T>
      index_proxy<object> operator[](T const& key) const
      {
          return index_proxy<object>(
              *this, m_handle.interpreter(), key
          );
      }

      void swap(object& other)
      {
          m_handle.swap(other.m_handle);
      }

  private:
      handle m_handle;
  };

  inline void object::push(lua_State* interpreter) const
  {
      m_handle.push(interpreter);
  }

  inline lua_State* object::interpreter() const
  {
      return m_handle.interpreter();
  }

  inline bool object::is_valid() const
  {
      return m_handle.interpreter() != 0;
  }

  inline object::operator object::safe_bool_type*() const
  {
      return is_valid()?(safe_bool_type*)1:0;
  }

} // namespace adl

using adl::object;

template<>
struct value_wrapper_traits<object>
{
    typedef boost::mpl::true_ is_specialized;

    static lua_State* interpreter(object const& value)
    {
        return value.interpreter();
    }

    static void unwrap(lua_State* interpreter, object const& value)
    {
        value.push(interpreter);
    }

    static bool check(...)
    {
        return true;
    }
};

template<class Next>
inline void adl::index_proxy<Next>::push(lua_State* interpreter)
{
    assert(interpreter == m_interpreter);

    value_wrapper_traits<Next>::unwrap(m_interpreter, m_next);

    lua_pushvalue(m_interpreter, m_key_index);
    lua_gettable(m_interpreter, -2);
    lua_remove(m_interpreter, -2);
}

template<class Next>
inline adl::index_proxy<Next>::operator object()
{
    detail::stack_pop pop(m_interpreter, 1);
    push(m_interpreter);
    return object(from_stack(m_interpreter, -1));
}

template<class AccessPolicy>
adl::iterator_proxy<AccessPolicy>::operator object()
{
    lua_pushvalue(m_interpreter, m_key_index);
    AccessPolicy::get(m_interpreter, m_table_index);
    detail::stack_pop pop(m_interpreter, 1);
    return object(from_stack(m_interpreter, -1));
}

template<class AccessPolicy>
object detail::basic_iterator<AccessPolicy>::key() const
{
    return object(m_key);
}

namespace detail 
{

  template<
      class T
    , class ValueWrapper
    , class Policies
    , class ErrorPolicy
  >
  typename ErrorPolicy::type object_cast_aux(
      ValueWrapper const& value_wrapper
    , T*
    , Policies*
    , ErrorPolicy*
  )
  {
      lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
          value_wrapper
      );

#ifndef LUABIND_NO_ERROR_CHECKING
      if (!interpreter) 
          return ErrorPolicy::handle_error(interpreter, LUABIND_TYPEID(void));
#endif

      value_wrapper_traits<ValueWrapper>::unwrap(interpreter, value_wrapper);

      detail::stack_pop pop(interpreter, 1);

      typedef typename detail::find_conversion_policy<
          0
        , Policies
      >::type converter_generator;

      typename converter_generator::template generate_converter<
          T
        , lua_to_cpp
      >::type cv;

#ifndef LUABIND_NO_ERROR_CHECKING
      if (cv.match(interpreter, LUABIND_DECORATE_TYPE(T), -1) < 0)
      {
          return ErrorPolicy::handle_error(interpreter, LUABIND_TYPEID(T));
      }
#endif

      return cv.apply(interpreter, LUABIND_DECORATE_TYPE(T), -1);
  }

  template<class T>
  struct throw_error_policy
  {
      typedef T type;

      static T handle_error(lua_State* interpreter, LUABIND_TYPE_INFO type_info)
      {
#ifndef LUABIND_NO_EXCEPTIONS
          throw cast_failed(interpreter, type_info);
#else
          cast_failed_callback_fun e = get_cast_failed_callback();
          if (e) e(interpreter, type_info);

          assert(0 && "object_cast failed. If you want to handle this error use "
              "luabind::set_error_callback()");
          std::terminate();
#endif
      }
  };

  template<class T>
  struct nothrow_error_policy
  {
      typedef boost::optional<T> type;

      static type handle_error(lua_State*, LUABIND_TYPE_INFO)
      {
          return type();
      }
  };

} // namespace detail

template<class T, class ValueWrapper>
T object_cast(ValueWrapper const& value_wrapper)
{
    return detail::object_cast_aux(
        value_wrapper
      , (T*)0
      , (detail::null_type*)0
      , (detail::throw_error_policy<T>*)0
    );
}

template<class T, class ValueWrapper, class Policies>
T object_cast(ValueWrapper const& value_wrapper, Policies const&)
{
    return detail::object_cast_aux(
        value_wrapper
      , (T*)0
      , (Policies*)0
      , (detail::throw_error_policy<T>*)0
    );
}

template<class T, class ValueWrapper>
boost::optional<T> object_cast_nothrow(ValueWrapper const& value_wrapper)
{
    return detail::object_cast_aux(
        value_wrapper
      , (T*)0
      , (detail::null_type*)0
      , (detail::nothrow_error_policy<T>*)0
    );
}

template<class T, class ValueWrapper, class Policies>
boost::optional<T> object_cast_nothrow(ValueWrapper const& value_wrapper, Policies const&)
{
    return detail::object_cast_aux(
        value_wrapper
      , (T*)0
      , (Policies*)0
      , (detail::nothrow_error_policy<T>*)0
    );
}

namespace detail
{

  template<class T>
  void convert_to_lua(lua_State*, const T&);

  template<int Index, class T, class Policies>
  void convert_to_lua_p(lua_State*, const T&, const Policies&);

  template<int Index>
  struct push_args_from_tuple
  {
      template<class H, class T, class Policies>
      inline static void apply(lua_State* L, const boost::tuples::cons<H, T>& x, const Policies& p) 
      {
          convert_to_lua_p<Index>(L, *x.get_head(), p);
          push_args_from_tuple<Index+1>::apply(L, x.get_tail(), p);
      }

      template<class H, class T>
      inline static void apply(lua_State* L, const boost::tuples::cons<H, T>& x) 
      {
          convert_to_lua(L, *x.get_head());
          push_args_from_tuple<Index+1>::apply(L, x.get_tail());
      }

      template<class Policies>
      inline static void apply(lua_State*, const boost::tuples::null_type&, const Policies&) {};

      inline static void apply(lua_State*, const boost::tuples::null_type&) {};
  };

} // namespace detail

namespace adl
{

  template<class ValueWrapper, class Arguments>
  struct call_proxy
  {
      call_proxy(ValueWrapper& value_wrapper, Arguments arguments)
        : value_wrapper(&value_wrapper)
        , arguments(arguments)
      {}

      call_proxy(call_proxy const& other)
        : value_wrapper(other.value_wrapper)
        , arguments(other.arguments)
      {
          other.value_wrapper = 0;
      }

      ~call_proxy()
      {
          if (value_wrapper)
              call((detail::null_type*)0);
      }

      operator object()
      {
          return call((detail::null_type*)0);
      }

      template<class Policies>
      object operator[](Policies const&)
      {
          return call((Policies*)0);
      }

      template<class Policies>
      object call(Policies*)
      {
          lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
              *value_wrapper
          );

          value_wrapper_traits<ValueWrapper>::unwrap(
              interpreter
            , *value_wrapper
          );

          value_wrapper = 0;

          detail::push_args_from_tuple<1>::apply(interpreter, arguments, Policies());

          if (detail::pcall(interpreter, boost::tuples::length<Arguments>::value, 1))
          {
#ifndef LUABIND_NO_EXCEPTIONS
              throw luabind::error(interpreter);
#else
              error_callback_fun e = get_error_callback();
              if (e) e(interpreter);

              assert(0 && "the lua function threw an error and exceptions are disabled."
                  "if you want to handle this error use luabind::set_error_callback()");
              std::terminate();
#endif
          }

          detail::stack_pop pop(interpreter, 1);
          return object(from_stack(interpreter, -1));
      }

      mutable ValueWrapper* value_wrapper;
      Arguments arguments;
  };

  template<class Derived>
  call_proxy<Derived, boost::tuples::tuple<> >
  object_interface<Derived>::operator()()
  {
      return call_proxy<Derived, boost::tuples::tuple<> >(
          derived()
        , boost::tuples::tuple<>()
      );
  }

} // namespace adl

inline object newtable(lua_State* interpreter)
{
    lua_newtable(interpreter);
    detail::stack_pop pop(interpreter, 1);
    return object(from_stack(interpreter, -1));
}

inline object globals(lua_State* interpreter)
{
    lua_pushvalue(interpreter, LUA_GLOBALSINDEX);
    detail::stack_pop pop(interpreter, 1);
    return object(from_stack(interpreter, -1));
}

template<class ValueWrapper, class K>
inline object gettable(ValueWrapper const& table, K const& key)
{
    lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
        table
    );

    value_wrapper_traits<ValueWrapper>::unwrap(interpreter, table);
    detail::stack_pop pop(interpreter, 2);
    detail::push(interpreter, key);
    lua_gettable(interpreter, -2);
    return object(from_stack(interpreter, -1));
}

template<class ValueWrapper, class K, class T>
inline void settable(ValueWrapper const& table, K const& key, T const& value)
{
    lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
        table
    );

    // TODO: Exception safe?
    
    value_wrapper_traits<ValueWrapper>::unwrap(interpreter, table);
    detail::stack_pop pop(interpreter, 1);
    detail::push(interpreter, key);
    detail::push(interpreter, value);
    lua_settable(interpreter, -3);
}

template<class ValueWrapper, class K>
inline object rawget(ValueWrapper const& table, K const& key)
{
    lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
        table
    );

    value_wrapper_traits<ValueWrapper>::unwrap(interpreter, table);
    detail::stack_pop pop(interpreter, 2);
    detail::push(interpreter, key);
    lua_rawget(interpreter, -2);
    return object(from_stack(interpreter, -1));
}

template<class ValueWrapper, class K, class T>
inline void rawset(ValueWrapper const& table, K const& key, T const& value)
{
    lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
        table
    );

    // TODO: Exception safe?
    
    value_wrapper_traits<ValueWrapper>::unwrap(interpreter, table);
    detail::stack_pop pop(interpreter, 1);
    detail::push(interpreter, key);
    detail::push(interpreter, value);
    lua_rawset(interpreter, -3);
}

template<class ValueWrapper>
inline int type(ValueWrapper const& value)
{
    lua_State* interpreter = value_wrapper_traits<ValueWrapper>::interpreter(
        value
    );

    value_wrapper_traits<ValueWrapper>::unwrap(interpreter, value);
    detail::stack_pop pop(interpreter, 1);
    return lua_type(interpreter, -1);
}

} // namespace luabind

#endif // LUABIND_OBJECT_050419_HPP

