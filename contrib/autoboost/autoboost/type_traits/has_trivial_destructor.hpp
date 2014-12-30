
//  (C) Copyright Steve Cleary, Beman Dawes, Howard Hinnant & John Maddock 2000.
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef AUTOBOOST_TT_HAS_TRIVIAL_DESTRUCTOR_HPP_INCLUDED
#define AUTOBOOST_TT_HAS_TRIVIAL_DESTRUCTOR_HPP_INCLUDED

#include <autoboost/type_traits/config.hpp>
#include <autoboost/type_traits/intrinsics.hpp>
#include <autoboost/type_traits/is_pod.hpp>
#include <autoboost/type_traits/detail/ice_or.hpp>

// should be the last #include
#include <autoboost/type_traits/detail/bool_trait_def.hpp>

namespace autoboost {

namespace detail {

template <typename T>
struct has_trivial_dtor_impl
{
#ifdef AUTOBOOST_HAS_TRIVIAL_DESTRUCTOR
   AUTOBOOST_STATIC_CONSTANT(bool, value = AUTOBOOST_HAS_TRIVIAL_DESTRUCTOR(T));
#else
   AUTOBOOST_STATIC_CONSTANT(bool, value = ::autoboost::is_pod<T>::value);
#endif
};

} // namespace detail

AUTOBOOST_TT_AUX_BOOL_TRAIT_DEF1(has_trivial_destructor,T,::autoboost::detail::has_trivial_dtor_impl<T>::value)

AUTOBOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_destructor,void,false)
#ifndef AUTOBOOST_NO_CV_VOID_SPECIALIZATIONS
AUTOBOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_destructor,void const,false)
AUTOBOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_destructor,void const volatile,false)
AUTOBOOST_TT_AUX_BOOL_TRAIT_SPEC1(has_trivial_destructor,void volatile,false)
#endif

} // namespace autoboost

#include <autoboost/type_traits/detail/bool_trait_undef.hpp>

#endif // AUTOBOOST_TT_HAS_TRIVIAL_DESTRUCTOR_HPP_INCLUDED
