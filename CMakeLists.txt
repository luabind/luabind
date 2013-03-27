cmake_minimum_required(VERSION 2.6)
project(luabind)
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules/")

find_package(Boost 1.51.0 REQUIRED)
find_package(Lua52 REQUIRED)

add_subdirectory("src")
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