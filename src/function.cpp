// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define LUABIND_BUILDING

#include <luabind/exception_handler.hpp>
#include <luabind/make_function.hpp>

#include <boost/bind.hpp>

namespace luabind { namespace detail {

namespace
{

  struct function
  {
      function(
          int arity
        , function_callback const& call
        , function_callback const& score
        , signature_callback const& signature
      )
        : m_arity(arity)
        , m_call(call)
        , m_score(score)
        , m_signature(signature)
        , m_next(0)
      {}

      int call(lua_State*) const;

      std::string m_name;
      int m_arity;
      function_callback m_call;
      function_callback m_score;
      signature_callback m_signature;
      function const* m_next;
      object m_keep_alive;
  };

  int const max_candidates = 10;

  int function::call(lua_State* L) const
  {
      function const* candidates[max_candidates];
      int candidate_idx = 0;

      function const* best = 0;
      int best_score = std::numeric_limits<int>::max();

      int args = lua_gettop(L);

      char const* function_name = m_name.empty() ? "<unknown>" : m_name.c_str();

      for (function const* f = this; f != 0; f = f->m_next)
      {
          if (args != f->m_arity)
              continue;

          int score = f->m_score(L);

          if (score < 0)
              continue;

          if (score < best_score)
          {
              best = f;
              best_score = score;
              candidate_idx = 0;
          }
          else if (score == best_score)
          {
              best = 0;
          }

          assert(candidate_idx < max_candidates);
          candidates[candidate_idx++] = f;
      }

      if (!best)
      {
          if (best_score != std::numeric_limits<int>::max())
          {
              // Ambiguous
              lua_pushstring(L, "Ambiguous, candidates:\n");
              for (int i = 0; i < candidate_idx; ++i)
              {
                  if (i != 0)
                      lua_pushstring(L, "\n");
                  candidates[i]->m_signature(L, function_name);
              }
              lua_concat(L, candidate_idx * 2);
          }
          else
          {
              // No overload
              lua_pushstring(L, "No matching overload found, candidates:\n");
              int count = 0;
              for (function const* f = this; f != 0; f = f->m_next)
              {
                  if (count != 0)
                      lua_pushstring(L, "\n");
                  f->m_signature(L, function_name);
                  ++count;
              }
              lua_concat(L, count * 2);
          }

          return -2;
      }

      return best->m_call(L);
  }

  int function_dispatcher(lua_State* L)
  {
      function const* f =
          (function const*)lua_touserdata(L, lua_upvalueindex(1));

      int results = -2;
      {
          boost::optional<int> result = handle_exceptions<int>(
              L, boost::bind(&function::call, f, L));

          if (result)
              results = *result;
      }

      if (results == -2)
          lua_error(L);

      return results;
  }

  int function_destroy(lua_State* L)
  {
      function* fn = (function*)lua_touserdata(L, 1);
      fn->~function();
      return 0;
  }

  void push_function_metatable(lua_State* L)
  {
      lua_pushstring(L, "luabind.function");
      lua_rawget(L, LUA_REGISTRYINDEX);

      if (lua_istable(L, -1))
          return;

      lua_pop(L, 1);

      lua_newtable(L);

      lua_pushstring(L, "__gc");
      lua_pushcclosure(L, &function_destroy, 0);
      lua_rawset(L, -3);

      lua_pushstring(L, "luabind.function");
      lua_pushvalue(L, -2);
      lua_rawset(L, LUA_REGISTRYINDEX);
  }

} // namespace unnamed

LUABIND_API bool is_luabind_function(lua_State* L, int index)
{
    return lua_tocfunction(L, index) == &function_dispatcher;
}

LUABIND_API void add_overload(
    object const& context, char const* name, object const& fn)
{
    function* f = touserdata<function>(getupvalue(fn, 1));
    f->m_name = name;

    if (object overloads = context[name])
    {
        if (tocfunction(overloads) == &function_dispatcher
         && tocfunction(fn) == &function_dispatcher)
        {
            f->m_next = touserdata<function>(getupvalue(overloads, 1));
            f->m_keep_alive = overloads;
            f->m_name = name;
        }
    }

    context[name] = fn;
}

LUABIND_API object make_function_aux(
    lua_State* L, int arity
  , function_callback const& call
  , function_callback const& score
  , signature_callback const& signature
)
{
    void* storage = lua_newuserdata(L, sizeof(function));
    push_function_metatable(L);
    new (storage) function(arity, call, score, signature);
    lua_setmetatable(L, -2);

    lua_pushcclosure(L, &function_dispatcher, 1);
    stack_pop pop(L, 1);

    return object(from_stack(L, -1));
}

}} // namespace luabind::detail

