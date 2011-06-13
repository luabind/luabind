include_directories(
    ${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
    
add_executable(benchmark "benchmark.cpp")
target_link_libraries(benchmark ${LUA_LIBRARIES} luabind)

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
    "destruction"
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

if (NOT LUABIND_SKIP_HEADER_TESTS)
	get_filename_component(BASE "${CMAKE_CURRENT_SOURCE_DIR}/../luabind" ABSOLUTE)
	foreach(HEADER ${APIHEADERS})
		get_filename_component(FULLHEADER "${HEADER}" ABSOLUTE)
		file(RELATIVE_PATH SHORTNAME "${BASE}" "${FULLHEADER}")
		string(REPLACE "/" "_" SHORTNAME "${SHORTNAME}")
		string(REPLACE ".hpp" "" SHORTNAME "${SHORTNAME}")
		configure_file(test_headercompile.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/test_headercompile_${SHORTNAME}.cpp" @ONLY)

		add_executable(test_headercompile_${SHORTNAME} "${CMAKE_CURRENT_BINARY_DIR}/test_headercompile_${SHORTNAME}.cpp")
        set_property(TARGET test_headercompile_${SHORTNAME} PROPERTY FOLDER "Tests/Headers")
		target_link_libraries(test_headercompile_${SHORTNAME} luabind)
	endforeach()
endif()
