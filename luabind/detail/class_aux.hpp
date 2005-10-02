// Copyright (c) 2005 Daniel Wallin and Arvid Norberg

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

#ifndef LUABIND_CLASS_AUX_051001_HPP
#define LUABIND_CLASS_AUX_051001_HPP

#include <luabind/config.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>

#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

namespace luabind { 

namespace mpl = boost::mpl;

namespace detail 
{ 
  struct null_type; 
}

template<
    BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(
        LUABIND_MAX_BASES, class T, detail::null_type
    )
>
struct bases
{};

namespace detail 
{
  BOOST_PYTHON_IS_XXX_DEF(bases, bases, LUABIND_MAX_BASES)

  template<class T>
  struct is_unspecified : mpl::false_
  {};

  template<>
  struct is_unspecified<unspecified> : mpl::true_
  {};

  template<class Parameters, class Predicate, class DefaultValue>
  struct get_parameter
  {
      typedef typename boost::mpl::find_if<
          Parameters
        , boost::mpl::and_<
              boost::mpl::not_<is_unspecified<boost::mpl::_> >
            , Predicate
          >
      >::type iterator;

      typedef typename boost::mpl::eval_if<
          boost::is_same<
              iterator
            , typename boost::mpl::end<Parameters>::type
          >
        , boost::mpl::identity<DefaultValue>
        , boost::mpl::deref<iterator>
      >::type type;
  };

  template<class T, class X1, class X2, class X3>
  struct class_aux
  {
      typedef boost::mpl::vector3<X1,X2,X3> arguments;

      typedef typename get_parameter<
          arguments
        , boost::is_base_and_derived<T, boost::mpl::_>
        , null_type
      >::type wrapped_type;

      typedef typename get_parameter<
          arguments
        , boost::mpl::not_<
              boost::mpl::or_<
                  boost::mpl::or_<
                      is_bases<boost::mpl::_>
                    , boost::is_base_and_derived<boost::mpl::_, T>
                  >
                , boost::is_base_and_derived<T, boost::mpl::_>
              >
          >
        , null_type
      >::type held_type;

      typedef typename get_parameter<
          arguments
        , boost::mpl::or_<
              is_bases<boost::mpl::_>
            , boost::is_base_and_derived<boost::mpl::_, T>
          >
        , bases<>
      >::type bases0;

      typedef typename boost::mpl::if_<
          is_bases<bases0>
        , bases0
        , bases<bases0>
      >::type bases_type;
  };

}} // namespace luabind::detail

#endif // LUABIND_CLASS_AUX_051001_HPP

