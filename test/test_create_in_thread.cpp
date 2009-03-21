// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <luabind/luabind.hpp>

int count = 0;

struct X
{
    X()
    {
        ++count;
    }

    ~X()
    {
        --count;
    }
};

#include <iostream>

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L) [
        class_<X>("X")
            .def(constructor<>())
    ];

    for (int i = 0; i < 100; ++i)
    {
        lua_State* thread = lua_newthread(L);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        DOSTRING(thread,
            "local x = X()\n"
        );

        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
}
