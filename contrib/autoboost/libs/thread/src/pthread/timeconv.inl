// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2009 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// boostinspect:nounnamed

#include <boost/assert.hpp>

namespace {
const int MILLISECONDS_PER_SECOND = 1000;
const int NANOSECONDS_PER_SECOND = 1000000000;
const int NANOSECONDS_PER_MILLISECOND = 1000000;

const int MICROSECONDS_PER_SECOND = 1000000;
const int NANOSECONDS_PER_MICROSECOND = 1000;

#if defined BOOST_THREAD_USES_DATETIME
inline void to_time(int milliseconds, autoboost::xtime& xt)
{
    int res = 0;
    res = autoboost::xtime_get(&xt, autoboost::TIME_UTC_);
    BOOST_ASSERT(res == autoboost::TIME_UTC_); (void)res;

    xt.sec += (milliseconds / MILLISECONDS_PER_SECOND);
    xt.nsec += ((milliseconds % MILLISECONDS_PER_SECOND) *
        NANOSECONDS_PER_MILLISECOND);

    if (xt.nsec >= NANOSECONDS_PER_SECOND)
    {
        ++xt.sec;
        xt.nsec -= NANOSECONDS_PER_SECOND;
    }
}
#endif
#if defined(BOOST_HAS_PTHREADS)
#if defined BOOST_THREAD_USES_DATETIME
inline void to_timespec(const autoboost::xtime& xt, timespec& ts)
{
    ts.tv_sec = static_cast<int>(xt.sec);
    ts.tv_nsec = static_cast<int>(xt.nsec);
    if(ts.tv_nsec >= NANOSECONDS_PER_SECOND)
    {
        ts.tv_sec += ts.tv_nsec / NANOSECONDS_PER_SECOND;
        ts.tv_nsec %= NANOSECONDS_PER_SECOND;
    }
}
#endif
inline void to_time(int milliseconds, timespec& ts)
{
#if defined BOOST_THREAD_USES_DATETIME
    autoboost::xtime xt;
    to_time(milliseconds, xt);
    to_timespec(xt, ts);
#else
    ts.tv_sec += (milliseconds / MILLISECONDS_PER_SECOND);
    ts.tv_nsec += ((milliseconds % MILLISECONDS_PER_SECOND) *
        NANOSECONDS_PER_MILLISECOND);

    if (ts.tv_nsec >= NANOSECONDS_PER_SECOND)
    {
        ++ts.tv_sec;
        ts.tv_nsec -= NANOSECONDS_PER_SECOND;
    }
#endif
}

#if defined BOOST_THREAD_USES_DATETIME
inline void to_timespec_duration(const autoboost::xtime& xt, timespec& ts)
{
    autoboost::xtime cur;
    int res = 0;
    res = autoboost::xtime_get(&cur, autoboost::TIME_UTC_);
    BOOST_ASSERT(res == autoboost::TIME_UTC_); (void)res;

    if (autoboost::xtime_cmp(xt, cur) <= 0)
    {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
    }
    else
    {
        ts.tv_sec = xt.sec - cur.sec;
        ts.tv_nsec = xt.nsec - cur.nsec;

        if( ts.tv_nsec < 0 )
        {
            ts.tv_sec -= 1;
            ts.tv_nsec += NANOSECONDS_PER_SECOND;
        }
        if(ts.tv_nsec >= NANOSECONDS_PER_SECOND)
        {
            ts.tv_sec += ts.tv_nsec / NANOSECONDS_PER_SECOND;
            ts.tv_nsec %= NANOSECONDS_PER_SECOND;
        }
    }
}
#endif
#endif

#if defined BOOST_THREAD_USES_DATETIME
inline void to_duration(autoboost::xtime xt, int& milliseconds)
{
    autoboost::xtime cur;
    int res = 0;
    res = autoboost::xtime_get(&cur, autoboost::TIME_UTC_);
    BOOST_ASSERT(res == autoboost::TIME_UTC_); (void)res;

    if (autoboost::xtime_cmp(xt, cur) <= 0)
        milliseconds = 0;
    else
    {
        if (cur.nsec > xt.nsec)
        {
            xt.nsec += NANOSECONDS_PER_SECOND;
            --xt.sec;
        }
        milliseconds = (int)((xt.sec - cur.sec) * MILLISECONDS_PER_SECOND) +
            (((xt.nsec - cur.nsec) + (NANOSECONDS_PER_MILLISECOND/2)) /
                NANOSECONDS_PER_MILLISECOND);
    }
}

inline void to_microduration(autoboost::xtime xt, int& microseconds)
{
    autoboost::xtime cur;
    int res = 0;
    res = autoboost::xtime_get(&cur, autoboost::TIME_UTC_);
    BOOST_ASSERT(res == autoboost::TIME_UTC_); (void)res;

    if (autoboost::xtime_cmp(xt, cur) <= 0)
        microseconds = 0;
    else
    {
        if (cur.nsec > xt.nsec)
        {
            xt.nsec += NANOSECONDS_PER_SECOND;
            --xt.sec;
        }
        microseconds = (int)((xt.sec - cur.sec) * MICROSECONDS_PER_SECOND) +
            (((xt.nsec - cur.nsec) + (NANOSECONDS_PER_MICROSECOND/2)) /
                NANOSECONDS_PER_MICROSECOND);
    }
}
#endif
}

// Change Log:
//    1 Jun 01  Initial creation.