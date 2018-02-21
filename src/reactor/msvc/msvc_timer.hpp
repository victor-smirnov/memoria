// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/reactor/message/message.hpp>
#include <memoria/v1/reactor/timer.hpp>

#include <memoria/v1/fiber/fiber.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <memory>
#include <atomic>

#include <Windows.h>

namespace memoria {
namespace v1 {
namespace reactor {

class TimerImpl;
class Reactor;

class TimerMessage: public Message {

	LocalSharedPtr<TimerImpl> timer_;

public:
	TimerMessage(int cpu, LocalSharedPtr<TimerImpl> timer):
		Message(cpu, false),
		timer_(timer)
	{
		return_ = true;
	}

	virtual ~TimerMessage() = default;

	virtual void process() noexcept {};
	virtual void finish();

	virtual std::string describe() { return "TimerMessage"; }
};

class TimerImpl: public EnableSharedFromThis<TimerImpl> {
    HANDLE timer_fd_;

    bool stopped_{false};

	Reactor& engine_;

	uint64_t remaining_;
	bool auto_canceling_requested_{ false };

	TimerFn timer_fn_;


public:
	friend class TimerMessage;

    TimerImpl(Timer::TimeUnit start_after, Timer::TimeUnit repeat_after, uint64_t count, TimerFn timer_fn);
    ~TimerImpl() noexcept;

    void cancel();

	void onFiring();

	uint64_t remaining() const { return remaining_; }

    bool is_running() const {
        return !stopped_;
    }
};

}}}
