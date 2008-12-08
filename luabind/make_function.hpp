// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_MAKE_FUNCTION_081014_HPP
# define LUABIND_MAKE_FUNCTION_081014_HPP

# include <luabind/config.hpp>
# include <luabind/object.hpp>
# include <luabind/detail/call.hpp>
# include <luabind/detail/compute_score.hpp>
# include <luabind/detail/deduce_signature.hpp>
# include <luabind/detail/format_signature.hpp>

# include <boost/function/function1.hpp>

namespace luabind {

namespace detail
{

  template <class F, class Signature, class Policies>
  struct function_invoke
  {
      function_invoke(F f)
        : f(f)
      {}

      int operator()(lua_State* L) const
      {
          return invoke(L, f, Signature(), Policies());
      }

      F f;
  };

  template <class Signature, class Policies>
  struct function_score
  {
      int operator()(lua_State* L) const
      {
          return compute_score(L, Signature(), Policies());
      }
  };

  template <class Signature>
  struct function_signature
  {
      int operator()(lua_State* L, char const* function) const
      {
          format_signature(L, function, Signature());
          return 0;
      }
  };

  typedef boost::function1<int, lua_State*> function_callback;
  typedef boost::function2<int, lua_State*, char const*> signature_callback;

  LUABIND_API object make_function_aux(
      lua_State*, int
    , function_callback const& call
    , function_callback const& score
    , signature_callback const& signature
  );

  LUABIND_API void add_overload(object const&, char const*, object const&);

} // namespace detail

template <class F, class Signature, class Policies>
object make_function(lua_State* L, F f, Signature, Policies)
{
    return detail::make_function_aux(
        L
      , detail::compute_arity(Signature(), Policies())
      , detail::function_invoke<F, Signature, Policies>(f)
      , detail::function_score<Signature, Policies>()
      , detail::function_signature<Signature>()
    );
}

template <class F>
object make_function(lua_State* L, F f)
{
    return make_function(L, detail::deduce_signature(f), detail::null_type());
}

} // namespace luabind

#endif // LUABIND_MAKE_FUNCTION_081014_HPP

