# Embedded build for LuaBind
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com/
# Iowa State University HCI Graduate Program/VRAC

# ls test_*.cpp | sort | sed s/test_// | sed s/.cpp//
# remove typetraits value_wrapper and has_get_pointer because they are
# compile-only tests
set(TESTS
	abstract_base
	adopt
	adopt_wrapper
	attributes
	automatic_smart_ptr
	back_reference
	builtin_converters
	class_info
	collapse_converter
	const
	construction
	create_in_thread
	def_from_base
	dynamic_type
	exception_handlers
	exceptions
	extend_class_in_lua
	free_functions
	held_type
	implicit_cast
	implicit_raw
	iterator
	lua_classes
	null_pointer
	object
	operators
	policies
	private_destructors
	properties
	scope
	separation
	set_instance_value
	shadow
	shared_ptr
	simple_class
	smart_ptr_attributes
	super_leak
	table
	tag_function
	unsigned_int
	user_defined_converter
	vector_of_object
	virtual_inheritance
	yield)

add_library(test_main main.cpp)

foreach(test ${TESTS})
	add_executable(test_${test} test_${test}.cpp)
	target_link_libraries(test_${test} test_main luabind)
	add_test(NAME ${test} COMMAND test_${test})	
endforeach()
