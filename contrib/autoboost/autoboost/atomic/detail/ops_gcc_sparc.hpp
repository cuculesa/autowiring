/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2010 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_sparc.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef AUTOBOOST_ATOMIC_DETAIL_OPS_GCC_SPARC_HPP_INCLUDED_
#define AUTOBOOST_ATOMIC_DETAIL_OPS_GCC_SPARC_HPP_INCLUDED_

#include <autoboost/memory_order.hpp>
#include <autoboost/atomic/detail/config.hpp>
#include <autoboost/atomic/detail/storage_type.hpp>
#include <autoboost/atomic/detail/operations_fwd.hpp>
#include <autoboost/atomic/capabilities.hpp>
#include <autoboost/atomic/detail/ops_cas_based.hpp>
#include <autoboost/atomic/detail/ops_extending_cas_based.hpp>

#ifdef AUTOBOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace autoboost {
namespace atomics {
namespace detail {

struct gcc_sparc_cas_base
{
    static AUTOBOOST_FORCEINLINE void fence_before(memory_order order) AUTOBOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __asm__ __volatile__ ("membar #Sync" ::: "memory");
        else if ((order & memory_order_release) != 0)
            __asm__ __volatile__ ("membar #StoreStore | #LoadStore" ::: "memory");
    }

    static AUTOBOOST_FORCEINLINE void fence_after(memory_order order) AUTOBOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __asm__ __volatile__ ("membar #Sync" ::: "memory");
        else if ((order & (memory_order_consume | memory_order_acquire)) != 0)
            __asm__ __volatile__ ("membar #StoreStore | #LoadStore" ::: "memory");
    }

    static AUTOBOOST_FORCEINLINE void fence_after_store(memory_order order) AUTOBOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __asm__ __volatile__ ("membar #Sync" ::: "memory");
    }
};

template< bool Signed >
struct gcc_sparc_cas32 :
    public gcc_sparc_cas_base
{
    typedef typename make_storage_type< 4u, Signed >::type storage_type;

    static AUTOBOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) AUTOBOOST_NOEXCEPT
    {
        fence_before_store(order);
        storage = v;
        fence_after_store(order);
    }

    static AUTOBOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) AUTOBOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static AUTOBOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) AUTOBOOST_NOEXCEPT
    {
        fence_before(success_order);
        storage_type previous = expected;
        __asm__ __volatile__
        (
            "cas [%1], %2, %0"
            : "+r" (desired)
            : "r" (&storage), "r" (previous)
            : "memory"
        );
        const bool success = (desired == previous);
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = desired;
        return success;
    }

    static AUTOBOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) AUTOBOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static AUTOBOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) AUTOBOOST_NOEXCEPT
    {
        return true;
    }
};

template< bool Signed >
struct operations< 4u, Signed > :
    public cas_based_operations< gcc_sparc_cas32< Signed > >
{
    typedef cas_based_operations< gcc_sparc_cas32< Signed > > base_type;
    typedef typename base_type::storage_type storage_type;

    static AUTOBOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) AUTOBOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm__ __volatile__
        (
            "swap [%1], %0"
            : "+r" (v)
            : "r" (&storage)
            : "memory"
        );
        base_type::fence_after(order);
        return v;
    }

    static AUTOBOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) AUTOBOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }
};

template< bool Signed >
struct operations< 1u, Signed > :
    public extending_cas_based_operations< operations< 4u, Signed >, 1u, Signed >
{
};

template< bool Signed >
struct operations< 2u, Signed > :
    public extending_cas_based_operations< operations< 4u, Signed >, 2u, Signed >
{
};

template< bool Signed >
struct gcc_sparc_cas64 :
    public gcc_sparc_cas_base
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;

    static AUTOBOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) AUTOBOOST_NOEXCEPT
    {
        fence_before_store(order);
        storage = v;
        fence_after_store(order);
    }

    static AUTOBOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) AUTOBOOST_NOEXCEPT
    {
        storage_type v = storage;
        fence_after(order);
        return v;
    }

    static AUTOBOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) AUTOBOOST_NOEXCEPT
    {
        fence_before(success_order);
        storage_type previous = expected;
        __asm__ __volatile__
        (
            "casx [%1], %2, %0"
            : "+r" (desired)
            : "r" (&storage), "r" (previous)
            : "memory"
        );
        const bool success = (desired == previous);
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        expected = desired;
        return success;
    }

    static AUTOBOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) AUTOBOOST_NOEXCEPT
    {
        return compare_exchange_strong(storage, expected, desired, success_order, failure_order);
    }

    static AUTOBOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) AUTOBOOST_NOEXCEPT
    {
        return true;
    }
};

template< bool Signed >
struct operations< 8u, Signed > :
    public cas_based_operations< gcc_sparc_cas64< Signed > >
{
};


AUTOBOOST_FORCEINLINE void thread_fence(memory_order order) AUTOBOOST_NOEXCEPT
{
    switch (order)
    {
    case memory_order_release:
        __asm__ __volatile__ ("membar #StoreStore | #LoadStore" ::: "memory");
        break;
    case memory_order_consume:
    case memory_order_acquire:
        __asm__ __volatile__ ("membar #LoadLoad | #LoadStore" ::: "memory");
        break;
    case memory_order_acq_rel:
        __asm__ __volatile__ ("membar #LoadLoad | #LoadStore | #StoreStore" ::: "memory");
        break;
    case memory_order_seq_cst:
        __asm__ __volatile__ ("membar #Sync" ::: "memory");
        break;
    case memory_order_relaxed:
    default:
        break;
    }
}

AUTOBOOST_FORCEINLINE void signal_fence(memory_order order) AUTOBOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace autoboost

#endif // AUTOBOOST_ATOMIC_DETAIL_OPS_GCC_SPARC_HPP_INCLUDED_
