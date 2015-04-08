
#include "test.hpp"
#include <luabind/luabind.hpp>

class indexable
{
public:
    indexable() : val1(1), val2(2), prop(0)
    {}

    int val1;
    int val2;

    int prop;

    int getVal(const char* x)
    {
        if (*x == 'a')
        {
            return val1;
        }
        return val2;
    }

    void setVal(const char* x, int v)
    {
        if (*x == 'a')
        {
            val1 = v;
            return;
        }
        val2 = v;
    }
};

indexable instance;
indexable* get()
{
    return &instance;
}

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L)[
        class_<indexable>("test")

            .defaultGet(&indexable::getVal)
            .defaultSet(&indexable::setVal)
            
            .def_readwrite("prop", &indexable::prop),
            def("get", &get)
    ];
    DOSTRING(L, "x = get()\n");
    TEST_CHECK(instance.val1 == 1);
    TEST_CHECK(instance.val2 == 2);
    TEST_CHECK(instance.prop == 0);

    DOSTRING(L, "assert(x.a == 1)\n");
    DOSTRING(L, "assert(x.b == 2)\n");

    DOSTRING(L, "x.a = 5\n");
    TEST_CHECK(instance.val1 == 5);
    TEST_CHECK(instance.val2 == 2);
    TEST_CHECK(instance.prop == 0);

    DOSTRING(L, "x.b = 10\n");
    TEST_CHECK(instance.val1 == 5);
    TEST_CHECK(instance.val2 == 10);
    TEST_CHECK(instance.prop == 0);

    // Check if properties work
    DOSTRING(L, "x.prop = 20\n");
    TEST_CHECK(instance.val1 == 5);
    TEST_CHECK(instance.val2 == 10);
    TEST_CHECK(instance.prop == 20);
}

