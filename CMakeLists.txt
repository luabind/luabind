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

if(NOT Boost_FOUND)
    set(Boost_USE_STATIC_LIBS     ON)
    set(Boost_USE_MULTITHREADED   ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost REQUIRED)
endif()

if(NOT LUA_FOUND AND NOT LUA51_FOUND)
    find_package(Lua51 REQUIRED)
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # We are the top-level project
    option(LUABIND_INSTALL "Install the LuaBind library and headers" ON)
    option(LUABIND_NO_ERROR_CHECKING "Build so that all the Lua code is expected only to make legal calls?" OFF)
    option(LUABIND_NO_EXCEPTIONS "Disable all usage of try, catch, and throw?" OFF)
    option(LUABIND_CPLUSPLUS_LUA "Was Lua built as C++?" OFF)
    if(NOT Boost_VERSION VERSION_GREATER 1.46)
        option(LUABIND_USE_NOEXCEPT "If your compiler is C++11 compliant, but you're using old Boost, you need to set this." OFF)
    endif()
    option(LUABIND_DYNAMIC_LINK "Build luabind as a shared library?" OFF)
endif()

# Configure the build info header
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/luabind")
configure_file(build_information.hpp.cmake_in "${CMAKE_CURRENT_BINARY_DIR}/luabind/build_information.hpp")


set(BUILD_SHARED_LIBS ${LUABIND_DYNAMIC_LINK})

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
