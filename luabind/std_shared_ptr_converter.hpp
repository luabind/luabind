#if defined(LUABIND_TEST_HEADERCOMPILE)
# include <boost/config.hpp>
# if (defined(BOOST_NO_CXX11_SMART_PTR) \
      && (!defined(BOOST_MSVC) || BOOST_MSVC < 1600))
// Skip file contents if in headercompile test and std::shared_ptr is not
// supported.
#  define LUABIND_STD_SHAREDPTR_CONVERTER_HPP_INCLUDED
# endif
#endif


#ifndef LUABIND_STD_SHAREDPTR_CONVERTER_HPP_INCLUDED
#define LUABIND_STD_SHAREDPTR_CONVERTER_HPP_INCLUDED LUABIND_STD_SHAREDPTR_CONVERTER_HPP_INCLUDED

#include <boost/config.hpp>

#include <memory> // shared_ptr

#if BOOST_VERSION >= 105300
# include <luabind/detail/has_get_pointer.hpp>

# include <boost/get_pointer.hpp>

namespace luabind { namespace detail { namespace has_get_pointer_ {
  template<class T>
  struct impl<std::shared_ptr<T>> {
      BOOST_STATIC_CONSTANT(bool, value = true);
      typedef boost::mpl::bool_<value> type;
  };

  template<class T>
  struct impl<const std::shared_ptr<T>>: impl<std::shared_ptr<T>> { };

  template<class T>
  struct impl<volatile std::shared_ptr<T>>: impl<std::shared_ptr<T>> { };

  template<class T>
  struct impl<const volatile std::shared_ptr<T>>: impl<std::shared_ptr<T>> { };
}}
using boost::get_pointer;
}



#else // if BOOST_VERSION < 105300

// Not standard conforming: add function to ::std(::tr1)
namespace std {

#if defined(_MSC_VER) && _MSC_VER < 1700
namespace tr1 {
#endif

    template<class T>
    T * get_pointer(shared_ptr<T> const& p) { return p.get(); }

#if defined(_MSC_VER) && _MSC_VER < 1700
} // namespace tr1
#endif

} // namespace std

#endif // if BOOST_VERSION < 105300 / else

#endif
