function(add_test_executable _name)
    add_executable(${_name} main.cpp test_${_name}.cpp)
    target_link_libraries(${_name} luabind)
    add_test(${_name} ${_name})
endfunction()

function(add_test_try_compile _name)
    add_test(${_name} ${CMAKE_CXX_COMPILER} -I${CMAKE_SOURCE_DIR} ${cflags} -c test_${_name}.cpp -o
        output)
endfunction()

function(add_test_try_compile_fail _name)
    add_test(${_name} ${CMAKE_CXX_COMPILER} -I${CMAKE_SOURCE_DIR} ${cflags} -c test_${_name}.cpp -o
        output)
    set_property(TEST ${_name} PROPERTY WILL_FAIL TRUE)
endfunction()

include_directories(${LUA_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} 
                    ${CMAKE_SOURCE_DIR})

add_test_executable(back_reference)
add_test_executable(def_from_base)
add_test_executable(object)
add_test_executable(abstract_base)
add_test_executable(attributes)
add_test_executable(const)
add_test_executable(construction)
add_test_executable(exceptions)
add_test_executable(free_functions)
add_test_executable(held_type)
add_test_executable(implicit_cast)
add_test_executable(iterator)
add_test_executable(lua_classes)
add_test_executable(null_pointer)
add_test_executable(operators)
add_test_executable(policies)
add_test_executable(scope)
add_test_executable(separation)
add_test_executable(simple_class)
add_test_executable(yield)

add_test_try_compile(typetraits)
add_test_try_compile(value_wrapper)

# It's possible to test for compiling failure
add_test_try_compile_fail(has_get_pointer)
