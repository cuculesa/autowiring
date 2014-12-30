/*=============================================================================
    Copyright (c) 2009 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef AUTOBOOST_FUSION_VIEW_FILTER_VIEW_DETAIL_KEY_OF_IMPL_HPP
#define AUTOBOOST_FUSION_VIEW_FILTER_VIEW_DETAIL_KEY_OF_IMPL_HPP

#include <autoboost/fusion/support/config.hpp>
#include <autoboost/fusion/iterator/key_of.hpp>

namespace autoboost { namespace fusion { namespace extension
{
    template <typename>
    struct key_of_impl;

    template <>
    struct key_of_impl<filter_view_iterator_tag>
    {
        template <typename It>
        struct apply
          : result_of::key_of<typename It::first_type>
        {};
    };
}}}

#endif
