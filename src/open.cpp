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

#include <luabind/lua_include.hpp>

#include <luabind/luabind.hpp>
#include <luabind/function.hpp>

namespace luabind {

namespace
{

  int make_property(lua_State* L)
  {
      int args = lua_gettop(L);

      if (args == 0 || args > 2)
      {
          lua_pushstring(L, "make_property() called with wrong number of arguments.");
          lua_error(L);
      }

      if (args == 1)
          lua_pushnil(L);

      lua_pushcclosure(L, &detail::property_tag, 2);
      return 1;
  }

} // namespace unnamed

    void open(lua_State* L)
    {
        // get the global class registry, or create one if it doesn't exist
        // (it's global within a lua state)
        detail::class_registry* r = 0;

        // If you hit this assert it's because you have called luabind::open()
        // twice on the same lua_State.
        assert((detail::class_registry::get_registry(L) == 0) 
            && "you cannot call luabind::open() twice");

        lua_pushstring(L, "__luabind_classes");
        r = static_cast<detail::class_registry*>(
            lua_newuserdata(L, sizeof(detail::class_registry)));

        // set gc metatable
        lua_newtable(L);
        lua_pushstring(L, "__gc");
        lua_pushcclosure(
            L
          , detail::garbage_collector_s<
                detail::class_registry
            >::apply
          , 0);

        lua_settable(L, -3);
        lua_setmetatable(L, -2);

        new(r) detail::class_registry(L);
        lua_settable(L, LUA_REGISTRYINDEX);

        // add functions (class, cast etc...)
        lua_pushstring(L, "class");
        lua_pushcclosure(L, detail::create_class::stage1, 0);
        lua_settable(L, LUA_GLOBALSINDEX);

        lua_pushstring(L, "property");
        lua_pushcclosure(L, &make_property, 0);
        lua_settable(L, LUA_GLOBALSINDEX);
    }

} // namespace luabind

