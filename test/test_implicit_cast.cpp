#include "test.hpp"
#include <luabind/luabind.hpp>

namespace {

    struct A : counted_type<A> 
    { virtual ~A() {} };

    struct B : A, counted_type<B>  
    {};

    struct test_implicit : counted_type<test_implicit>
    {
        char const* f(A*) { return "f(A*)"; }
        char const* f(B*) { return "f(B*)"; }
    };

    struct char_pointer_convertible
        : counted_type<char_pointer_convertible>
    {
        operator const char*() const { return "foo!"; }
    };

    void func(const char_pointer_convertible& f)
    {
    }

} // anonymous namespace

void test_implicit_cast()
{
    COUNTER_GUARD(A);
    COUNTER_GUARD(B);
    COUNTER_GUARD(test_implicit);
    COUNTER_GUARD(char_pointer_convertible);

    lua_state L;

    using namespace luabind;

    typedef char const*(test_implicit::*f1)(A*);
    typedef char const*(test_implicit::*f2)(B*);

    module(L)
    [
        class_<A>("A")
            .def(constructor<>()),
    
        class_<B, A>("B")
            .def(constructor<>()),
    
        class_<test_implicit>("test")
            .def(constructor<>())
            .def("f", (f1)&test_implicit::f)
            .def("f", (f2)&test_implicit::f),

        class_<char_pointer_convertible>("char_ptr")
            .def(constructor<>()),

        def("func", &func)
    ];

    DOSTRING(L, "a = A()");
    DOSTRING(L, "b = B()");
    DOSTRING(L, "t = test()");

    DOSTRING(L, "assert(t:f(a) == 'f(A*)')");
    DOSTRING(L, "assert(t:f(b) == 'f(B*)')");

    DOSTRING(L, 
        "a = char_ptr()\n"
        "func(a)");
}

