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

``LUABIND_MAX_ARITY``
    Controls the maximum arity of functions that are registered with luabind. 
    You can't register functions that takes more parameters than the number 
    this macro is set to. It defaults to 5, so, if your functions have greater 
    arity you have to redefine it. A high limit will increase compilation time.

``LUABIND_MAX_BASES``
    Controls the maximum number of classes one class can derive from in 
    luabind (the number of classes specified within ``bases<>``). 
    ``LUABIND_MAX_BASES`` defaults to 4. A high limit will increase 
    compilation time.

``LUABIND_NO_ERROR_CHECKING``
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

``LUABIND_NO_EXCEPTIONS``
    This define will disable all usage of try, catch and throw in luabind. 
    This will in many cases disable run-time errors, when performing invalid 
    casts or calling Lua functions that fails or returns values that cannot 
    be converted by the given policy. luabind requires that no function called 
    directly or indirectly by luabind throws an exception (throwing exceptions 
    through Lua has undefined behavior).

    Where exceptions are the only way to get an error report from luabind, 
    they will be replaced with calls to the callback functions set with
    ``set_error_callback()`` and ``set_cast_failed_callback()``.

``LUA_API``
    If you want to link dynamically against Lua, you can set this define to 
    the import-keyword on your compiler and platform. On Windows in Visual Studio 
    this should be ``__declspec(dllimport)`` if you want to link against Lua 
    as a dll.

``LUABIND_DYNAMIC_LINK``
    Must be defined if you intend to link against the luabind shared
    library.

``LUABIND_CPLUSPLUS_LUA``
    Without this, all included Lua headers will be wrapped in ``extern "C"``.
    Define this if you compiled Lua as C++.

``NDEBUG``
    This define will disable all asserts and should be defined in a release 
    build.

``LUABIND_USE_NOEXCEPT``
    If you use Boost <= 1.46 but your compiler is C++11 compliant and marks
   destructors as noexcept by default, you need to define LUABIND_USE_NOEXCEPT.
   Failing to do so will cause std::terminate() being called if a destructor
   throws (e.g. the destructor of the proxy returned by ``call_function`` or
   ``object::operator()``).

CMake options
~~~~~~~~~~~~~

The following options can be given to CMake when building luabind. You can
either specify them directly on the commandline using ``-Doption=value`` or
let CMake ask you for each of them by invoking it with the ``-i`` flag. On
Windows, you will usually prefer ``cmake-gui``.

If not specified otherwise, all options are boolean and default to ``OFF``.

``LUABIND_ENABLE_WARNINGS``
    Enable compiler warnings during the build of luabind and the tests.

``LUABIND_NO_CXX11``
    Disable C++11 support: By default the ``-std=c++11`` compiler option is
    specified on g++ >= 4.7 and all Clang versions (no version check is
    implemented for this compiler). Enabling this option will cause the
    compiler flag to be omitted.

``LUABIND_SKIP_TESTS``
    Do not build the tests (they must be run manually in any case).

``LUABIND_BUILD_HEADER_TESTS``
    Enable this for (many) additional compilation tests: Each of Luabind’s
    public headers will be included in a generated .cpp file which otherwise
    only contains an empty main function, meaning that one dummy binary per
    header file is generated.

    This can only be enabled if ``LUABIND_SKIP_TESTS`` is not enabled.

``LUABIND_APPEND_VERSION_SUFFIX``
    With this option set (the default), built binaries and object archives
    will be named e.g. ``luabind09.dll`` instead of just ``luabind.dll``.
    
``LUABIND_LUA_VERSION``
    A string variable with defaults to ``""`` (the empty string). It can be
    set to ``"51"`` or ``"52"`` to use the respective Lua versions. If left
    empty, Lua52 will be tried first before falling back to Lua51.

``BUILD_SHARED_LIBS``
    Build luabind as a shared/dynamic library (.so or DLL). If enabling this,
    you should link Lua dynamically to both Luabind and your application. If
    you encounter double-free errors or other memory corruption, this might be
    the problem.

``CMAKE_BUILD_TYPE``
    See the corresponding entry in the CMake manual.