CXX	 = g++
CXXFLAGS = -Wall -ftemplate-depth-50 -Os

sources =\
	main.cpp\
	test_iterator.cpp\
	test_lua_classes.cpp\
	test_typetraits.cpp\
	test_construction.cpp\
	test_operators.cpp\
	test_attributes.cpp\
	test_exceptions.cpp\
	test_const.cpp\
	test_null_pointer.cpp\
	test_implicit_cast.cpp\
	test_policies.cpp\
	test_free_functions.cpp\
	test_object.cpp\
	test_held_type.cpp

target = test_luabind

#libs = -lluabind -llua -llualib -L/scratch/lua-5.0/lib -L../lib
libs = -lluabind -llua -llualib -L/usr/local/lib -L../lib

#inc = -I./ -I/scratch/lua-5.0/include -I- -I.. -I/scratch/boost_1_30_0
inc = -I./ -I/usr/local/include -I- -I..


RM = rm
AR = ar
MAKEDEPEND = $(CXX) -MM
DEPENDFILE = Makedepend

O_FILES = $(sources:.cpp=.o)

all: $(target)

$(target): $(O_FILES)
	$(CXX) $(CXXFLAGS) $^ $(inc) $(libs) -o $@

.cpp:
	$(CXX) $(CXXFLAGS) $(inc) -c -o $*.o $*.cpp

.cpp.o:
	$(CXX) $(CXXFLAGS) $(inc) -c -o $*.o $*.cpp

depend:
	touch $(DEPENDFILE)
	$(RM) $(DEPENDFILE)
	$(MAKEDEPEND) $(CXXFLAGS) $(inc) $(sources) > $(DEPENDFILE)

relink:
	touch $(target)
	$(RM) $(target)
	make

clean:
	touch core
	touch $(O_FILES)
	touch $(target)
	$(RM) $(O_FILES) $(target) core

include $(DEPENDFILE)
# DO NOT DELETE
