include_directories(
    ${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

set(luabind_SRCS
    class.cpp
    class_info.cpp
    class_registry.cpp
    class_rep.cpp
    create_class.cpp
    error.cpp
    exception_handler.cpp
    function.cpp
    function_introspection.cpp
    inheritance.cpp
    link_compatibility.cpp
    object_rep.cpp
    open.cpp
    operator.cpp
    pcall.cpp
    scope.cpp
    set_package_preload.cpp
    shared_ptr_converter.cpp
    stack_content_by_name.cpp
    weak_ref.cpp
    wrapper_base.cpp)

set(luabind_HDRS
    ../luabind/std_shared_ptr_converter.hpp
    ../luabind/adopt_policy.hpp
    ../luabind/back_reference.hpp
    ../luabind/back_reference_fwd.hpp
    ../luabind/class.hpp
    ../luabind/class_info.hpp
    ../luabind/config.hpp
    ../luabind/container_policy.hpp
    ../luabind/copy_policy.hpp
    ../luabind/dependency_policy.hpp
    ../luabind/discard_result_policy.hpp
    ../luabind/error.hpp
    ../luabind/error_callback_fun.hpp
    ../luabind/exception_handler.hpp
    ../luabind/from_stack.hpp
    ../luabind/function.hpp
    ../luabind/function_introspection.hpp
    ../luabind/get_main_thread.hpp
    ../luabind/get_pointer.hpp
    ../luabind/handle.hpp
    ../luabind/intrusive_ptr_converter.hpp
    ../luabind/iterator_policy.hpp
    ../luabind/luabind.hpp
    ../luabind/lua_state_fwd.hpp
    ../luabind/lua_include.hpp
    ../luabind/make_function.hpp
    ../luabind/nil.hpp
    ../luabind/no_dependency.hpp
    ../luabind/object.hpp
    ../luabind/object_fwd.hpp
    ../luabind/open.hpp
    ../luabind/operator.hpp
    ../luabind/out_value_policy.hpp
    ../luabind/prefix.hpp
    ../luabind/raw_policy.hpp
    ../luabind/return_reference_to_policy.hpp
    ../luabind/scope.hpp
    ../luabind/set_package_preload.hpp
    ../luabind/shared_ptr_converter.hpp
    ../luabind/tag_function.hpp
    ../luabind/typeid.hpp
    ../luabind/value_wrapper.hpp
    ../luabind/version.hpp
    ../luabind/weak_ref.hpp
    ../luabind/wrapper_base.hpp
    ../luabind/yield_policy.hpp)

set(luabind_detail_HDRS
    ../luabind/detail/call.hpp
    ../luabind/detail/call_function.hpp
    ../luabind/detail/call_member.hpp
    ../luabind/detail/call_operator_iterate.hpp
    ../luabind/detail/class_registry.hpp
    ../luabind/detail/class_rep.hpp
    ../luabind/detail/constructor.hpp
    ../luabind/detail/convert_to_lua.hpp
    ../luabind/detail/debug.hpp
    ../luabind/detail/decorate_type.hpp
    ../luabind/detail/deduce_signature.hpp
    ../luabind/detail/enum_maker.hpp
    ../luabind/detail/format_signature.hpp
    ../luabind/detail/garbage_collector.hpp
    ../luabind/detail/has_get_pointer.hpp
    ../luabind/detail/inheritance.hpp
    ../luabind/detail/instance_holder.hpp
    ../luabind/detail/link_compatibility.hpp
    ../luabind/detail/make_instance.hpp
    ../luabind/detail/most_derived.hpp
    ../luabind/detail/object_call.hpp
    ../luabind/detail/object_rep.hpp
    ../luabind/detail/open.hpp
    ../luabind/detail/operator_id.hpp
    ../luabind/detail/other.hpp
    ../luabind/detail/pcall.hpp
    ../luabind/detail/pointee_sizeof.hpp
    ../luabind/detail/policy.hpp
    ../luabind/detail/primitives.hpp
    ../luabind/detail/property.hpp
    ../luabind/detail/signature_match.hpp
    ../luabind/detail/stack_utils.hpp
    ../luabind/detail/typetraits.hpp
    ../luabind/detail/yes_no.hpp)

source_group("detail headers" FILES ${luabind_detail_HDRS})

set(CMAKE_DEBUG_POSTFIX "-d")
add_library(luabind ${luabind_SRCS} ${luabind_HDRS} ${luabind_detail_HDRS})

if (LUABIND_APPEND_VERSION_SUFFIX)
    set_property(TARGET luabind PROPERTY OUTPUT_NAME "luabind09")
else()
    # This is neccesary when the LUABIND_APPEND_VERSION_SUFFIX variable is
    # changed from ON to OFF.
    set_property(TARGET luabind PROPERTY OUTPUT_NAME "luabind")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND LUABIND_ENABLE_WARNINGS)
    # Clang complains about '#define LUABIND_BUILDING'.
    set_property(TARGET luabind
                 PROPERTY COMPILE_FLAGS "-Wno-unused-macros")
endif()

target_link_libraries(luabind ${LUA_LIBRARIES})

install(TARGETS luabind
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

set(ALLHEADERS ${luabind_HDRS} ${luabind_detail_HDRS} PARENT_SCOPE)
set(APIHEADERS ${luabind_HDRS} PARENT_SCOPE)
