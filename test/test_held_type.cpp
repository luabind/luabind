#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <memory>

namespace luabind
{
	template<class T>
	T* get_pointer(boost::shared_ptr<T>& p) { return p.get(); }

	template<class T>
	T* get_pointer(const std::auto_ptr<T>& p) { return p.get(); }
}

#include "test.h"

namespace
{

	LUABIND_ANONYMOUS_FIX int feedback = 0;

	struct base
	{
		base(): n(4) { feedback = 3; }
		virtual ~base() { feedback = 1; }

		void f(int)
		{
			feedback = n;
		}

		int n;
	};

	// this is here to make sure the pointer offsetting works
	struct first_base
	{
		virtual void a() {}
		int padding;
	};

	struct derived: first_base, base
	{
		derived(): n2(7) { feedback = 7; }
		void f() { feedback = 5; }
		int n2;
	};

	void tester(base* t)
	{
		if (t->n == 4) feedback = 2;
	}

	void tester_(derived* t)
	{
		if (t->n2 == 7) feedback = 8;
	}

	void tester2(boost::shared_ptr<base> t)
	{
		if (t->n == 4) feedback = 9;
	}

	void tester3(boost::shared_ptr<const base> t)
	{
		if (t->n == 4) feedback = 10;
	}

	void tester4(const boost::shared_ptr<const base>& t)
	{
		if (t->n == 4) feedback = 11;
	}

	void tester5(const boost::shared_ptr<const base>* t)
	{
		if ((*t)->n == 4) feedback = 12;
	}

	void tester6(const boost::shared_ptr<base>* t)
	{
		if ((*t)->n == 4) feedback = 13;
	}

	void tester7(boost::shared_ptr<base>* t)
	{
		if ((*t)->n == 4) feedback = 14;
	}

	void tester8(boost::weak_ptr<base> t)
	{
		if (!t.expired()) feedback = 19;
	}

	boost::shared_ptr<base> tester9()
	{
		feedback = 20;
		return boost::shared_ptr<base>(new base());
	}

	struct tester10
	{
		tester10() : m_member(new base()) {}
		const boost::shared_ptr<base>& test() { return m_member; }
		boost::shared_ptr<base> m_member;
	};

	struct ownership {};

	void test_auto_ptr(std::auto_ptr<ownership>) {}

} // anonymous namespace


namespace luabind
{
	template<class A>
	LUABIND_TYPE_INFO get_const_holder(luabind::detail::type<boost::shared_ptr<A> >)
	{
		return LUABIND_TYPEID(boost::shared_ptr<const A>);
	}
}

#define LUABIND_IMPLICIT_CONVERSION(from, to, to_stripped)				\
	namespace luabind { namespace converters {								\
		yes_t is_user_defined(to);													\
		int match_lua_to_cpp(lua_State* L, to, int index)					\
		{																					\
			luabind::detail::default_policy										\
				::generate_converter<from, detail::lua_to_cpp>::type c;	\
			return c.match(L, LUABIND_DECORATE_TYPE(from), index);		\
		}																					\
																							\
		to_stripped convert_lua_to_cpp(lua_State* L, to, int index)		\
		{																					\
			luabind::detail::default_policy										\
				::generate_converter<from, detail::lua_to_cpp>::type c;	\
			return static_cast<to_stripped>(c.apply(L, 						\
						LUABIND_DECORATE_TYPE(from), index));					\
		}																					\
		}}

typedef boost::shared_ptr<base> t1;
typedef luabind::converters::by_value<boost::weak_ptr<base> > t2;
typedef boost::weak_ptr<base> t3;

//LUABIND_IMPLICIT_CONVERSION(t1,t2,t3)

namespace luabind { namespace converters
{
	template<class T>
	no_t is_user_defined(T);

	template<class T>
	int match_lua_to_cpp(lua_State*, T, int);

	template<class T>
	void convert_lua_to_cpp(lua_State*, T, int);
/*
	template<class To, class From>
	struct implicit_conversion
	{
		friend yes_t is_user_defined(by_value<To>);

		friend int match_lua_to_cpp(lua_State* L, by_value<To>, int index)
		{
			typename luabind::detail::default_policy
				::generate_converter<From, detail::lua_to_cpp>::type c;
			return c.match(L, by_value<From>(), index);
		}

		friend To convert_lua_to_cpp(lua_State* L, by_value<To>, int index)
		{
			typename luabind::detail::default_policy
				::generate_converter<From, detail::lua_to_cpp>::type c;
			return static_cast<To>(c.apply(L,
						by_value<From>(), index));
		}
	};
*/
//	implicit_conversion<t3,t1> casdsa;
}}

namespace test_adl
{
	struct adl_helper {};
	
	template<class T>
	struct A
	{
		friend int f(adl_helper, T) { return sizeof(T); }
	};

//	A<float> a;
};

bool test_held_type()
{
//	test_adl::A<float>();

//	f(test_adl::adl_helper(), 3.f);

	// This feature is not finished yet

	using namespace luabind;

//	converters::implicit_conversion<boost::weak_ptr<base>, boost::shared_ptr<base> >();

	boost::shared_ptr<base> ptr(new base());

	{
		lua_State* L = lua_open();
		lua_baselibopen(L);
		lua_closer c(L);
		int top = lua_gettop(L);

		open(L);

		int (a)[6];

		module(L)
		[
			class_<ownership, std::auto_ptr<ownership> >("ownership")
				.def(constructor<>()),

			def("test_auto_ptr", &test_auto_ptr),

			def("tester", &tester),
			def("tester", &tester_),
			def("tester2", &tester2),
			def("tester3", &tester3),
			def("tester4", &tester4),
			def("tester5", &tester5),
			def("tester6", &tester6),
			def("tester7", &tester7),
//			def("tester8", &tester8),
			def("tester9", &tester9),

			class_<base, boost::shared_ptr<base> >("base")
				.def(constructor<>())
				.def("f", &base::f),

			class_<derived, base, boost::shared_ptr<base> >("derived")
				.def(constructor<>())
				.def("f", &derived::f),

			class_<tester10>("tester10")
				.def(constructor<>())
				.def("test", &tester10::test)
		];

		lua_gettop(L);

		object g = get_globals(L);
		g["test"] = ptr;
		g["foobar"] = boost::shared_ptr<const base>(new base());

		if (dostring(L, "a = ownership()")) return false;
		if (dostring(L, "if a.__ok then print('1: ok!') end")) return false;
		if (dostring(L, "test_auto_ptr(a)")) return false;
		if (dostring(L, "if a.__ok then print('2: ok!') end")) return false;
		
		if (dostring(L, "if test.__ok then print('ok!') end")) return false;

		if (dostring(L, "tester(test)")) return false;
		if (feedback != 2) return false;
		if (dostring(L, "a = base()")) return false;
		if (feedback != 3) return false;
		if (dostring(L, "b = derived()")) return false;
		if (feedback != 7) return false;
		if (dostring(L, "tester(a)")) return false;
		if (feedback != 2) return false;
		if (dostring(L, "tester(b)")) return false;
		if (feedback != 8) return false;
		if (dostring(L, "tester2(b)")) return false;
		if (feedback != 9) return false;
		if (dostring(L, "tester3(b)")) return false;
		if (feedback != 10) return false;
		if (dostring(L, "tester4(b)")) return false;
		if (feedback != 11) return false;
		if (dostring(L, "tester5(b)")) return false;
		if (feedback != 12) return false;
		if (dostring(L, "tester6(b)")) return false;
		if (feedback != 13) return false;
		if (dostring(L, "tester7(b)")) return false;
		if (feedback != 14) return false;
//		if (dostring(L, "tester8(b)")) return false;
//		if (feedback != 19) return false;
		if (dostring(L, "c = tester9()")) return false;
		if (feedback != 3) return false;
		feedback= 0;

		if (dostring(L, "d = tester10()")) return false;
		if (dostring(L, "e = d:test()")) return false;
		if (feedback != 3) return false;


		if (top != lua_gettop(L)) return false;

	}

	ptr.reset();
	
	if (feedback != 1) return false;

	return true;
}

