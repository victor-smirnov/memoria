

//          Copyright Victor Smirnov 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/config.hpp>

#include <boost/assert.hpp>

#include <boost/fiber/context.hpp>
#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/spinlock.hpp>


#include <boost/fiber/mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/operations.hpp>

#include <cstdint>
#include <mutex>


#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4251)
#endif

namespace boost {
namespace fibers {
    
    
// Note that this shared mutex is not up/down-gradable. That means an owner holding
// exclusive lock must not obtain shared lock and vise versa.

template <typename T>
class count_down_latch {
    using Mutex = mutex;
    
	T value_;
	Mutex mutex_;
	condition_variable cv;
public:
	count_down_latch(): value_() {}
	count_down_latch(const T& value): value_(value) {}

	void inc() {
		std::unique_lock<Mutex> lk(mutex_);
		++value_;
        cv.notify_all();
	}

	void dec() {
		std::unique_lock<Mutex> lk(mutex_);
		--value_;
        cv.notify_all();
	}

	const T& get() {
		std::unique_lock<Mutex> lk(mutex_);
		return value_;
	}

	void wait(const T& value)
	{
		std::unique_lock<Mutex> lk(mutex_);
        cv.wait(lk, [&]{
            return value_ == value;
        });
	}

	template< class Rep, class Period, class Predicate >
	void waitFor(const T& value, const std::chrono::duration<Rep, Period>& rel_time)
	{
		std::unique_lock<Mutex> lk(mutex_);
		cv.wait_for(lk, rel_time, [&]{return value_ == value;});
	}

	void waitFor(const T& value, int64_t rel_time_ms)
	{
		std::unique_lock<Mutex> lk(mutex_);
		cv.wait_for(lk, std::chrono::milliseconds(rel_time_ms), [&]{return value_ == value;});
	}
};

    
}}
