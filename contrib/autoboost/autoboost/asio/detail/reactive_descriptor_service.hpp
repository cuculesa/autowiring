//
// detail/reactive_descriptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef AUTOBOOST_ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP
#define AUTOBOOST_ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <autoboost/asio/detail/config.hpp>

#if !defined(AUTOBOOST_ASIO_WINDOWS) \
  && !defined(AUTOBOOST_ASIO_WINDOWS_RUNTIME) \
  && !defined(__CYGWIN__)

#include <autoboost/asio/buffer.hpp>
#include <autoboost/asio/io_service.hpp>
#include <autoboost/asio/detail/addressof.hpp>
#include <autoboost/asio/detail/bind_handler.hpp>
#include <autoboost/asio/detail/buffer_sequence_adapter.hpp>
#include <autoboost/asio/detail/descriptor_ops.hpp>
#include <autoboost/asio/detail/descriptor_read_op.hpp>
#include <autoboost/asio/detail/descriptor_write_op.hpp>
#include <autoboost/asio/detail/fenced_block.hpp>
#include <autoboost/asio/detail/noncopyable.hpp>
#include <autoboost/asio/detail/reactive_null_buffers_op.hpp>
#include <autoboost/asio/detail/reactor.hpp>

#include <autoboost/asio/detail/push_options.hpp>

namespace autoboost {
namespace asio {
namespace detail {

class reactive_descriptor_service
{
public:
  // The native type of a descriptor.
  typedef int native_handle_type;

  // The implementation type of the descriptor.
  class implementation_type
    : private autoboost::asio::detail::noncopyable
  {
  public:
    // Default constructor.
    implementation_type()
      : descriptor_(-1),
        state_(0)
    {
    }

  private:
    // Only this service will have access to the internal values.
    friend class reactive_descriptor_service;

    // The native descriptor representation.
    int descriptor_;

    // The current state of the descriptor.
    descriptor_ops::state_type state_;

    // Per-descriptor data used by the reactor.
    reactor::per_descriptor_data reactor_data_;
  };

  // Constructor.
  AUTOBOOST_ASIO_DECL reactive_descriptor_service(
      autoboost::asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  AUTOBOOST_ASIO_DECL void shutdown_service();

  // Construct a new descriptor implementation.
  AUTOBOOST_ASIO_DECL void construct(implementation_type& impl);

  // Move-construct a new descriptor implementation.
  AUTOBOOST_ASIO_DECL void move_construct(implementation_type& impl,
      implementation_type& other_impl);

  // Move-assign from another descriptor implementation.
  AUTOBOOST_ASIO_DECL void move_assign(implementation_type& impl,
      reactive_descriptor_service& other_service,
      implementation_type& other_impl);

  // Destroy a descriptor implementation.
  AUTOBOOST_ASIO_DECL void destroy(implementation_type& impl);

  // Assign a native descriptor to a descriptor implementation.
  AUTOBOOST_ASIO_DECL autoboost::system::error_code assign(implementation_type& impl,
      const native_handle_type& native_descriptor,
      autoboost::system::error_code& ec);

  // Determine whether the descriptor is open.
  bool is_open(const implementation_type& impl) const
  {
    return impl.descriptor_ != -1;
  }

  // Destroy a descriptor implementation.
  AUTOBOOST_ASIO_DECL autoboost::system::error_code close(implementation_type& impl,
      autoboost::system::error_code& ec);

  // Get the native descriptor representation.
  native_handle_type native_handle(const implementation_type& impl) const
  {
    return impl.descriptor_;
  }

  // Release ownership of the native descriptor representation.
  AUTOBOOST_ASIO_DECL native_handle_type release(implementation_type& impl);

  // Cancel all operations associated with the descriptor.
  AUTOBOOST_ASIO_DECL autoboost::system::error_code cancel(implementation_type& impl,
      autoboost::system::error_code& ec);

  // Perform an IO control command on the descriptor.
  template <typename IO_Control_Command>
  autoboost::system::error_code io_control(implementation_type& impl,
      IO_Control_Command& command, autoboost::system::error_code& ec)
  {
    descriptor_ops::ioctl(impl.descriptor_, impl.state_,
        command.name(), static_cast<ioctl_arg_type*>(command.data()), ec);
    return ec;
  }

  // Gets the non-blocking mode of the descriptor.
  bool non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::user_set_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the descriptor.
  autoboost::system::error_code non_blocking(implementation_type& impl,
      bool mode, autoboost::system::error_code& ec)
  {
    descriptor_ops::set_user_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    return ec;
  }

  // Gets the non-blocking mode of the native descriptor implementation.
  bool native_non_blocking(const implementation_type& impl) const
  {
    return (impl.state_ & descriptor_ops::internal_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the native descriptor implementation.
  autoboost::system::error_code native_non_blocking(implementation_type& impl,
      bool mode, autoboost::system::error_code& ec)
  {
    descriptor_ops::set_internal_non_blocking(
        impl.descriptor_, impl.state_, mode, ec);
    return ec;
  }

  // Write some data to the descriptor.
  template <typename ConstBufferSequence>
  size_t write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, autoboost::system::error_code& ec)
  {
    buffer_sequence_adapter<autoboost::asio::const_buffer,
        ConstBufferSequence> bufs(buffers);

    return descriptor_ops::sync_write(impl.descriptor_, impl.state_,
        bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
  }

  // Wait until data can be written without blocking.
  size_t write_some(implementation_type& impl,
      const null_buffers&, autoboost::system::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_write(impl.descriptor_, impl.state_, ec);

    return 0;
  }

  // Start an asynchronous write. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler>
  void async_write_some(implementation_type& impl,
      const ConstBufferSequence& buffers, Handler& handler)
  {
    bool is_continuation =
      autoboost_asio_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef descriptor_write_op<ConstBufferSequence, Handler> op;
    typename op::ptr p = { autoboost::asio::detail::addressof(handler),
      autoboost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.descriptor_, buffers, handler);

    AUTOBOOST_ASIO_HANDLER_CREATION((p.p, "descriptor", &impl, "async_write_some"));

    start_op(impl, reactor::write_op, p.p, is_continuation, true,
        buffer_sequence_adapter<autoboost::asio::const_buffer,
          ConstBufferSequence>::all_empty(buffers));
    p.v = p.p = 0;
  }

  // Start an asynchronous wait until data can be written without blocking.
  template <typename Handler>
  void async_write_some(implementation_type& impl,
      const null_buffers&, Handler& handler)
  {
    bool is_continuation =
      autoboost_asio_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef reactive_null_buffers_op<Handler> op;
    typename op::ptr p = { autoboost::asio::detail::addressof(handler),
      autoboost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(handler);

    AUTOBOOST_ASIO_HANDLER_CREATION((p.p, "descriptor",
          &impl, "async_write_some(null_buffers)"));

    start_op(impl, reactor::write_op, p.p, is_continuation, false, false);
    p.v = p.p = 0;
  }

  // Read some data from the stream. Returns the number of bytes read.
  template <typename MutableBufferSequence>
  size_t read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, autoboost::system::error_code& ec)
  {
    buffer_sequence_adapter<autoboost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    return descriptor_ops::sync_read(impl.descriptor_, impl.state_,
        bufs.buffers(), bufs.count(), bufs.all_empty(), ec);
  }

  // Wait until data can be read without blocking.
  size_t read_some(implementation_type& impl,
      const null_buffers&, autoboost::system::error_code& ec)
  {
    // Wait for descriptor to become ready.
    descriptor_ops::poll_read(impl.descriptor_, impl.state_, ec);

    return 0;
  }

  // Start an asynchronous read. The buffer for the data being read must be
  // valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence, typename Handler>
  void async_read_some(implementation_type& impl,
      const MutableBufferSequence& buffers, Handler& handler)
  {
    bool is_continuation =
      autoboost_asio_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef descriptor_read_op<MutableBufferSequence, Handler> op;
    typename op::ptr p = { autoboost::asio::detail::addressof(handler),
      autoboost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.descriptor_, buffers, handler);

    AUTOBOOST_ASIO_HANDLER_CREATION((p.p, "descriptor", &impl, "async_read_some"));

    start_op(impl, reactor::read_op, p.p, is_continuation, true,
        buffer_sequence_adapter<autoboost::asio::mutable_buffer,
          MutableBufferSequence>::all_empty(buffers));
    p.v = p.p = 0;
  }

  // Wait until data can be read without blocking.
  template <typename Handler>
  void async_read_some(implementation_type& impl,
      const null_buffers&, Handler& handler)
  {
    bool is_continuation =
      autoboost_asio_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef reactive_null_buffers_op<Handler> op;
    typename op::ptr p = { autoboost::asio::detail::addressof(handler),
      autoboost_asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(handler);

    AUTOBOOST_ASIO_HANDLER_CREATION((p.p, "descriptor",
          &impl, "async_read_some(null_buffers)"));

    start_op(impl, reactor::read_op, p.p, is_continuation, false, false);
    p.v = p.p = 0;
  }

private:
  // Start the asynchronous operation.
  AUTOBOOST_ASIO_DECL void start_op(implementation_type& impl, int op_type,
      reactor_op* op, bool is_continuation, bool is_non_blocking, bool noop);

  // The selector that performs event demultiplexing for the service.
  reactor& reactor_;
};

} // namespace detail
} // namespace asio
} // namespace autoboost

#include <autoboost/asio/detail/pop_options.hpp>

#if defined(AUTOBOOST_ASIO_HEADER_ONLY)
# include <autoboost/asio/detail/impl/reactive_descriptor_service.ipp>
#endif // defined(AUTOBOOST_ASIO_HEADER_ONLY)

#endif // !defined(AUTOBOOST_ASIO_WINDOWS)
       //   && !defined(AUTOBOOST_ASIO_WINDOWS_RUNTIME)
       //   && !defined(__CYGWIN__)

#endif // AUTOBOOST_ASIO_DETAIL_REACTIVE_DESCRIPTOR_SERVICE_HPP
