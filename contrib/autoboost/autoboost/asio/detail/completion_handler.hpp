//
// detail/completion_handler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef AUTOBOOST_ASIO_DETAIL_COMPLETION_HANDLER_HPP
#define AUTOBOOST_ASIO_DETAIL_COMPLETION_HANDLER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <autoboost/asio/detail/addressof.hpp>
#include <autoboost/asio/detail/config.hpp>
#include <autoboost/asio/detail/fenced_block.hpp>
#include <autoboost/asio/detail/handler_alloc_helpers.hpp>
#include <autoboost/asio/detail/handler_invoke_helpers.hpp>
#include <autoboost/asio/detail/operation.hpp>

#include <autoboost/asio/detail/push_options.hpp>

namespace autoboost {
namespace asio {
namespace detail {

template <typename Handler>
class completion_handler : public operation
{
public:
  AUTOBOOST_ASIO_DEFINE_HANDLER_PTR(completion_handler);

  completion_handler(Handler& h)
    : operation(&completion_handler::do_complete),
      handler_(AUTOBOOST_ASIO_MOVE_CAST(Handler)(h))
  {
  }

  static void do_complete(io_service_impl* owner, operation* base,
      const autoboost::system::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    completion_handler* h(static_cast<completion_handler*>(base));
    ptr p = { autoboost::asio::detail::addressof(h->handler_), h, h };

    AUTOBOOST_ASIO_HANDLER_COMPLETION((h));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    Handler handler(AUTOBOOST_ASIO_MOVE_CAST(Handler)(h->handler_));
    p.h = autoboost::asio::detail::addressof(handler);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      AUTOBOOST_ASIO_HANDLER_INVOCATION_BEGIN(());
      autoboost_asio_handler_invoke_helpers::invoke(handler, handler);
      AUTOBOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler handler_;
};

} // namespace detail
} // namespace asio
} // namespace autoboost

#include <autoboost/asio/detail/pop_options.hpp>

#endif // AUTOBOOST_ASIO_DETAIL_COMPLETION_HANDLER_HPP
