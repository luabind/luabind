cmake_minimum_required(VERSION 2.6)
project(luabind)

find_package(Boost 1.51.0 REQUIRED)

# Adjust to actual locations
set (LUABINARIES_DIR "C:/myProgs/cpp/Bibliotheken/luabinaries")
set (LUA_INCLUDE_DIR "${LUABINARIES_DIR}/include")
set (LUA_LIBRARY
    optimized "${LUABINARIES_DIR}/lua52.lib"
    debug     "${LUABINARIES_DIR}/lua52-d.lib")
add_subdirectory("src")