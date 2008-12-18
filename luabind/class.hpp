// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef LUABIND_CLASS_HPP_INCLUDED
#define LUABIND_CLASS_HPP_INCLUDED

/*
	ISSUES:
	------------------------------------------------------

	* solved for member functions, not application operator *
	if we have a base class that defines a function a derived class must be able to
	override that function (not just overload). Right now we just add the other overload
	to the overloads list and will probably get an ambiguity. If we want to support this
	each method_rep must include a vector of type_info pointers for each parameter.
	Operators do not have this problem, since operators always have to have
	it's own type as one of the arguments, no ambiguity can occur. Application
	operator, on the other hand, would have this problem.
	Properties cannot be overloaded, so they should always be overridden.
	If this is to work for application operator, we really need to specify if an application
	operator is const or not.

	If one class registers two functions with the same name and the same
	signature, there's currently no error. The last registered function will
	be the one that's used.
	How do we know which class registered the function? If the function was
	defined by the base class, it is a legal operation, to override it.
	we cannot look at the pointer offset, since it always will be zero for one of the bases.



	TODO:
	------------------------------------------------------

 	finish smart pointer support
		* the adopt policy should not be able to adopt pointers to held_types. This
		must be prohibited.
		* name_of_type must recognize holder_types and not return "custom"

	document custom policies, custom converters

	store the instance object for policies.

	support the __concat metamethod. This is a bit tricky, since it cannot be
	treated as a normal operator. It is a binary operator but we want to use the
	__tostring implementation for both arguments.
	
*/

#include <luabind/prefix.hpp>
#include <luabind/config.hpp>

#include <string>
#include <map>
#include <vector>
#include <cassert>

#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/apply.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/logical.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/logical.hpp>

#include <luabind/config.hpp>
#include <luabind/scope.hpp>
#include <luabind/raw_policy.hpp>
#include <luabind/back_reference.hpp>
#include <luabind/function.hpp>
#include <luabind/dependency_policy.hpp>
#include <luabind/detail/constructor.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/detail/deduce_signature.hpp>
#include <luabind/detail/compute_score.hpp>
#include <luabind/detail/primitives.hpp>
#include <luabind/detail/property.hpp>
#include <luabind/detail/typetraits.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/detail/construct_rep.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/calc_arity.hpp>
#include <luabind/detail/call_member.hpp>
#include <luabind/detail/enum_maker.hpp>
#include <luabind/detail/get_signature.hpp>
#include <luabind/detail/implicit_cast.hpp>
#include <luabind/detail/operator_id.hpp>
#include <luabind/detail/pointee_typeid.hpp>
#include <luabind/detail/link_compatibility.hpp>

// to remove the 'this' used in initialization list-warning
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4355)
#endif

namespace boost
{

  template <class T> class shared_ptr;

} // namespace boost

namespace luabind
{	
	namespace detail
	{
		struct unspecified {};

		template<class Derived> struct operator_;

		struct you_need_to_define_a_get_const_holder_function_for_your_smart_ptr {};
	}

	template<class T, class X1 = detail::unspecified, class X2 = detail::unspecified, class X3 = detail::unspecified>
	struct class_;

	// TODO: this function will only be invoked if the user hasn't defined a correct overload
	// maybe we should have a static assert in here?
	inline detail::you_need_to_define_a_get_const_holder_function_for_your_smart_ptr*
	get_const_holder(...)
	{
		return 0;
	}

	template <class T>
	boost::shared_ptr<T const>* get_const_holder(boost::shared_ptr<T>*)
	{
		return 0;
	}

	namespace detail
	{
		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, class A)>
		double is_bases_helper(const bases<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, A)>&);

#ifndef BOOST_MSVC
		template<class T>
		char is_bases_helper(const T&);
#else
		char is_bases_helper(...);
#endif

		template<class T>
		struct is_bases
		{
			static const T& t;

			BOOST_STATIC_CONSTANT(bool, value = sizeof(is_bases_helper(t)) == sizeof(double));
			typedef boost::mpl::bool_<value> type;
			BOOST_MPL_AUX_LAMBDA_SUPPORT(1,is_bases,(T))
		};

		double is_not_unspecified_helper(const unspecified*);
		char is_not_unspecified_helper(...);

		template<class T>
		struct is_not_unspecified
		{
			BOOST_STATIC_CONSTANT(bool, value = sizeof(is_not_unspecified_helper(static_cast<T*>(0))) == sizeof(char));
			typedef boost::mpl::bool_<value> type;
			BOOST_MPL_AUX_LAMBDA_SUPPORT(1,is_not_unspecified,(T))
		};

		template<class Predicate>
		struct get_predicate
		{
			typedef typename boost::mpl::and_<
			  	is_not_unspecified<boost::mpl::_1>
			  , Predicate
			> type;
		};

		template<class Parameters, class Predicate, class DefaultValue>
		struct extract_parameter
		{
			typedef typename get_predicate<Predicate>::type pred;
			typedef typename boost::mpl::find_if<Parameters, pred>::type iterator;
			typedef typename boost::mpl::eval_if<
				boost::is_same<
					iterator
				  , typename boost::mpl::end<Parameters>::type
				>
			  , boost::mpl::identity<DefaultValue>
			  , boost::mpl::deref<iterator>
			>::type type;
		};

		template<class Fn, class Class, class Policies>
		struct mem_fn_callback
		{
			typedef int result_type;

			int operator()(lua_State* L) const
			{
				return invoke(L, fn, deduce_signature(fn, (Class*)0), Policies());
			}

			mem_fn_callback(Fn fn_)
				: fn(fn_)
			{
			}

			Fn fn;
		};

		template<class Fn, class Class, class Policies>
		struct mem_fn_matcher
		{
			typedef int result_type;

			int operator()(lua_State* L) const
			{
				return compute_score(L, deduce_signature(fn, (Class*)0), Policies());
			}

			mem_fn_matcher(Fn fn_)
				: fn(fn_)
			{
			}

			Fn fn;
		};

		struct pure_virtual_tag
		{
			static void precall(lua_State*, index_map const&) {}
			static void postcall(lua_State*, index_map const&) {}
		};

		template<class Policies>
		struct has_pure_virtual
		{
			typedef typename boost::mpl::eval_if<
				boost::is_same<pure_virtual_tag, typename Policies::head>
			  , boost::mpl::true_
			  , has_pure_virtual<typename Policies::tail>
			>::type type;

			BOOST_STATIC_CONSTANT(bool, value = type::value);
		};

		template<>
		struct has_pure_virtual<null_type>
		{
			BOOST_STATIC_CONSTANT(bool, value = false);
			typedef boost::mpl::bool_<value> type;
		};

		// prints the types of the values on the stack, in the
		// range [start_index, lua_gettop()]

		LUABIND_API std::string stack_content_by_name(lua_State* L, int start_index);
	
		struct LUABIND_API create_class
		{
			static int stage1(lua_State* L);
			static int stage2(lua_State* L);
		};

		// if the class is held by a smart pointer, we need to be able to
		// implicitly dereference the pointer when needed.

		template<class UnderlyingT, class HeldT>
		struct extract_underlying_type
		{
			static void* extract(void* ptr)
			{
				HeldT& held_obj = *reinterpret_cast<HeldT*>(ptr);
				UnderlyingT* underlying_ptr = static_cast<UnderlyingT*>(get_pointer(held_obj));
				return underlying_ptr;
			}
		};

		template<class UnderlyingT, class HeldT>
		struct extract_underlying_const_type
		{
			static const void* extract(void* ptr)
			{
				HeldT& held_obj = *reinterpret_cast<HeldT*>(ptr);
				const UnderlyingT* underlying_ptr = static_cast<const UnderlyingT*>(get_pointer(held_obj));
				return underlying_ptr;
			}
		};

		template<class HeldType>
		struct internal_holder_extractor
		{
			typedef void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return &detail::extract_underlying_type<T, HeldType>::extract;
			}
		};

		template<>
		struct internal_holder_extractor<detail::null_type>
		{
			typedef void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return 0;
			}
		};


		template<class HeldType, class ConstHolderType>
		struct convert_holder
		{
			static void apply(void* holder, void* target)
			{
				new(target) ConstHolderType(*reinterpret_cast<HeldType*>(holder));
			};
		};


		template<class HeldType>
		struct const_converter
		{
			typedef void(*converter_fun)(void*, void*);

			template<class ConstHolderType>
			static converter_fun apply(ConstHolderType*)
			{
				return &detail::convert_holder<HeldType, ConstHolderType>::apply;
			}
		};

		template<>
		struct const_converter<detail::null_type>
		{
			typedef void(*converter_fun)(void*, void*);

			template<class T>
			static converter_fun apply(T*)
			{
				return 0;
			}
		};




		template<class HeldType>
		struct internal_const_holder_extractor
		{
			typedef const void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return get_extractor(detail::type_<T>(), get_const_holder(static_cast<HeldType*>(0)));
			}
		private:
			template<class T, class ConstHolderType>
			static extractor_fun get_extractor(detail::type_<T>, ConstHolderType*)
			{
				return &detail::extract_underlying_const_type<T, ConstHolderType>::extract;
			}
		};

		template<>
		struct internal_const_holder_extractor<detail::null_type>
		{
			typedef const void*(*extractor_fun)(void*);

			template<class T>
			static extractor_fun apply(detail::type_<T>)
			{
				return 0;
			}
		};



		// this is simply a selector that returns the type_info
		// of the held type, or invalid_type_info if we don't have
		// a held_type
		template<class HeldType>
		struct internal_holder_type
		{
			static LUABIND_TYPE_INFO apply()
			{
				return LUABIND_TYPEID(HeldType);
			}
		};

		template<>
		struct internal_holder_type<detail::null_type>
		{
			static LUABIND_TYPE_INFO apply()
			{
				return LUABIND_INVALID_TYPE_INFO;
			}
		};


		// this is the actual held_type constructor
		template<class HeldType, class T>
		struct internal_construct_holder
		{
			static void apply(void* target, void* raw_pointer)
			{
				new(target) HeldType(static_cast<T*>(raw_pointer));
			}
		};

		// this is the actual held_type default constructor
		template<class HeldType, class T>
		struct internal_default_construct_holder
		{
			static void apply(void* target)
			{
				new(target) HeldType();
			}
		};

		// the following two functions are the ones that returns
		// a pointer to a held_type_constructor, or 0 if there
		// is no held_type
		template<class HeldType>
		struct holder_constructor
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return &internal_construct_holder<HeldType, T>::apply;
			}
		};

		template<>
		struct holder_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};

		// the following two functions are the ones that returns
		// a pointer to a const_held_type_constructor, or 0 if there
		// is no held_type
		template<class HolderType>
		struct const_holder_constructor
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return get_const_holder_constructor(detail::type_<T>(), get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class T, class ConstHolderType>
			static constructor get_const_holder_constructor(detail::type_<T>, ConstHolderType*)
			{
				return &internal_construct_holder<ConstHolderType, T>::apply;
			}
		};

		template<>
		struct const_holder_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*,void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};


		// the following two functions are the ones that returns
		// a pointer to a held_type_constructor, or 0 if there
		// is no held_type. The holder_type is default constructed
		template<class HeldType>
		struct holder_default_constructor
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return &internal_default_construct_holder<HeldType, T>::apply;
			}
		};

		template<>
		struct holder_default_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};


		// the following two functions are the ones that returns
		// a pointer to a const_held_type_constructor, or 0 if there
		// is no held_type. The constructed held_type is default
		// constructed
		template<class HolderType>
		struct const_holder_default_constructor
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return get_const_holder_default_constructor(detail::type_<T>(), get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class T, class ConstHolderType>
			static constructor get_const_holder_default_constructor(detail::type_<T>, ConstHolderType*)
			{
				return &internal_default_construct_holder<ConstHolderType, T>::apply;
			}
		};

		template<>
		struct const_holder_default_constructor<detail::null_type>
		{
			typedef void(*constructor)(void*);
			template<class T>
			static constructor apply(detail::type_<T>)
			{
				return 0;
			}
		};




		// this is a selector that returns the size of the held_type
		// or 0 if we don't have a held_type
		template <class HolderType>
		struct internal_holder_size
		{
			static int apply() { return get_internal_holder_size(get_const_holder(static_cast<HolderType*>(0))); }
		private:
			template<class ConstHolderType>
			static int get_internal_holder_size(ConstHolderType*)
			{
				return max_c<sizeof(HolderType), sizeof(ConstHolderType)>::value;
			}
		};

		template <>
		struct internal_holder_size<detail::null_type>
		{
			static int apply() {	return 0; }
		};


		// if we have a held type, return the destructor to it
		// note the difference. The held_type should only be destructed (not deleted)
		// since it's constructed in the lua userdata
		template<class HeldType>
		struct internal_holder_destructor
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return &detail::destruct_only_s<HeldType>::apply;
			}
		};

		// if we don't have a held type, return the destructor of the raw type
		template<>
		struct internal_holder_destructor<detail::null_type>
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return &detail::delete_s<T>::apply;
			}
		};

		
		// if we have a held type, return the destructor to it's const version
		template<class HolderType>
		struct internal_const_holder_destructor
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return const_holder_type_destructor(get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class ConstHolderType>
			static destructor_t const_holder_type_destructor(ConstHolderType*)
			{
				return &detail::destruct_only_s<ConstHolderType>::apply;
			}

		};

		// if we don't have a held type, return the destructor of the raw type
		template<>
		struct internal_const_holder_destructor<detail::null_type>
		{
			typedef void(*destructor_t)(void*);
			template<class T>
			static destructor_t apply(detail::type_<T>)
			{
				return 0;
			}
		};




		template<class HolderType>
		struct get_holder_alignment
		{
			static int apply()
			{
				return internal_alignment(get_const_holder(static_cast<HolderType*>(0)));
			}

		private:

			template<class ConstHolderType>
			static int internal_alignment(ConstHolderType*)
			{
				return detail::max_c<boost::alignment_of<HolderType>::value
					, boost::alignment_of<ConstHolderType>::value>::value;
			}
		};

		template<>
		struct get_holder_alignment<detail::null_type>
		{
			static int apply()
			{
				return 1;
			}
		};


	} // detail

	namespace detail {

		template<class T>
		struct static_scope
		{
			static_scope(T& self_) : self(self_)
			{
			}

			T& operator[](scope s) const
			{
				self.add_inner_scope(s);
				return self;
			}

		private:
			template<class U> void operator,(U const&) const;
			void operator=(static_scope const&);
			
			T& self;
		};

		struct class_registration;

		struct LUABIND_API class_base : scope
		{
		public:
			class_base(char const* name);		

			struct base_desc
			{
				LUABIND_TYPE_INFO type;
				int ptr_offset;
			};

			void init(
				LUABIND_TYPE_INFO type
				, LUABIND_TYPE_INFO holder_type
				, LUABIND_TYPE_INFO const_holder_type
				, void*(*extractor)(void*)
				, const void*(*const_extractor)(void*)
				, void(*const_converter)(void*,void*)
				, void(*holder_constructor)(void*,void*)
				, void(*const_holder_constructor)(void*,void*)
				, void(*holder_default_constructor)(void*)
				, void(*const_holder_default_constructor)(void*)
				, void(*adopt_fun)(void*)
				, void(*destructor)(void*)
				, void(*const_holder_destructor)(void*)
				, int holder_size
				, int holder_alignment);

			void add_getter(
				const char* name
				, const boost::function2<int, lua_State*, int>& g);

#ifdef LUABIND_NO_ERROR_CHECKING
			void add_setter(
				const char* name
				, const boost::function2<int, lua_State*, int>& s);
#else
			void add_setter(
				const char* name
				, const boost::function2<int, lua_State*, int>& s
				, int (*match)(lua_State*, int)
				, void (*get_sig_ptr)(lua_State*, std::string&));
#endif

			void add_base(const base_desc& b);
			void add_constructor(const detail::construct_rep::overload_t& o);	

#ifndef LUABIND_NO_ERROR_CHECKING
			void add_operator(
				int op_id
				,  int(*func)(lua_State*)
				, int(*matcher)(lua_State*)
				, void(*sig)(lua_State*
				, std::string&)
				, int arity);
#else
			void add_operator(
				int op_id
				,  int(*func)(lua_State*)
				, int(*matcher)(lua_State*)
				, int arity);
#endif

			void add_member(registration* member);
			void add_default_member(registration* member);

			const char* name() const;

			void add_static_constant(const char* name, int val);
			void add_inner_scope(scope& s);

		private:
			class_registration* m_registration;
		};
	
        template<class T, class W>
        struct adopt_function
		{
		    static void execute(void* p)
            {
                if (W* wrapper = dynamic_cast<W*>(static_cast<T*>(p)))
                {
                    wrapped_self_t& self = wrap_access::ref(*wrapper);

                    LUABIND_CHECK_STACK(self.state());

                    self.get(self.state());
                    self.m_strong_ref.set(self.state());
                }
            }
        };

		template <class Class, class F, class Policies>
		struct memfun_registration : registration
		{
			memfun_registration(char const* name, F f, Policies const& policies)
			  : name(name)
			  , f(f)
			  , policies(policies)
			{}

			void register_(lua_State* L) const
			{
				object fn = make_function(
					L, f, deduce_signature(f, (Class*)0), policies);

				add_overload(
					object(from_stack(L, -1))
				  , name
				  , fn
				);
			}

			char const* name;
			F f;
			Policies policies;
		};

        template <class Class, class Signature, class Policies>
        struct constructor_registration : registration
        {
            constructor_registration(Policies const& policies)
              : policies(policies)
            {}

            void register_(lua_State* L) const
            {
                object fn = make_function(
                    L, construct<Class, Signature>(), Signature(), policies);

                add_overload(
                    object(from_stack(L, -1))
                  , "__init"
                  , fn
                );
            }

            Policies policies;
        };

        template <class T>
        struct reference_result
          : mpl::if_<
                mpl::or_<boost::is_pointer<T>, is_primitive<T> >
              , T
              , typename boost::add_reference<T>::type
            >
        {};

        template <class T, class Policies>
        struct inject_dependency_policy
          : mpl::if_<
                is_primitive<T>
              , Policies
              , policy_cons<dependency_policy<0, 1>, Policies>
            >
        {};

        template <
            class Class
          , class Get, class GetPolicies
          , class Set = null_type, class SetPolicies = null_type
        >
        struct property_registration : registration
        {
            property_registration(
                char const* name
              , Get const& get
              , GetPolicies const& get_policies
              , Set const& set = Set()
              , SetPolicies const& set_policies = SetPolicies()
            )
              : name(name)
              , get(get)
              , get_policies(get_policies)
              , set(set)
              , set_policies(set_policies)
            {}

            void register_(lua_State* L) const
            {
                object context(from_stack(L, -1));
                register_aux(
                    L
                  , context
                  , make_get(L, get, boost::is_member_object_pointer<Get>())
                  , set
                );
            }

            template <class F>
            object make_get(lua_State* L, F const& f, mpl::false_) const
            {
                return make_function(
                    L, f, deduce_signature(f, (Class*)0), get_policies);
            }

            template <class T, class D>
            object make_get(lua_State* L, D T::* mem_ptr, mpl::true_) const
            {
                typedef typename reference_result<D>::type result_type;
                typedef typename inject_dependency_policy<
                    D, GetPolicies>::type policies;

                return make_function(
                    L
                  , access_member_ptr<T, D, result_type>(mem_ptr)
                  , mpl::vector2<result_type, Class const&>()
                  , policies()
                );
            }

            template <class F>
            object make_set(lua_State* L, F const& f, mpl::false_) const
            {
                return make_function(
                    L, f, deduce_signature(f, (Class*)0), set_policies);
            }

            template <class T, class D>
            object make_set(lua_State* L, D T::* mem_ptr, mpl::true_) const
            {
                return make_function(
                    L
                  , access_member_ptr<T, D>(mem_ptr)
                  , mpl::vector3<void, Class&, D const&>()
                  , set_policies
                );
            }

            template <class S>
            void register_aux(
                lua_State* L, object const& context
              , object const& get_, S const&) const
            {
                context[name] = property(
                    get_
                  , make_set(L, set, boost::is_member_object_pointer<Set>())
                );
            }

            void register_aux(
                lua_State* L, object const& context
              , object const& get_, null_type) const
            {
                context[name] = property(get_);
            }

            char const* name;
            Get get;
            GetPolicies get_policies;
            Set set;
            SetPolicies set_policies;
        };

	} // namespace detail

	// registers a class in the lua environment
	template<class T, class X1, class X2, class X3>
	struct class_: detail::class_base 
	{
		typedef class_<T, X1, X2, X3> self_t;

	private:

		template<class A, class B, class C, class D>
		class_(const class_<A,B,C,D>&);

	public:

		// WrappedType MUST inherit from T
		typedef typename detail::extract_parameter<
		    boost::mpl::vector3<X1,X2,X3>
		  , boost::is_base_and_derived<T, boost::mpl::_>
		  , detail::null_type
		>::type WrappedType;

		typedef typename detail::extract_parameter<
		    boost::mpl::list3<X1,X2,X3>
		  , boost::mpl::not_<
		        boost::mpl::or_<
				    boost::mpl::or_<
					    detail::is_bases<boost::mpl::_>
					  , boost::is_base_and_derived<boost::mpl::_, T>
					>
				  , boost::is_base_and_derived<T, boost::mpl::_>
				>
			>
		  , detail::null_type
		>::type HeldType;

		// this function generates conversion information
		// in the given class_rep structure. It will be able
		// to implicitly cast to the given template type
		template<class To>
		void gen_base_info(detail::type_<To>)
		{
			// fist, make sure the given base class is registered.
			// if it's not registered we can't push it's lua table onto
			// the stack because it doesn't have a table

			// try to cast this type to the base type and remember
			// the pointer offset. For multiple inheritance the pointer
			// may change when casting. Since we need to be able to
			// cast we need this pointer offset.
			// store the information in this class' base class-vector
			base_desc base;
			base.type = LUABIND_TYPEID(To);
			base.ptr_offset = detail::ptr_offset(detail::type_<T>(), detail::type_<To>());
			add_base(base);
		}

		void gen_base_info(detail::type_<detail::null_type>)
		{}

#define LUABIND_GEN_BASE_INFO(z, n, text) gen_base_info(detail::type_<B##n>());

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, class B)>
		void generate_baseclass_list(detail::type_<bases<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_BASES, B)> >)
		{
			BOOST_PP_REPEAT(LUABIND_MAX_BASES, LUABIND_GEN_BASE_INFO, _)
		}

#undef LUABIND_GEN_BASE_INFO

		class_(const char* name): class_base(name), scope(*this)
		{
#ifndef NDEBUG
			detail::check_link_compatibility();
#endif
		   	init(); 
		}

		template<class F>
		class_& def(const char* name, F f)
		{
			return this->virtual_def(
				name, f, detail::null_type()
			  , detail::null_type(), boost::mpl::true_());
		}

		// virtual functions
		template<class F, class DefaultOrPolicies>
		class_& def(char const* name, F fn, DefaultOrPolicies default_or_policies)
		{
			return this->virtual_def(
				name, fn, default_or_policies, detail::null_type()
			  , LUABIND_MSVC_TYPENAME detail::is_policy_cons<DefaultOrPolicies>::type());
		}

		template<class F, class Default, class Policies>
		class_& def(char const* name, F fn
			, Default default_, Policies const& policies)
		{
			return this->virtual_def(
				name, fn, default_
			  , policies, boost::mpl::false_());
		}

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A)>
		class_& def(constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)> sig)
		{
            return this->def_constructor(&sig, detail::null_type());
		}

		template<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, class A), class Policies>
		class_& def(constructor<BOOST_PP_ENUM_PARAMS(LUABIND_MAX_ARITY, A)> sig, const Policies& policies)
		{
            return this->def_constructor(&sig, policies);
		}

        template <class Getter>
        class_& property(const char* name, Getter g)
        {
            this->add_member(
                new detail::property_registration<T, Getter, detail::null_type>(
                    name, g, detail::null_type()));
            return *this;
        }

        template <class Getter, class MaybeSetter>
        class_& property(const char* name, Getter g, MaybeSetter s)
        {
            return property_impl(
                name, g, s
              , boost::mpl::bool_<detail::is_policy_cons<MaybeSetter>::value>()
            );
        }

        template<class Getter, class Setter, class GetPolicies>
        class_& property(const char* name, Getter g, Setter s, const GetPolicies& get_policies)
        {
            typedef detail::property_registration<
                T, Getter, GetPolicies, Setter, detail::null_type
            > registration_type;

            this->add_member(
                new registration_type(name, g, get_policies, s));
            return *this;
        }

        template<class Getter, class Setter, class GetPolicies, class SetPolicies>
        class_& property(
            const char* name
          , Getter g, Setter s
          , GetPolicies const& get_policies
          , SetPolicies const& set_policies)
        {
            typedef detail::property_registration<
                T, Getter, GetPolicies, Setter, SetPolicies
            > registration_type;

            this->add_member(
                new registration_type(name, g, get_policies, s, set_policies));
            return *this;
        }

        template <class C, class D>
        class_& def_readonly(const char* name, D C::*mem_ptr)
        {
            typedef detail::property_registration<T, D C::*, detail::null_type>
                registration_type;

            this->add_member(
                new registration_type(name, mem_ptr, detail::null_type()));
            return *this;
        }

        template <class C, class D, class Policies>
        class_& def_readonly(const char* name, D C::*mem_ptr, Policies const& policies)
        {
            typedef detail::property_registration<T, D C::*, Policies>
                registration_type;

            this->add_member(
                new registration_type(name, mem_ptr, policies));
            return *this;
        }

        template <class C, class D>
        class_& def_readwrite(const char* name, D C::*mem_ptr)
        {
            typedef detail::property_registration<
                T, D C::*, detail::null_type, D C::*
            > registration_type;

            this->add_member(
                new registration_type(
                    name, mem_ptr, detail::null_type(), mem_ptr));
            return *this;
        }

        template <class C, class D, class GetPolicies>
        class_& def_readwrite(
            const char* name, D C::*mem_ptr, GetPolicies const& get_policies)
        {
            typedef detail::property_registration<
                T, D C::*, GetPolicies, D C::*
            > registration_type;

            this->add_member(
                new registration_type(
                    name, mem_ptr, get_policies, mem_ptr));
            return *this;
        }

        template <class C, class D, class GetPolicies, class SetPolicies>
        class_& def_readwrite(
            const char* name
          , D C::*mem_ptr
          , GetPolicies const& get_policies
          , SetPolicies const& set_policies
        )
        {
            typedef detail::property_registration<
                T, D C::*, GetPolicies, D C::*, SetPolicies
            > registration_type;

            this->add_member(
                new registration_type(
                    name, mem_ptr, get_policies, mem_ptr, set_policies));
            return *this;
        }

		template<class Derived, class Policies>
		class_& def(detail::operator_<Derived>, Policies const& policies)
		{
			return this->def(
				Derived::name()
			  , &Derived::template apply<T, Policies>::execute
			  , raw(_1) + policies
			);
		}

		template<class Derived>
		class_& def(detail::operator_<Derived>)
		{
			return this->def(
				Derived::name()
			  , &Derived::template apply<T, detail::null_type>::execute
			  , raw(_1)
			);
		}

/*
		template<class op_id, class Left, class Right, class Policies>
		class_& def(detail::operator_<op_id, Left, Right>, const Policies& policies)
		{
			typedef typename detail::operator_unwrapper<Policies, op_id, T, Left, Right> op_type;
#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, &detail::get_signature<constructor<typename op_type::left_t, typename op_type::right_t> >::apply
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#else
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#endif
			return *this;
		}

		template<class op_id, class Left, class Right>
		class_& def(detail::operator_<op_id, Left, Right>)
		{
			typedef typename detail::operator_unwrapper<detail::null_type, op_id, T, Left, Right> op_type;

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, &detail::get_signature<constructor<LUABIND_MSVC_TYPENAME op_type::left_t, LUABIND_MSVC_TYPENAME op_type::right_t> >::apply
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#else
			add_operator(op_type::get_id()
									, &op_type::execute
									, &op_type::match
									, detail::is_unary(op_type::get_id()) ? 1 : 2);
#endif
			return *this;
		}

		template<class Signature, bool Constant>
		class_& def(detail::application_operator<Signature, Constant>*)
		{
			typedef detail::application_operator<Signature, Constant, detail::null_type> op_t;

			int arity = detail::calc_arity<Signature::arity>::apply(
				Signature(), static_cast<detail::null_type*>(0));

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, &detail::get_signature<Signature>::apply
				, arity + 1);
#else
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, arity + 1);
#endif

			return *this;
		}

		template<class Signature, bool Constant, class Policies>
		class_& def(detail::application_operator<Signature, Constant>*, const Policies& policies)
		{
			typedef detail::application_operator<Signature, Constant, Policies> op_t;

			int arity = detail::calc_arity<Signature::arity>::apply(Signature(), static_cast<Policies*>(0));

#ifndef LUABIND_NO_ERROR_CHECKING
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, &detail::get_signature<Signature>::apply
				, arity + 1);
#else
			add_operator(
				detail::op_call
				, &op_t::template apply<T>::execute
				, &op_t::match
				, arity + 1);
#endif

			return *this;
		}
*/
		detail::enum_maker<self_t> enum_(const char*)
		{
			return detail::enum_maker<self_t>(*this);
		}
		
		detail::static_scope<self_t> scope;
		
	private:
		void operator=(class_ const&);

		void init()
		{
			typedef typename detail::extract_parameter<
					boost::mpl::list3<X1,X2,X3>
				,	boost::mpl::or_<
							detail::is_bases<boost::mpl::_>
						,	boost::is_base_and_derived<boost::mpl::_, T>
					>
				,	no_bases
			>::type bases_t;

			typedef typename 
				boost::mpl::if_<detail::is_bases<bases_t>
					,	bases_t
					,	bases<bases_t>
				>::type Base;
	
			class_base::init(LUABIND_TYPEID(T)
				, detail::internal_holder_type<HeldType>::apply()
				, detail::pointee_typeid(
					get_const_holder(static_cast<HeldType*>(0)))
				, detail::internal_holder_extractor<HeldType>::apply(detail::type_<T>())
				, detail::internal_const_holder_extractor<HeldType>::apply(detail::type_<T>())
				, detail::const_converter<HeldType>::apply(
					get_const_holder((HeldType*)0))
				, detail::holder_constructor<HeldType>::apply(detail::type_<T>())
				, detail::const_holder_constructor<HeldType>::apply(detail::type_<T>())
				, detail::holder_default_constructor<HeldType>::apply(detail::type_<T>())
				, detail::const_holder_default_constructor<HeldType>::apply(detail::type_<T>())
				, get_adopt_fun((WrappedType*)0) // adopt fun
				, detail::internal_holder_destructor<HeldType>::apply(detail::type_<T>())
				, detail::internal_const_holder_destructor<HeldType>::apply(detail::type_<T>())
				, detail::internal_holder_size<HeldType>::apply()
				, detail::get_holder_alignment<HeldType>::apply());

			generate_baseclass_list(detail::type_<Base>());
		}

		template<class Getter, class GetPolicies>
		class_& property_impl(const char* name,
									 Getter g,
									 GetPolicies policies,
									 boost::mpl::bool_<true>)
		{
            this->add_member(
                new detail::property_registration<T, Getter, GetPolicies>(
                    name, g, policies));
			return *this;
		}

		template<class Getter, class Setter>
		class_& property_impl(const char* name,
									 Getter g,
									 Setter s,
									 boost::mpl::bool_<false>)
		{
            typedef detail::property_registration<
                T, Getter, detail::null_type, Setter, detail::null_type
            > registration_type;

            this->add_member(
                new registration_type(name, g, detail::null_type(), s));
			return *this;
		}

		// these handle default implementation of virtual functions
		template<class F, class Policies>
		class_& virtual_def(char const* name, F const& fn
			, Policies const&, detail::null_type, boost::mpl::true_)
		{
			this->add_member(
				new detail::memfun_registration<T, F, Policies>(
					name, fn, Policies()));
			return *this;
		}

		template<class F, class Default, class Policies>
		class_& virtual_def(char const* name, F const& fn
			, Default const& default_, Policies const&, boost::mpl::false_)
		{
			this->add_member(
				new detail::memfun_registration<T, F, Policies>(
					name, fn, Policies()));

			this->add_default_member(
				new detail::memfun_registration<T, Default, Policies>(
					name, default_, Policies()));

			return *this;
		}

        template<class Signature, class Policies>
		class_& def_constructor(Signature*, Policies const&)
        {
            typedef typename Signature::signature signature;

            typedef typename boost::mpl::if_<
                boost::is_same<WrappedType, detail::null_type>
              , T
              , WrappedType
            >::type construct_type;

            this->add_member(
                new detail::constructor_registration<
                    construct_type, signature, Policies>(
                        Policies()));

            this->add_default_member(
                new detail::constructor_registration<
                    construct_type, signature, Policies>(
                        Policies()));

            return *this;
        }

		typedef void(*adopt_fun_t)(void*);

		template<class W>
		adopt_fun_t get_adopt_fun(W*)
		{
            return &detail::adopt_function<T, W>::execute;
		}

		adopt_fun_t get_adopt_fun(detail::null_type*)
		{
			return 0;
		}
	};

	detail::policy_cons<
		detail::pure_virtual_tag, detail::null_type> const pure_virtual = {};

    namespace detail
    {
      inline void ignore_unused_pure_virtual()
      {
          (void)pure_virtual;
      }
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // LUABIND_CLASS_HPP_INCLUDED

