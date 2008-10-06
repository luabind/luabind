// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_COMPUTE_RANK_081006_HPP
# define LUABIND_COMPUTE_RANK_081006_HPP

# include <luabind/config.hpp>
# include <luabind/detail/policy.hpp>
# include <boost/mpl/apply_wrap.hpp>
# include <boost/mpl/begin_end.hpp>
# include <boost/mpl/int.hpp>
# include <boost/mpl/next.hpp>

namespace luabind { namespace detail {

namespace mpl = boost::mpl;

template <class Idx, class Iter, class End, class Policies>
int compute_score_aux(
    lua_State*L, int index, Idx, Iter, End end, Policies const& policies)
{
    typedef typename Iter::type arg_type;
    typedef typename find_conversion_policy<Idx::value, Policies>::type
        conversion_policy;
    typedef typename mpl::apply_wrap2<
        conversion_policy, arg_type, lua_to_cpp>::type converter;

    int score = converter::match(L, LUABIND_DECORATE_TYPE(arg_type), index);

    if (score < 0)
        return score;

    if (conversion_policy::has_arg)
        ++index;

    int next = compute_score_aux(
        L
      , index
      , typename mpl::next<Idx>::type()
      , typename mpl::next<Iter>::type()
      , end
      , policies
    );

    if (next < 0)
        return next;

    return score + next;
}

template <class Idx, class End, class Policies>
int compute_score_aux(lua_State*, int, Idx, End, End, Policies const&)
{
    return 0;
}

template <class Signature, class Policies>
int compute_score(lua_State* L, Signature, Policies const& policies)
{
    return compute_score_aux(
        L
      , 1
      , mpl::int_<1>()
      , typename mpl::next<typename mpl::begin<Signature>::type>::type()
      , typename mpl::end<Signature>::type()
      , policies
    );
}

template <class Iter, class End, class Idx, class Policies>
int compute_arity_aux(Iter, End end, Idx, Policies const& policies)
{
    typedef typename find_conversion_policy<Idx::value, Policies>::type
        conversion_policy;

    int next = compute_arity_aux(
        typename mpl::next<Iter>::type()
      , end
      , typename mpl::next<Idx>::type()
      , policies
    );

    return (conversion_policy::has_arg ? 1 : 0) + next;
}

template <class End, class Idx, class Policies>
int compute_arity_aux(End, End, Idx, Policies const&)
{
    return 0;
}

template <class Signature, class Policies>
int compute_arity(Signature, Policies const& policies)
{
    return compute_arity_aux(
        typename mpl::next<typename mpl::begin<Signature>::type>::type()
      , typename mpl::end<Signature>::type()
      , mpl::int_<1>()
      , policies
    );
}

}} // namespace luabind::detail

#endif // LUABIND_COMPUTE_RANK_081006_HPP
