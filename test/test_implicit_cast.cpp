// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

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

