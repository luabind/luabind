#include "test.h"
#include <iostream>
#include <string>

extern "C"
{
	#include "lauxlib.h"
}

namespace
{
	using namespace luabind;
	LUABIND_ANONYMOUS_FIX int feedback1 = 0;
	LUABIND_ANONYMOUS_FIX int feedback2 = 0;
	LUABIND_ANONYMOUS_FIX int feedback3 = 0;
	LUABIND_ANONYMOUS_FIX int feedback4 = 0;
	LUABIND_ANONYMOUS_FIX int feedback5 = 0;

	void test_object_param(const object& table)
	{
		object current_object;
		current_object = table;
		
		if (table.type() == LUA_TTABLE)
		{
			feedback1 = 1;

			feedback2 = object_cast<int>(table["oh"]);

			feedback3 = 0;
			for (object::array_iterator i = table.abegin(); i != table.aend(); ++i)
			{
				feedback3 += object_cast<int>(*i);
			}

			feedback4 = 0;
			for (object::iterator j = table.begin(); j != table.end(); ++j)
			{
				feedback4 += object_cast<int>(*j);
			}

			feedback5 = 0;
			for (object::raw_iterator j = table.raw_begin(); j != table.raw_end(); ++j)
			{
				feedback5 += object_cast<int>(*j);
			}

			table["blurp"] = 5;

		}
		else
		{
			feedback1 = 2;

			if (table.type() != LUA_TNIL)
			{
				feedback2 = 1;
			}
			else
			{
				feedback2 = 2;
			}
		}
	}

	int test_fun()
	{
		feedback1 = 3;
		return 42;
	}

	struct test_param
	{
		virtual ~test_param() { feedback1 = 30; }
		virtual void foo()
		{
			feedback1 = 1;
		}
	};

	struct test_param2 : public test_param
	{
		virtual ~test_param2() { feedback1 = 20; }
		virtual void foo()
		{
			feedback1 = 2;
		}
		virtual void foo( bool b )
		{
			feedback2 = 2;
		}
	};

	void test_match(const luabind::object& o)
	{
		feedback1 = 28;
	}

	void test_match(int i)
	{
		feedback1 = 27;
	}

/*******************************************/
// Evan's diabolical test case
	class Message {};

	class IMessageClient
	{
	public:
		virtual bool	OnMessage( Message* pMessage )		= 0;
	};

	class IState
	{
	public:
		virtual bool			OnMessage( Message* pMessage )		= 0;
		virtual const char *	getName()							= 0;
	};
	class StateImpl	: public IState
	{
	public:
		virtual bool			OnMessage( Message* pMessage )		{ return false; }
		virtual const char *	getName()							{ return NULL; }
	};
	class FSM
		: public IMessageClient, public StateImpl
	{
	protected:
		IState*	m_pCurrentState;
		IState*	m_pNewState;
		IState*	m_pGlobalState;
		IState* m_pPreviousState;
		bool	m_bForceStateTransition;
	public:
		FSM() {}
		virtual ~FSM() {}
		IState * getCurrentState()							{ return m_pCurrentState; }
		IState * getNewState()								{ return m_pNewState; }
		IState * getPreviousState()							{ return m_pPreviousState; }
		IState * getGlobalState()							{ return m_pGlobalState; }

		virtual const char * getName()						{ return NULL; }
		virtual void	Update( float fDeltaTime )			{}

		virtual bool	OnMessage( Message* pMessage )		{ return true; }

		virtual void addState( IState * state )				{}
		virtual void addState( IState * state, const std::string & stateName ) {}
	};


	class ActorBrain  : public IMessageClient
	{
	public:
		ActorBrain()			{}
		virtual ~ActorBrain()	{}
		virtual void setActorParent(void* pParent)			{ m_p= pParent; }
		void*	getParentActor()							{ return m_p; }
		virtual void Update(float fDeltaTime) = 0;
	protected:
		void* m_p;
	};
	class AgentBrain
		: public ActorBrain, public FSM
	{
	public:
		AgentBrain()	{}
		virtual bool OnMessage(Message * pMessage ) { return true; }
		virtual void	Update( float fDeltaTime )	{}
	};

} // anonymous namespace



bool test_object()
{
	using namespace luabind;

	lua_State* L = lua_open();
	luaopen_base( L );

	int top = lua_gettop(L);
	{
		open(L);

		module(L)
		[
			def("test_object_param", &test_object_param),
			def("test_fun", &test_fun),
			def("test_match", (void(*)(const luabind::object&))&test_match),
			def("test_match", (void(*)(int))&test_match),
		
			class_<test_param>("test_param")
				.def(constructor<>())
				.def("foo",&test_param::foo ),
			class_<test_param2, test_param>("test_param2")
				.def(constructor<>())
				.def("foo",(void(test_param2::*)(bool))&test_param2::foo ),

			// Evan's diabolical test
			class_<IMessageClient>( "IMessageClient" )
				.def( "onMessage",			&IMessageClient::OnMessage )
			,
			class_<IState>( "State" )
			,

			class_<FSM, IState>( "FSM" )
				.def( constructor<>() )
				.def( "addState", (void(FSM::*)(IState*))&FSM::addState )//, adopt(_1) )
				.def( "addState", (void(FSM::*)(IState*,const std::string &))&FSM::addState )//, adopt(_1) )
			,
			class_<ActorBrain>( "ActorBrain" )
			,
			class_<AgentBrain, bases<ActorBrain, FSM> >( "AgentBrain" )
				.def( constructor<>() )
		];

		// Evan's diabolical test
		int nResult = lua_dofile( L, "ew_test.lua" );

		dostring(L, "t = test_param();");
		dostring(L, "t:foo();");
		if (feedback1 != 1) return false;

		dostring(L, "t = test_param2();");
		dostring(L, "t:foo();");
		dostring(L, "t:foo(true);");
		if (feedback1 != 2) return false;
		if (feedback2 != 2) return false;

		dostring(L, "t = 2");
		dostring(L, "test_object_param(t)");
		if (feedback1 != 2) return false;
		if (feedback2 != 1) return false;

		dostring(L, "test_object_param(nil)");
		if (feedback1 != 2) return false;
		if (feedback2 != 2) return false;

		dostring(L, "t = { ['oh'] = 4, 3, 5, 7, 13 }");
		dostring(L, "test_object_param(t)");
		if (feedback1 != 1) return false;
		if (feedback2 != 4) return false;
		if (feedback3 != 28) return false;
		if (feedback4 != 32) return false;
		if (feedback5 != 32) return false;

		object g = get_globals(L);

		object t = g["t"];
		if (t.type() != LUA_TTABLE) return false;

		object blurp = t["blurp"];
		if (object_cast<int>(blurp) != 5) return false;

		object fun = g["test_fun"];
		object ret = fun();
		if (object_cast<int>(ret) != 42) return false;
		if (feedback1 != 3) return false;

		dostring(L, "function test_param_policies(x, y) end");
		object test_param_policies = g["test_param_policies"];
		int a = test_param_policies.type();
		if (a != LUA_TFUNCTION) return false;
		// call the function and tell lua to adopt the pointer passed as first argument
		test_param_policies(5, new test_param())[adopt(_2)];

		dostring(L, "test_match(7)");
		if (feedback1 != 27) return false;
		dostring(L, "test_match('oo')");
		if (feedback1 != 28) return false;

		dostring(L, "function test_object_policies(a) glob = a\nreturn 6\nend");
		object test_object_policies = g["test_object_policies"];
		object ret_val = test_object_policies("teststring")[detail::null_type()];
		if (object_cast<int>(ret_val) != 6) return false;
		if (object_cast<std::string>(g["glob"]) != "teststring") return false;
		if (object_cast<std::string>(g.at("glob")) != "teststring") return false;
		if (object_cast<std::string>(g.raw_at("glob")) != "teststring") return false;
	}

	if (top != lua_gettop(L)) return false;

	lua_close(L);

	// make sure lua adopted the pointer by checking that it has been deleted
	if (feedback1 != 30) return false;

	return true;
}
