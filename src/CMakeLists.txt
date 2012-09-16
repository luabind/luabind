add_definitions(-DLUABIND_BUILDING)
include_directories(${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

add_library(luabind wrapper_base.cpp weak_ref.cpp pcall.cpp error.cpp
                    scope.cpp class.cpp class_rep.cpp function.cpp
                    find_best_match.cpp implicit_cast.cpp open.cpp
                    create_class.cpp stack_content_by_name.cpp
                    object_rep.cpp class_info.cpp ref.cpp class_registry.cpp
                    overload_rep.cpp link_compatibility.cpp)

target_link_libraries(luabind ${LUA_LIBRARIES})
