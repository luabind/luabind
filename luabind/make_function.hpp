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

namespace luabind {

namespace detail
{

  struct function_base
  {
      virtual ~function_base()
      {}

      virtual int call(lua_State*) const = 0;
      virtual int compute_score(lua_State*) const = 0;
      virtual void format_signature(lua_State*, char const*) const = 0;
  };

  template <class F, class Signature, class Policies>
  struct function_impl : function_base
  {
      function_impl(F f, Policies policies)
        : f(f)
        , policies(policies)
      {}

      int call(lua_State* L) const
      {
          return invoke(L, f, Signature(), policies);
      }

      int compute_score(lua_State* L) const
      {
          return detail::compute_score(L, Signature(), policies);
      }

      void format_signature(lua_State* L, char const* function) const
      {
          detail::format_signature(L, function, Signature());
      }

      F f;
      Policies policies;
  };

  LUABIND_API object make_function_aux(
      lua_State*, int, function_base*
  );

  LUABIND_API void add_overload(object const&, char const*, object const&);

} // namespace detail

template <class F, class Signature, class Policies>
object make_function(lua_State* L, F f, Signature, Policies)
{
    return detail::make_function_aux(
        L
      , detail::compute_arity(Signature(), Policies())
      , new detail::function_impl<F, Signature, Policies>(
            f, Policies())
    );
}

template <class F>
object make_function(lua_State* L, F f)
{
    return make_function(L, detail::deduce_signature(f), detail::null_type());
}

} // namespace luabind

#endif // LUABIND_MAKE_FUNCTION_081014_HPP

