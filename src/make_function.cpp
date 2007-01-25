// Copyright (c) 2006 Daniel Wallin

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

#define LUABIND_SOURCE

#include <luabind/config.hpp>
#include <luabind/make_function.hpp>
#include <luabind/exception_handler.hpp>
#include <luabind/detail/stack_utils.hpp>
#include <boost/bind.hpp>

namespace luabind { namespace aux {

namespace
{

  class function
  {
  public:
      function(callback_type const& call, callback_type const& rank)
        : m_call(call)
        , m_rank(rank)
        , m_next(0)
      {}

      int call(lua_State* L) const;

      callback_type m_call;
      callback_type m_rank;
      function* m_next;

      // Hold a reference to the lua object that is
      // m_next so that it doesn't get garbage collected
      // while it's part of this overload set.
      object m_next_object;
  };

  int destroy_function(lua_State* L)
  {
      function* fn = (function*)lua_touserdata(L, 1);
      fn->~function();
      return 0;
  }

  int call_function(lua_State* L)
  {
      function const* fn = (function const*)lua_touserdata(L, 1);
      lua_remove(L, 1);

      int results = -1;
      {
          boost::optional<int> result = handle_exceptions<int>(
              L, boost::bind(&function::call, fn, L)
          );

          if (result) results = *result;
      }

      if (results < 0)
      {
          lua_error(L);
      }

      return results;
  }

  int function::call(lua_State* L) const
  {
      function const* best = 0;
      int best_rank = std::numeric_limits<int>::max();
      int next_best = std::numeric_limits<int>::max();

      for (function const* fn = this; fn != 0; fn = fn->m_next)
      {
          int rank = fn->m_rank(L);

          if (rank < 0) continue;

          if (rank < best_rank)
          {
              best = fn;
              best_rank = rank;
          }
          else if (rank < next_best)
          {
              next_best = rank;
          }
      }

      if (!best)
      {
          lua_pushstring(L, "No overload found");
          // TODO: Maybe build a list of possible overloads.
          return -1;
      }

      if (best_rank == next_best)
      {
          lua_pushstring(L, "Ambiguity");
          // TODO: Maybe build a list of possible overloads.
          return -1;
      }

      return best->m_call(L);
  }

  void push_function_metatable(lua_State* L)
  {
      lua_pushstring(L, "luabind.function");
      lua_rawget(L, LUA_REGISTRYINDEX);

      if (lua_istable(L, -1))
          return;

      lua_pop(L, 1);

      lua_newtable(L);
      lua_pushstring(L, "luabind.function");
      lua_pushboolean(L, 1);
      lua_rawset(L, -3);

      lua_pushstring(L, "__gc");
      lua_pushcclosure(L, &destroy_function, 0);
      lua_rawset(L, -3);

      lua_pushstring(L, "__call");
      lua_pushcclosure(L, &call_function, 0);
      lua_rawset(L, -3);

      lua_pushstring(L, "luabind.function");
      lua_pushvalue(L, -2);
      lua_rawset(L, LUA_REGISTRYINDEX);
  }

} // namespace unnamed

LUABIND_API object make_function(
    lua_State* L, callback_type const& call, callback_type const& rank)
{
    void* storage = lua_newuserdata(L, sizeof(function));
    detail::stack_pop pop(L, 1);
    new (storage) function(call, rank);

    push_function_metatable(L);
    lua_setmetatable(L, -2);

    return object(from_stack(L, -1));
}

}} // namespace luabind::aux

namespace luabind {

LUABIND_API void add_overload(
    object const& target, char const* name, object const& fn)
{
    if (object overloads = target[name])
    {
        if (getmetatable(overloads)["luabind.function"] 
         && getmetatable(fn)["luabind.function"])
        {
            aux::function* f = touserdata<aux::function>(fn);
            f->m_next = touserdata<aux::function>(overloads);
            f->m_next_object = overloads;
        }
    }

    target[name] = fn;
}

} // namespace luabind

