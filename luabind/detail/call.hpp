// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !BOOST_PP_IS_ITERATING

# ifndef LUABIND_CALL2_080911_HPP
#  define LUABIND_CALL2_080911_HPP

#  include <boost/mpl/apply_wrap.hpp>
#  include <boost/mpl/begin_end.hpp>
#  include <boost/mpl/deref.hpp>
#  include <boost/mpl/front.hpp>
#  include <boost/mpl/long.hpp>
#  include <boost/mpl/size.hpp>
#  include <boost/preprocessor/control/if.hpp>
#  include <boost/preprocessor/iteration/iterate.hpp>
#  include <boost/preprocessor/iteration/local.hpp>
#  include <boost/preprocessor/repetition/enum.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#  include <boost/type_traits/is_void.hpp>

#  include <luabind/config.hpp>
#  include <luabind/detail/policy.hpp>
#  include <luabind/yield_policy.hpp>

namespace luabind { namespace detail {

template <class F, class Signature, class Policies, class IsVoid>
int invoke0(
    lua_State* L, F const& f, Signature
  , Policies const& policies, IsVoid, mpl::true_)
{
    return invoke_member(
        L, f, Signature(), policies
      , mpl::long_<mpl::size<Signature>::value - 1>(), IsVoid()
    );
}

template <class F, class Signature, class Policies, class IsVoid>
int invoke0(
    lua_State* L, F const& f, Signature
  , Policies const& policies, IsVoid, mpl::false_)
{
    return invoke_normal(
        L, f, Signature(), policies
      , mpl::long_<mpl::size<Signature>::value - 1>(), IsVoid()
    );
}

template <class F, class Signature, class Policies>
int invoke(lua_State* L, F const& f, Signature, Policies const& policies)
{
    return invoke0(
        L, f, Signature(), policies
      , boost::is_void<typename mpl::front<Signature>::type>()
      , boost::is_member_function_pointer<F>()
   );
}

inline int maybe_yield_aux(lua_State*, int results, mpl::false_)
{
    return results;
}

inline int maybe_yield_aux(lua_State* L, int results, mpl::true_)
{
    return lua_yield(L, results);
}

template <class Policies>
int maybe_yield(lua_State* L, int results, Policies*)
{
    return maybe_yield_aux(
        L, results, mpl::bool_<has_yield<Policies>::value>());
}

#  define LUABIND_INVOKE_NEXT_ITER(n) \
    typename mpl::next< \
        BOOST_PP_IF( \
            n, BOOST_PP_CAT(iter,BOOST_PP_DEC(n)), first) \
    >::type

#  define LUABIND_INVOKE_NEXT_INDEX(n) \
    BOOST_PP_IF( \
        n \
      , BOOST_PP_CAT(index,BOOST_PP_DEC(n)) + \
            (BOOST_PP_CAT(p,BOOST_PP_DEC(n))::has_arg ? 1 : 0) \
      , 1 \
    )

#  define LUABIND_INVOKE_DECLARE_CONVERTER(n) \
    typedef LUABIND_INVOKE_NEXT_ITER(n) BOOST_PP_CAT(iter,n); \
    typedef typename mpl::deref<BOOST_PP_CAT(iter,n)>::type \
        BOOST_PP_CAT(a,n); \
    typedef typename find_conversion_policy<n + 1, Policies>::type \
        BOOST_PP_CAT(p,n); \
    typename mpl::apply_wrap2< \
        BOOST_PP_CAT(p,n), BOOST_PP_CAT(a,n), lua_to_cpp>::type BOOST_PP_CAT(c,n); \
    int const BOOST_PP_CAT(index,n) = LUABIND_INVOKE_NEXT_INDEX(n);

#  define LUABIND_INVOKE_ARG(z, n, base) \
    BOOST_PP_CAT(c,base(n)).apply( \
        L, LUABIND_DECORATE_TYPE(BOOST_PP_CAT(a,base(n))), BOOST_PP_CAT(index,base(n)))

#  define LUABIND_INVOKE_CONVERTER_POSTCALL(n) \
    BOOST_PP_CAT(c,n).converter_postcall( \
        L, LUABIND_DECORATE_TYPE(BOOST_PP_CAT(a,n)), BOOST_PP_CAT(index,n));

#  define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>))
#  include BOOST_PP_ITERATE()

#  define LUABIND_INVOKE_VOID
#  define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>))
#  include BOOST_PP_ITERATE()

#  undef LUABIND_INVOKE_VOID
#  define LUABIND_INVOKE_MEMBER
#  define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>))
#  include BOOST_PP_ITERATE()

#  define LUABIND_INVOKE_VOID
#  define BOOST_PP_ITERATION_PARAMS_1 \
    (3, (0, LUABIND_MAX_ARITY, <luabind/detail/call.hpp>))
#  include BOOST_PP_ITERATE()

}} // namespace luabind::detail

# endif // LUABIND_CALL2_080911_HPP

#else // BOOST_PP_IS_ITERATING

# ifdef LUABIND_INVOKE_MEMBER
#  define N BOOST_PP_INC(BOOST_PP_ITERATION())
# else
#  define N BOOST_PP_ITERATION()
# endif

template <class F, class Signature, class Policies>
int
# ifdef LUABIND_INVOKE_MEMBER
invoke_member
# else
invoke_normal
# endif
(
    lua_State* L, F const& f, Signature, Policies const&, mpl::long_<N>
# ifdef LUABIND_INVOKE_VOID
  , mpl::true_
# else
  , mpl::false_
# endif
)
{
    typedef typename mpl::begin<Signature>::type first;
# ifndef LUABIND_INVOKE_VOID
    typedef typename mpl::deref<first>::type result_type;
    typedef typename find_conversion_policy<0, Policies>::type result_policy;
    typename mpl::apply_wrap2<
        result_policy, result_type, cpp_to_lua>::type result_converter;
# endif

# if N > 0
#  define BOOST_PP_LOCAL_MACRO(n) LUABIND_INVOKE_DECLARE_CONVERTER(n)
#  define BOOST_PP_LOCAL_LIMITS (0,N-1)
#  include BOOST_PP_LOCAL_ITERATE()
# endif

    int const arguments = lua_gettop(L);

# ifndef LUABIND_INVOKE_VOID
    result_converter.apply(
        L,
# endif
# ifdef LUABIND_INVOKE_MEMBER
        (c0.apply(L, LUABIND_DECORATE_TYPE(a0), index0).*f)(
            BOOST_PP_ENUM(BOOST_PP_DEC(N), LUABIND_INVOKE_ARG, BOOST_PP_INC)
        )
# else
#  define LUABIND_INVOKE_IDENTITY(x) x
        f(
            BOOST_PP_ENUM(N, LUABIND_INVOKE_ARG, LUABIND_INVOKE_IDENTITY)
        )
#  undef LUABIND_INVOKE_IDENTITY
# endif
# ifndef LUABIND_INVOKE_VOID
    )
# endif
    ;

# if N > 0
#  define BOOST_PP_LOCAL_MACRO(n) LUABIND_INVOKE_CONVERTER_POSTCALL(n)
#  define BOOST_PP_LOCAL_LIMITS (0,N-1)
#  include BOOST_PP_LOCAL_ITERATE()
# endif

    int const results = lua_gettop(L) - arguments;

    int const indices[] = {
        arguments + results BOOST_PP_ENUM_TRAILING_PARAMS(N, index)
    };

    policy_list_postcall<Policies>::apply(L, indices);

    return maybe_yield(L, results, (Policies*)0);
}

# undef N

#endif

