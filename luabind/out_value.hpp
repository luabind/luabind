
template<int N>
struct char_array
{
	char storage[N];
};

template<class T>
struct indirect_sizeof
{
	template<class U>
	char_array<sizeof(U)> test(reference_to<U>);

	template<class U>
	char_array<sizeof(U)> test(const_reference_to<U>);

	template<class U>
	char_array<sizeof(U)> test(pointer_to<U>);

	template<class U>
	char_array<sizeof(U)> test(const_pointer_to<U>);

	template<class U>
	char_array<sizeof(U)> test(value_to<U>);

    BOOST_STATIC_CONSTANT(int, value = sizeof(test(LUABIND_DECORATE_TYPE(T))));
};

template<int N>
struct out_value_converter
{
	template<class T>
	static T& apply(lua_State* L, reference_to<T>, int index)
	{
		new (m_storage) T(primitive_param_converter::apply(L, value_to<T>(), index));
		return *reinterpret_cast<T*>(m_storage);
	}

	template<class T>
	static void postcall(lua_State* L, reference_to<T>, int)
	{
		primitive_result_converter::apply(L, *reinterpret_cast<T*>(m_storage));
	}

	template<class T>
	static int match(lua_State* L, reference_to<T>, int index)
	{
		return primitive_param_converter::match(L, value_to<T>(), index);
	}

	char m_storage[N];
};

template<class T>
struct out_value_policy
{
	BOOST_STATIC_CONSTANT(int, size = indirect_sizeof<T>::value);
	typedef out_value_converter<size> type;
};

template<int N>
struct out_value_policy_gen
{
	template<class T, class Direction>
	struct generate_policy
	{
		typedef typename mpl::apply_if<is_same<Direction, lua_to_cpp>
			, out_value_policy<T>
			, error
		>::type type;
	};
};
