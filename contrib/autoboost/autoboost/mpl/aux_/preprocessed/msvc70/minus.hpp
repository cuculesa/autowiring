
// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//

// Preprocessed version of "autoboost/mpl/minus.hpp" header
// -- DO NOT modify by hand!

namespace autoboost { namespace mpl {

template<
      typename Tag1
    , typename Tag2

    , AUTOBOOST_MPL_AUX_NTTP_DECL(int, tag1_)  = AUTOBOOST_MPL_AUX_MSVC_VALUE_WKND(Tag1)::value
    , AUTOBOOST_MPL_AUX_NTTP_DECL(int, tag2_)  = AUTOBOOST_MPL_AUX_MSVC_VALUE_WKND(Tag2)::value
    >
struct minus_impl
    : if_c<
          ( tag1_ > tag2_ )
        , aux::cast2nd_impl< minus_impl< Tag1,Tag1 >,Tag1, Tag2 >
        , aux::cast1st_impl< minus_impl< Tag2,Tag2 >,Tag1, Tag2 >
        >::type
{
};

/// for Digital Mars C++/compilers with no CTPS/TTP support
template<> struct minus_impl< na,na >
{
    template< typename U1, typename U2 > struct apply
    {
        typedef apply type;
        AUTOBOOST_STATIC_CONSTANT(int, value  = 0);
    };
};

template<> struct minus_impl< na,integral_c_tag >
{
    template< typename U1, typename U2 > struct apply
    {
        typedef apply type;
        AUTOBOOST_STATIC_CONSTANT(int, value  = 0);
    };
};

template<> struct minus_impl< integral_c_tag,na >
{
    template< typename U1, typename U2 > struct apply
    {
        typedef apply type;
        AUTOBOOST_STATIC_CONSTANT(int, value  = 0);
    };
};

template< typename T > struct minus_tag
    : tag< T,na >
{
};

/// forward declaration

template<
      typename AUTOBOOST_MPL_AUX_NA_PARAM(N1)
    , typename AUTOBOOST_MPL_AUX_NA_PARAM(N2)
    >
struct minus2;

template<
      typename AUTOBOOST_MPL_AUX_NA_PARAM(N1)
    , typename AUTOBOOST_MPL_AUX_NA_PARAM(N2)
    , typename N3 = na, typename N4 = na, typename N5 = na
    >
struct minus

    : aux::msvc_eti_base< typename if_<

          is_na<N3>
        , minus2< N1,N2 >
        , minus<
              minus2< N1,N2 >
            , N3, N4, N5
            >
        >::type

    >

{
    AUTOBOOST_MPL_AUX_LAMBDA_SUPPORT(
          5
        , minus
        , ( N1, N2, N3, N4, N5 )
        )
};

template<
      typename N1
    , typename N2
    >
struct minus2
    : aux::msvc_eti_base< typename apply_wrap2<
          minus_impl<
              typename minus_tag<N1>::type
            , typename minus_tag<N2>::type
            >
        , N1
        , N2
        >::type >::type

{
    AUTOBOOST_MPL_AUX_LAMBDA_SUPPORT(2, minus2, (N1, N2))

};

AUTOBOOST_MPL_AUX_NA_SPEC2(2, 5, minus)

}}

namespace autoboost { namespace mpl {

namespace aux {
template< typename T, T n1, T n2 >
struct minus_wknd
{
    AUTOBOOST_STATIC_CONSTANT(T, value  = (n1 - n2));
    typedef integral_c< T,value > type;
};

}

template<>
struct minus_impl< integral_c_tag,integral_c_tag >
{
    template< typename N1, typename N2 > struct apply
        : aux::minus_wknd<
              typename aux::largest_int<
                  typename N1::value_type
                , typename N2::value_type
                >::type
            , N1::value
            , N2::value
            >::type

    {
    };
};

}}