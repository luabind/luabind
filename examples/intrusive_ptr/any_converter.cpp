template<class T>
struct convert_any
{
	static void convert(lua_State* L, const boost::any& a)
	{
		typename luabind::detail::default_converter::template generate_converter<T>::type conv;
		conv.apply(any_cast<T>(&a));
	}
};

std::map<const std::type_info*, void(*)(const boost::any&)> any_converters;

template<class T>
void register_any_converter()
{
	any_converters[&typeid(T)] = convert_any<T>::convert;
}

namespace luabind
{
	namespace converters
	{
		yes_t is_user_defined(by_value<boost::any>);
		yes_t is_user_defined(by_const_reference<boost::any>);

		void convert_cpp_to_lua(lua_State* L, const boost::any& a)
		{
			typedef void(*conv_t)(const boost::any&);
			conv_t conv = any_converters[&a.type()];
			conv(L, a);
		}
	}
}

boost::any f()
{
	return "foobar";
}

int main()
{
	register_any_converter<int>();
	register_any_converter<float>();
	register_any_converter<const char*>();
	register_any_converter<std::string>();
	register_any_converter<boost::intrusive_ptr<Actor> >();

	lua_State* L = lua_open();

	function(L, "f", &f);

	lua_close(L);
}

