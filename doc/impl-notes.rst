Implementation notes
====================

The classes and objects are implemented as user data in Lua. To make sure that
the user data really is the internal structure it is supposed to be, we tag
their metatables. A user data who's metatable contains a boolean member named
``__luabind_classrep`` is expected to be a class exported by luabind. A user
data who's metatable contains a boolean member named ``__luabind_class`` is
expected to be an instantiation of a luabind class.

This means that if you make your own user data and tags its metatable with the
exact same names, you can very easily fool luabind and crash the application.

In the Lua registry, luabind keeps an entry called ``__luabind_classes``. It
should not be removed or overwritten.

In the global table, a variable called ``super`` is used every time a
constructor in a lua-class is called. This is to make it easy for that
constructor to call its base class' constructor. So, if you have a global
variable named super it may be overwritten. This is probably not the best
solution, and this restriction may be removed in the future.

.. note:: Deprecated

  ``super()`` has been deprecated since version 0.8 in favor of directly
  invoking the base class' ``__init()`` function::

    function Derived:__init()
        Base.__init(self)
    end

Luabind uses two upvalues for functions that it registers. The first is a
userdata containing a list of overloads for the function, the other is a light
userdata with the value 0x1337, this last value is used to identify functions
registered by luabind. It should be virtually impossible to have such a pointer
as secondary upvalue by pure chance. This means, if you are trying to replace
an existing function with a luabind function, luabind will see that the
secondary upvalue isn't the magic id number and replace it. If it can identify
the function to be a luabind function, it won't replace it, but rather add
another overload to it.

Inside the luabind namespace, there's another namespace called detail. This
namespace contains non-public classes and are not supposed to be used directly.
