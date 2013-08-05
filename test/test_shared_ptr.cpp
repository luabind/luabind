// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <luabind/shared_ptr_converter.hpp>
#include <luabind/std_shared_ptr_converter.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace {

struct B: boost::enable_shared_from_this<B> {};
struct D: B {};

struct X
{
    X(int value_)
      : value(value_)
    {}

    int value;
};

int get_value(boost::shared_ptr<X> const& p)
{
    return p->value;
}

boost::shared_ptr<X> filter(boost::shared_ptr<X> const& p, long expected_uc)
{
    TEST_CHECK(p.use_count() == expected_uc);
    return p;
}

void needs_b(boost::shared_ptr<B> const& p)
{
    TEST_CHECK(p.use_count() == 2);
}

#ifndef LUABIND_NO_STD_SHARED_PTR

struct B2: std::enable_shared_from_this<B2> {};
struct D2: B2 {};

void needs_b2(std::shared_ptr<B2> const& p)
{
    TEST_CHECK(p.use_count() == 2);
}

#endif

} // namespace unnamed

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L) [
        class_<X>("X")
            .def(constructor<int>()),
        def("get_value", &get_value),
        def("filter", &filter),

        class_<B, boost::shared_ptr<B> >("B")
            .def(constructor<>()),
        class_<D, B, boost::shared_ptr<D> >("D")
            .def(constructor<>()),
        def("needs_b", &needs_b)
        
#ifndef LUABIND_NO_STD_SHARED_PTR
        ,
        class_<B2, std::shared_ptr<B2> >("B2")
            .def(constructor<>()),
        class_<D2, B2, std::shared_ptr<D2> >("D2")
            .def(constructor<>()),
        def("needs_b2", &needs_b2)
#endif
    ];

    DOSTRING(L,
        "x = X(1)\n"
        "assert(get_value(x) == 1)\n"
    );

    DOSTRING(L,
        "assert(x == filter(x, 1))\n"
    );

    boost::shared_ptr<X> spx(new X(2));
    globals(L)["x2"] = spx;
    TEST_CHECK(spx.use_count() == 2);
    DOSTRING(L, "assert(get_value(x2) == 2)");

    // Raw equality could only be provided here at the cost of creating a new
    // shared_ptr with a custom deleter, which in turn would mean that the
    // refered C++-object would be deleted when the lua_State is closed.
    DOSTRING(L, "filter(x2, 3)");
    DOSTRING(L,
        "x = nil\n"
        "x2 = nil\n"
        "collectgarbage()\n"
        "collectgarbage()\n"
    );
    TEST_CHECK(spx.use_count() == 1);

    DOSTRING(L,
        "d = D()\n"
        "needs_b(d)\n" // test that automatic downcasting works
    );

#ifndef LUABIND_NO_STD_SHARED_PTR
    // And the same once again with std::shared_ptr:
    DOSTRING(L,
        "d2 = D2()\n"
        "needs_b2(d2)\n" // test that automatic downcasting works
    );
#endif
}
