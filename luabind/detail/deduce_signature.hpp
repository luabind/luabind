// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# ifndef LUABIND_DEDUCE_SIGNATURE_080911_HPP
#  define LUABIND_DEDUCE_SIGNATURE_080911_HPP

#  include <luabind/detail/most_derived.hpp>

#  include <boost/function_types/components.hpp>
#  include <boost/type_traits/add_reference.hpp>

namespace luabind { namespace detail {

namespace mpl = boost::mpl;
namespace fty = boost::function_types;

template <class F>
fty::components<F> deduce_signature(F, ...)
{
    return fty::components<F>();
}

template <class F, class Wrapped>
fty::components<F, boost::add_reference<most_derived<mpl::_1, Wrapped> > >
deduce_signature(F, Wrapped*)
{
    return fty::components<F, boost::add_reference<most_derived<mpl::_1, Wrapped> > >();
}

} } // namespace luabind::detail

#endif  // LUABIND_DEDUCE_SIGNATURE_080911_HPP
