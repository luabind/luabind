include_directories(
    ${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

add_library(testmain
    "main.cpp" "test.hpp"
    # These tests consist only of compile time asserts and define no test_main
    # function:
    "test_has_get_pointer.cpp"
    "test_typetraits.cpp")
    
target_link_libraries(testmain ${LUA_LIBRARIES} luabind)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(TESTS
    "abstract_base"
    "adopt"
    "adopt_wrapper"
    "attributes"
    "automatic_smart_ptr"
    "back_reference"
    "builtin_converters"
    "class_info"
    "collapse_converter"
    "const"
    "construction"
    "create_in_thread"
    "def_from_base"
    "dynamic_type"
    "exceptions"
    "exception_handlers"
    "extend_class_in_lua"
    "free_functions"
    "held_type"
    "implicit_cast"
    "implicit_raw"
    "iterator"
    "lua_classes"
    "null_pointer"
    "object"
    "operators"
    "policies"
    "private_destructors"
    "properties"
    "scope"
    "separation"
    "set_instance_value"
    "shadow"
    "shared_ptr"
    "simple_class"
    "smart_ptr_attributes"
    "super_leak"
    "table"
    "tag_function"
    "unsigned_int"
    "user_defined_converter"
    "value_wrapper"
    "vector_of_object"
    "virtual_inheritance"
    "yield")

foreach(test ${TESTS})
    add_executable(test_${test} "test_${test}.cpp")
    target_link_libraries(test_${test} testmain)
    set_property(TARGET test_${test} PROPERTY FOLDER "Tests")
    add_test(${test} test_${test})
endforeach(test)
