cmake_minimum_required(VERSION 2.6)
project(luabind)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_MULTITHREADED   ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.51.0 REQUIRED)
find_package(Lua52 REQUIRED)

if (MSVC)
    add_definitions(
        "/W4" # Warning level 4
        "/MP" # Multiprocessor compilation
        # Disable incorrect warning C4709
        # ("comma operator within array index expression"):
        "/wd4709")
else()
    # see http://stackoverflow.com/a/11004675/2128694
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
        if (GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7 AND NOT LUABIND_NO_CXX11)
            add_definitions("-std=c++11")
        endif()
        add_definitions("-pedantic" "-Wall" "-Wextra" "-Wcast-align"
                        "-Wcast-qual" "-Wctor-dtor-privacy"
                        "-Wdisabled-optimization" "-Wformat=2" "-Winit-self"
                        "-Wlogical-op" "-Wmissing-declarations"
                        "-Wmissing-include-dirs" "-Wnoexcept" "-Wold-style-cast"
                        "-Woverloaded-virtual"
                        "-Wredundant-decls"
                        "-Wshadow"
                        "-Wsign-conversion" "-Wsign-promo"
                        "-Wstrict-null-sentinel" "-Wstrict-overflow=5"
                        "-Wno-deprecated-declarations") # auto_ptr is OK for now.
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_definitions("-Weverything" "-Wno-shadow" "-Wno-undef"
                        "-Wno-global-constructors" "-Wno-weak-vtables"
                        "-Wno-padded" "-Wno-exit-time-destructors")
        if (NOT LUABIND_NO_CXX11)
            # I do not know (a) since when Clang supports this flags and (b) how to
            # check Clang's version. Use LUABIND_NO_CXX11 manually if you need to.
            add_definitions("-std=c++11" "-Wno-c++98-compat")
        endif()
    endif ()
endif()

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
