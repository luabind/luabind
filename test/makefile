include ../config

SOURCES = \
	test_abstract_base.cpp \
	test_attributes.cpp \
	test_const.cpp \
	test_construction.cpp \
	test_exceptions.cpp \
	test_free_functions.cpp \
	test_held_type.cpp \
	test_implicit_cast.cpp \
	test_iterator.cpp \
	test_lua_classes.cpp \
	test_null_pointer.cpp \
	test_object.cpp \
	test_operators.cpp \
	test_policies.cpp \
	test_scope.cpp \
	test_separation.cpp \
	test_simple_class.cpp \
	test_typetraits.cpp \
	test_yield.cpp \
	test_null_pointer.cpp \
	test_policies.cpp \
	test_free_functions.cpp \
	test_iterator.cpp \


all: $(SOURCES:.cpp=.exe)

%.exe:%.o main.o
	$(CXX) $? -o $@ -g -L$(LUA_PATH)/lib -L../lib -lluabind -llualib -llua

%.o:%.cpp
	$(CXX) -Wall -ftemplate-depth-100 -g $(CONFIG) -I$(LUA_PATH)/include -I. -I- -I../ -I$(BOOST_ROOT) -c $? -o $@

#just to force the tests to be run
test: $(SOURCES:.cpp=.dummy)

%.dummy:%.exe
	./$?

clean:
	rm -f $(SOURCES:.cpp=.o) $(SOURCES:.cpp=.exe) main.o core

