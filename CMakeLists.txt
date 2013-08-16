# Build for LuaBind
# Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com/
# Iowa State University HCI Graduate Program/VRAC

cmake_minimum_required(VERSION 2.8)
set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required
project(LuaBind)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "9")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION
    "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx11/Modules")

if(NOT Boost_FOUND)
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_MULTITHREADED   ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost REQUIRED)
endif()

if(NOT LUA_FOUND AND NOT LUA51_FOUND AND NOT LUA52_FOUND)
    set(LUABIND_LUA_VERSION "" CACHE STRING
        "51 or 52; empty to use 51 if 52 cannot be found and 52 otherwise.")

    if(LUABIND_LUA_VERSION)
        find_package("Lua${LUABIND_LUA_VERSION}" REQUIRED)
    else()
        find_package(Lua52)
        if(NOT LUA52_FOUND)
            find_package(Lua51)
            if(NOT LUA51_FOUND)
                message(FATAL_ERROR "Neither Lua 5.1 nor 5.2 could be found.")
            endif()
        endif()
    endif()
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
endif()

# C++11-related searches
find_package(CXXFeatures)

set(LUABIND_CXX11_DEFAULT OFF)
if(CXX11_COMPILER_FLAGS) # TODO this is not quite the right check to use here.
    set(LUABIND_CXX11_DEFAULT ON)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # We are the top-level project
    option(LUABIND_INSTALL "Install the LuaBind library and headers" ON)
    option(LUABIND_NO_ERROR_CHECKING "Build so that all the Lua code is expected only to make legal calls?" OFF)
    option(LUABIND_NO_EXCEPTIONS "Disable all usage of try, catch, and throw?" OFF)
    option(LUABIND_CPLUSPLUS_LUA "Was Lua built as C++?" OFF)
    option(LUABIND_DYNAMIC_LINK "Build luabind as a shared library?" OFF)
    option(LUABIND_NOT_THREADSAFE "Permit the use of static variables, thus making Luabind usable only from one of your system threads." OFF)
    option(LUABIND_ENABLE_WARNINGS "Enable extra warnings during the build of the library and tests" ON)
    option(LUABIND_USE_CXX11 "Build Luabind using C++11 features?" ${LUABIND_CXX11_DEFAULT})

    if(NOT Boost_VERSION VERSION_GREATER 1.46)
        option(LUABIND_USE_NOEXCEPT "If your compiler is C++11 compliant, but you're using old Boost, you need to set this." OFF)
    endif()

    if(LUABIND_USE_CXX11)
        option(LUABIND_NO_SCOPED_ENUM "Indicates that your compiler and standard library lacks scoped enums and std::underlying_type" OFF)

    else()
        # No CXX11, can't have scoped enum
        # TODO is this right?
        #set(LUABIND_NO_SCOPED_ENUM ON CACHE INTERNAL "" FORCE)
    endif()

endif()

# Configure the build info header
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/luabind")
configure_file(build_information.hpp.cmake_in "${CMAKE_CURRENT_BINARY_DIR}/luabind/build_information.hpp")

# Set up the build
set(BUILD_SHARED_LIBS ${LUABIND_DYNAMIC_LINK})

if(LUABIND_USE_CXX11 AND CXX11_COMPILER_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_COMPILER_FLAGS}")
endif()

# Test and enable warning flags if desired, as well as other compiler flag
include(CheckCXXCompilerFlag)
macro(_luabind_add_flag _flag _var)
    check_cxx_compiler_flag(${_flag} ${_var})
    if(${_var})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_flag}")
    endif()
endmacro()

if(MSVC)
    if(LUABIND_ENABLE_WARNINGS)
        # Warning level 4
        _luabind_add_flag(/W4 SUPPORTS_W4_FLAG)

        # Disable incorrect warning C4709
        # ("comma operator within array index expression")
        _luabind_add_flag(/wd4709 SUPPORTS_WD4709_FLAG)
    endif()
else()

    set(possible_flags)
    if(BUILD_SHARED_LIBS)
        list(APPEND possible_flags fvisibility=hidden)
    endif()
    if(LUABIND_ENABLE_WARNINGS)
        list(APPEND possible_flags
            Wall
            Wextra
            Wno-deprecated-declarations # auto_ptr is OK for now
            Wlogical-op
            Wmissing-declarations
            Wsign-conversion
            pedantic
            Wcast-align
            Wcast-qual
            Wctor-dtor-privacy
            Winit-self
            Wdisabled-optimizations
            Wformat=2
            Wmissing-include-dirs
            Wold-style-cast
            Woverloaded-virtual
            Wredundant-decls
            Wshadow
            Wsign-promo
            Wstrict-null-sentinel
            Wstrict-overflow=5)


        if(LUABIND_USE_CXX11)
            list(APPEND possible_flags Wnoexcept)
        endif()

        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
            list(APPEND possible_flags
                Weverything
                Wno-shadow
                Wno-undef
                Wno-global-constructors
                Wno-weak-vtables
                Wno-padded
                Wno-exit-time-destructors
                Wno-missing-noreturn)

            if(LUABIND_USE_CXX11)
               list(APPEND possible_flags
                    Wno-c++98-compat
                    Wno-c++98-compat-pedantic)
            else()
               list(APPEND possible_flags
                    Wno-c++11-long-long)
            endif()

        endif()

    endif()

    if(possible_flags)
        foreach(_flag ${possible_flags})
            string(REGEX REPLACE [-+=] _ sanitized_flag ${_flag})
            _luabind_add_flag(-${_flag} SUPPORTS_${sanitized_flag}_FLAG)
        endforeach()
    endif()
endif()

include_directories(${Boost_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
    ${LUA_INCLUDE_DIRS})

add_subdirectory(src)

# Set up testing and documentation, only if we are the top-level project.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
    add_subdirectory(doc)
endif()

if(LUABIND_INSTALL)
    if(WIN32 AND NOT CYGWIN)
        set(INSTALL_CMAKE_DIR CMake)
        set(INSTALL_CMAKE_DIR_LUA CMake)
    else()
        set(INSTALL_CMAKE_DIR lib/CMake/Luabind)
        set(INSTALL_CMAKE_DIR_LUA lib/CMake/Lua52)
    endif()
    install(FILES "cmake/Modules/FindLuabind.cmake" DESTINATION ${INSTALL_CMAKE_DIR})
    install(FILES "cmake/Modules/FindLua52.cmake" DESTINATION ${INSTALL_CMAKE_DIR_LUA})
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/luabind/build_information.hpp" DESTINATION include/luabind)
endif()
