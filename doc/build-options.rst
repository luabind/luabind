.. _part-build-options:

Build options
=============

There are a number of configuration options available when building luabind.
It is very important that your project has the exact same configuration 
options as the ones given when the library was build! The exceptions are the
``LUABIND_MAX_ARITY`` and ``LUABIND_MAX_BASES`` which are template-based 
options and only matters when you use the library (which means they can 
differ from the settings of the library).

The default settings which will be used if no other settings are given
can be found in ``luabind/config.hpp``.

If you want to change the settings of the library, you can modify the 
config file. It is included and used by all makefiles. You can change paths
to Lua and boost in there as well.

LUABIND_MAX_ARITY
    Controls the maximum arity of functions that are registered with luabind. 
    You can't register functions that takes more parameters than the number 
    this macro is set to. It defaults to 5, so, if your functions have greater 
    arity you have to redefine it. A high limit will increase compilation time.

LUABIND_MAX_BASES
    Controls the maximum number of classes one class can derive from in 
    luabind (the number of classes specified within ``bases<>``). 
    ``LUABIND_MAX_BASES`` defaults to 4. A high limit will increase 
    compilation time.

LUABIND_NO_ERROR_CHECKING
    If this macro is defined, all the Lua code is expected only to make legal 
    calls. If illegal function calls are made (e.g. giving parameters that 
    doesn't match the function signature) they will not be detected by luabind
    and the application will probably crash. Error checking could be disabled 
    when shipping a release build (given that no end-user has access to write 
    custom Lua code). Note that function parameter matching will be done if a 
    function is overloaded, since otherwise it's impossible to know which one 
    was called. Functions will still be able to throw exceptions when error 
    checking is disabled.

    If a function throws an exception it will be caught by luabind and 
    propagated with ``lua_error()``.

LUABIND_NO_EXCEPTIONS
    This define will disable all usage of try, catch and throw in luabind. 
    This will in many cases disable run-time errors, when performing invalid 
    casts or calling Lua functions that fails or returns values that cannot 
    be converted by the given policy. luabind requires that no function called 
    directly or indirectly by luabind throws an exception (throwing exceptions 
    through Lua has undefined behavior).

    Where exceptions are the only way to get an error report from luabind, 
    they will be replaced with calls to the callback functions set with
    ``set_error_callback()`` and ``set_cast_failed_callback()``.

LUA_API
    If you want to link dynamically against Lua, you can set this define to 
    the import-keyword on your compiler and platform. On Windows in Visual Studio 
    this should be ``__declspec(dllimport)`` if you want to link against Lua 
    as a dll.

LUABIND_DYNAMIC_LINK
    Must be defined if you intend to link against the luabind shared
    library.

LUABIND_NO_RTTI
    You can define this if you don't want luabind to use ``dynamic_cast<>``.
    It will disable :ref:`sec-objid`.

NDEBUG
    This define will disable all asserts and should be defined in a release 
    build.