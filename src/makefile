include ../config

SOURCES = \
	class_rep.cpp \
	function.cpp \
	find_best_match.cpp \
	implicit_cast.cpp \
	open.cpp

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = ../lib/libluabind.a





all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	$(AR) rcu $@ $(OBJECTS)
	ranlib $@

%.o:%.cpp
	$(CXX) -O3 -Wall -c $? -o $@ $(LUA_PATH) -I- -I.. $(BOOST_PATH) $(CONFIG)

clean:
	rm -f $(OBJECTS) $(TARGET)

