include ../../config

all: filesystem

filesystem: filesystem.cpp
	$(CXX) -shared filesystem.cpp -O3 -o filesystem.dll filesystem.def $(LUA_PATH) -I./ -I- $(BOOST_PATH) -I../../ -L../../lib $(LUA_LIB_PATH) -lluabind -llua -llualib -L./ -lboost_filesystem

clean:
	rm -f filesystem.o

