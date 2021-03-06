#ifndef AUTOBOOST_THREAD_QUEUE_BASE_HPP
#define AUTOBOOST_THREAD_QUEUE_BASE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <autoboost/thread/detail/config.hpp>
#include <autoboost/thread/detail/move.hpp>
#include <autoboost/thread/concurrent_queues/queue_op_status.hpp>
#include <autoboost/type_traits/conditional.hpp>
#include <autoboost/type_traits/is_copy_constructible.hpp>


#include <autoboost/config/abi_prefix.hpp>

namespace autoboost
{
namespace concurrent
{
namespace detail
{

  template <typename ValueType>
  class queue_base_copyable_only
  {
  public:
    typedef ValueType value_type;
    typedef std::size_t size_type;

    // Constructors/Assignment/Destructors
    virtual ~queue_base_copyable_only() {};

    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void push_back(const value_type& x) = 0;

    virtual void pull_front(value_type&) = 0;
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_push_back(const value_type& x) = 0;
    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_push_back(const value_type& x) = 0;
    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_push_back(const value_type& x) = 0;
    virtual queue_op_status wait_pull_front(ValueType& elem) = 0;

  };

  template <typename ValueType>
  class queue_base_movable_only
  {
  public:
    typedef ValueType value_type;
    typedef std::size_t size_type;
    // Constructors/Assignment/Destructors
    virtual ~queue_base_movable_only() {};

    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void pull_front(value_type&) = 0;
    // enable_if is_nothrow_movable<value_type>
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_pull_front(ValueType& elem) = 0;

    virtual void push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status try_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status nonblocking_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status wait_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
  };


  template <typename ValueType>
  class queue_base_copyable_and_movable
  {
  public:
    typedef ValueType value_type;
    typedef std::size_t size_type;
    // Constructors/Assignment/Destructors
    virtual ~queue_base_copyable_and_movable() {};


    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void push_back(const value_type& x) = 0;

    virtual void pull_front(value_type&) = 0;
    // enable_if is_nothrow_copy_movable<value_type>
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_push_back(const value_type& x) = 0;
    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_push_back(const value_type& x) = 0;
    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_push_back(const value_type& x) = 0;
    virtual queue_op_status wait_pull_front(ValueType& elem) = 0;

    virtual void push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status try_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status nonblocking_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status wait_push_back(AUTOBOOST_THREAD_RV_REF(value_type) x) = 0;
  };

  template <class T,
#if ! defined  AUTOBOOST_NO_CXX11_RVALUE_REFERENCES
#if defined __GNUC__
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = true
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif // __GNUC__
#elif defined _MSC_VER
#if _MSC_VER < 1700
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = true
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif // _MSC_VER
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif
#else
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = has_move_emulation_enabled<T>::value
#endif
      >
  struct queue_base;

  template <class T>
  struct queue_base<T, true, true> {
    typedef queue_base_copyable_and_movable<T> type;
  };
  template <class T>
  struct queue_base<T, true, false> {
    typedef queue_base_copyable_only<T> type;
  };
  template <class T>
  struct queue_base<T, false, true> {
    typedef queue_base_movable_only<T> type;
  };

}

  template <typename ValueType>
  class queue_base :
    public detail::queue_base<ValueType>::type
  {
  public:
      typedef ValueType value_type;
      typedef std::size_t size_type;
    // Constructors/Assignment/Destructors
    virtual ~queue_base() {};
  };

}
using concurrent::queue_base;

}

#include <autoboost/config/abi_suffix.hpp>

#endif
