include ../config

SOURCES = \
	ref.cpp \
	scope.cpp \
	class.cpp \
	class_rep.cpp \
	function.cpp \
	find_best_match.cpp \
	implicit_cast.cpp \
	open.cpp \
	create_class.cpp \
	stack_content_by_name.cpp \
	object.cpp \
	class_info.cpp \
    class_registry

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = ../lib/libluabind.a





all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	$(AR) rcu $@ $(OBJECTS)
	ranlib $@

%.o:%.cpp
	$(CXX) -g -Wall -c $? -o $@ $(LUA_PATH) -I- -I.. $(BOOST_PATH) $(CONFIG)

clean:
	rm -f $(OBJECTS) $(TARGET)

