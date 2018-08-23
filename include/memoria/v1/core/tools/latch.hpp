
// Copyright 2016 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types.hpp>

#include <iostream>
#include <type_traits>
#include <memory>
#include <condition_variable>
#include <mutex>


namespace memoria {
namespace v1 {

template <typename T>
class CountDownLatch {
	T value_;
	std::mutex mutex_;
	std::condition_variable cv;
public:
	CountDownLatch(): value_() {}
	CountDownLatch(const T& value): value_(value) {}

	void inc() {
		std::unique_lock<std::mutex> lk(mutex_);
		++value_;
        cv.notify_all();
	}

	void dec() {
		std::unique_lock<std::mutex> lk(mutex_);
		--value_;
        cv.notify_all();
	}

	const T& get() {
		std::unique_lock<std::mutex> lk(mutex_);
		return value_;
	}

	void wait(const T& value)
	{
		std::unique_lock<std::mutex> lk(mutex_);
		cv.wait(lk, [&]{return value_ == value;});
	}

	template< class Rep, class Period, class Predicate >
    bool waitFor(const T& value, const std::chrono::duration<Rep, Period>& rel_time)
	{
		std::unique_lock<std::mutex> lk(mutex_);
        return cv.wait_for(lk, rel_time, [&]{return value_ == value;});
	}

    bool waitFor(const T& value, int64_t rel_time_ms)
	{
		std::unique_lock<std::mutex> lk(mutex_);
        return cv.wait_for(lk, std::chrono::milliseconds(rel_time_ms), [&]{return value_ == value;});
	}
};



template <typename T, typename MutexT = std::mutex>
class MutexlessCountDownLatch {
	T value_;
	MutexT& mutex_;
	std::condition_variable cv;
public:
	MutexlessCountDownLatch(MutexT& mutex): value_(), mutex_(mutex) {}
	MutexlessCountDownLatch(MutexT& mutex, const T& value): value_(value) {}

	void inc() {
		std::unique_lock<MutexT> lk(mutex_);
		++value_;
	}

	void dec() {
		std::unique_lock<MutexT> lk(mutex_);
		--value_;
	}

	const T& get() const {
		std::unique_lock<MutexT> lk(mutex_);
		return value_;
	}

	void wait(const T& value)
	{
		std::unique_lock<MutexT> lk(mutex_);
		cv.wait(lk, [&]{return value_ == value;});
	}

	template< class Rep, class Period, class Predicate >
	void waitFor(const T& value, const std::chrono::duration<Rep, Period>& rel_time)
	{
		std::unique_lock<MutexT> lk(mutex_);
		cv.wait_for(lk, rel_time, [&]{return value_ == value;});
	}

	void waitFor(const T& value, int64_t rel_time_ms)
	{
		std::unique_lock<MutexT> lk(mutex_);
		cv.wait_for(lk, std::chrono::milliseconds(rel_time_ms), [&]{return value_ == value;});
	}
};


}}
