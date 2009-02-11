// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <luabind/detail/shared_ptr_converter.hpp>

struct X
{
    X(int value)
      : value(value)
    {}

    int value;
};

int get_value(boost::shared_ptr<X> const& p)
{
    return p->value;
}

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L) [
        class_<X>("X")
            .def(constructor<int>()),
        def("get_value", &get_value)
    ];

    DOSTRING(L,
        "x = X(1)\n"
        "assert(get_value(x) == 1)\n"
    );
}
