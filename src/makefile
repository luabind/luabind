# the library's configuration is
# controlled through the CONFIG variable

#$(CONFIG) = \
#	-DLUABIND_DONT_COPY_STRINGS \
#	-DLUABIND_NO_EXCEPTIONS \
#	-DLUABIND_NO_ERROR_CHECKING

SRCS =	\
	class_rep.cpp \
	function.cpp \
	find_best_match.cpp \
	implicit_cast.cpp \
	open.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = ../lib/libluabind.a

all:	$(TARGET)

$(TARGET):	$(OBJS)
	$(AR) rcu $@ $(OBJS)
	ranlib $@

%.o:%.cpp
	g++ -c $? -o $@ -I- -I.. $(CONFIG)

clean:
	rm -f $(OBJS) $(TARGET)

