#include <iostream>

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

bool dostring(lua_State* L, const char* str)
{
	if (luaL_loadbuffer(L, str, std::strlen(str), str) || lua_pcall(L, 0, 0, 0))
	{
		const char* a = lua_tostring(L, -1);
		std::cout << a << "\n";
		lua_pop(L, 1);
		return true;
	}
	return false;
}

#define LUABIND_NO_HEADERS_ONLY

#include <luabind/luabind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "directory_iterator.hpp"

const boost::filesystem::path& 
identity(const boost::filesystem::path& x)
{
	return x;
}

void bind_filesystem(lua_State* L)
{
	using namespace luabind;
	using namespace boost::filesystem;

	module(L, "filesystem")
	[
		class_<path>("path")
			.def(constructor<>())
			.def(constructor<const char*>())
			.def("string", &path::string)
			.def("native_file_string", &path::native_file_string)
			.def("native_directory_string", &path::native_directory_string)
			.def("root_path", &path::root_path)
			.def("root_name", &path::root_name)
			.def("root_directory", &path::root_directory)
			.def("relative_path", &path::relative_path)
			.def("leaf", &path::leaf)
			.def("branch_path", &path::branch_path)

			.def("empty", &path::empty)
			.def("is_complete", &path::is_complete)
			.def("is_directory", &is_directory)
			.def("is_empty", &is_directory)
			.def("has_root_path", &path::has_root_path)
			.def("has_root_name", &path::has_root_name)
			.def("has_root_directory", &path::has_root_directory)
			.def("has_relative_path", &path::has_relative_path)
			.def("has_leaf", &path::has_leaf)
			.def("has_branch_path", &path::has_branch_path)

			.def(const_self / const_self)
			.def(other<const char*>() / const_self)
			.def(const_self / other<const char*>())

			.property("contents", &identity, return_directory_iterator)
			,

		def("exists", &exists),
		def("initial_path", &initial_path)
	];
}

int main(int argc, const char* argv[])
{
	lua_State* L = lua_open();
	luaopen_base(L);
	luaopen_string(L);
	luaopen_table(L);
	luaopen_math(L);
	luaopen_io(L);

	if (argc < 2)
	{
		std::cout << "usage: filesystem filename [args]\n";
		return 1;
	}
	
	using namespace luabind;

	open(L);

	bind_filesystem(L);
	
	object args = newtable(L);

	for (int i = 0; i < argc; ++i)
	{
		args[i + 1] = argv[i];
	}

	args["n"] = argc;

	object globals = get_globals(L);
	globals["args"] = args;

	lua_dofile(L, argv[1]);
}

