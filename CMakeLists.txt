cmake_minimum_required(VERSION 2.6)
project(luabind)

option(LUABIND_ENABLE_WARNINGS
       "Enable compiler warnings during the build of luabind and the tests.")
option(LUABIND_NO_CXX11
       "Disable C++11 support (only has an effect for g++ and Clang)")
option(LUABIND_SKIP_TESTS
       "Do not build the tests (they must be *run* manually in any case)")
option(LUABIND_BUILD_HEADER_TESTS
       "Enable this for (many) additional compilation tests (N/A if LUABIND_SKIP_TESTS).")
option(BUILD_SHARED_LIBS
    "Build luabind as a shared/dynamic library (.so or DLL). You must link Lua dynamically then.")

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.51.0 REQUIRED)
find_package(Lua52 REQUIRED)

if (BUILD_SHARED_LIBS)
    add_definitions("-DLUABIND_DYNAMIC_LINK")
endif()

if (MSVC)
    add_definitions("/MP") # Multiprocessor compilation
    if (LUABIND_ENABLE_WARNINGS)
        add_definitions(
            "/W4" # Warning level 4
            
            # Disable incorrect warning C4709
            # ("comma operator within array index expression"):
            "/wd4709")
    endif(LUABIND_ENABLE_WARNINGS)
    if (BUILD_SHARED_LIBS) # Yes, this also concers the default /W3.
        # warning C4251
        # "... needs to have dll-interface to be used by clients of ..."
        # is issued a lot for private symbols or symbols in a detail
        # namespace.
        add_definitions("/wd4251")
    endif(BUILD_SHARED_LIBS)
else(MSVC)
    # see http://stackoverflow.com/a/11004675/2128694
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7
                AND NOT LUABIND_NO_CXX11)
            add_definitions("-std=c++11")
            if (LUABIND_ENABLE_WARNINGS)
                add_definitions("-Wnoexcept")
            endif(LUABIND_ENABLE_WARNINGS)
        endif() # gcc >= 4.7 and C++11 support not explicitly disabled
        # This warning is enabled by default:
        add_definitions("-Wno-deprecated-declarations") # auto_ptr is OK for now.
        if (LUABIND_ENABLE_WARNINGS)
            if (GCC_VERSION VERSION_GREATER 4.3
                    OR GCC_VERSION VERSION_EQUAL 4.3)
                add_definitions("-Wlogical-op" "-Wmissing-declarations"
                                "-Wsign-conversion")
            endif() # gcc >= 4.3
            add_definitions("-pedantic" "-Wall" "-Wextra" "-Wcast-align"
                            "-Wcast-qual" "-Wctor-dtor-privacy" "-Winit-self"
                            "-Wdisabled-optimization" "-Wformat=2"
                            "-Wmissing-include-dirs" "-Wold-style-cast"
                            "-Woverloaded-virtual" "-Wredundant-decls"
                            "-Wshadow" "-Wsign-promo"
                            "-Wstrict-null-sentinel" "-Wstrict-overflow=5")
        endif(LUABIND_ENABLE_WARNINGS)
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        if (LUABIND_ENABLE_WARNINGS)
            add_definitions("-Weverything" "-Wno-shadow" "-Wno-undef"
                            "-Wno-global-constructors" "-Wno-weak-vtables"
                            "-Wno-padded" "-Wno-exit-time-destructors")
        endif(LUABIND_ENABLE_WARNINGS)
        if (NOT LUABIND_NO_CXX11)
            # I do not know (a) since when Clang supports this flags and (b) how to
            # check Clang's version. Use LUABIND_NO_CXX11 manually if you need to.
            add_definitions("-std=c++11")
            if (LUABIND_ENABLE_WARNINGS)
                add_definitions("-Wno-c++98-compat")
            endif(LUABIND_ENABLE_WARNINGS)
        endif(NOT LUABIND_NO_CXX11)
    endif() # Clang
endif(MSVC) # else part

add_subdirectory("src")

if (NOT LUABIND_SKIP_TESTS)
    enable_testing()
    add_subdirectory("test")
endif()

install(DIRECTORY luabind DESTINATION include
        FILES_MATCHING PATTERN "*.hpp")
if(WIN32 AND NOT CYGWIN)
  set(INSTALL_CMAKE_DIR CMake)
  set(INSTALL_CMAKE_DIR_LUA CMake)
else()
  set(INSTALL_CMAKE_DIR lib/CMake/Luabind)
  set(INSTALL_CMAKE_DIR_LUA lib/CMake/Lua52)
endif()
INSTALL(FILES "cmake/Modules/FindLuabind.cmake" DESTINATION ${INSTALL_CMAKE_DIR})
INSTALL(FILES "cmake/Modules/FindLua52.cmake" DESTINATION ${INSTALL_CMAKE_DIR_LUA})
