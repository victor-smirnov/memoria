
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


#include <memoria/reactor/reactor.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include "msvc_timer.hpp"

namespace memoria {
namespace reactor {

VOID CALLBACK TimerCallback(
	_In_ PVOID   timerImplPtr,
	_In_ BOOLEAN
) 
{
	ptr_cast<TimerImpl>(timerImplPtr)->onFiring();
}


TimerImpl::TimerImpl(Timer::TimeUnit start_after, Timer::TimeUnit repeat_after, uint64_t count, TimerFn timer_fn):
	engine_(engine()),
	remaining_(count),
	timer_fn_(timer_fn)
{
	if (!CreateTimerQueueTimer(
		&timer_fd_, engine_.io_poller().timer_queue(), 
		TimerCallback, this, 
		start_after.count(), 
		repeat_after.count(), 
		WT_EXECUTEDEFAULT)
	)
    {
		MMA_THROW(SystemException()) << WhatCInfo("Can't create a new timer");
    }
}

TimerImpl::~TimerImpl() noexcept
{
    cancel();
}

void TimerImpl::onFiring() 
{
	using TimerMessageSharedPtr = std::shared_ptr<TimerImpl>;
	TimerMessage* msg_ptr = new TimerMessage(engine_.cpu(), this->shared_from_this());
	engine_.send_message(msg_ptr);
}

void TimerImpl::cancel()
{
	if (!stopped_)
	{
		stopped_ = true;

		if (!DeleteTimerQueueTimer(engine_.io_poller().timer_queue(), timer_fd_, nullptr))
		{
			MMA_THROW(SystemException()) << WhatCInfo("Can't cancel timer");
		}
	}
}



void TimerMessage::finish() 
{
	auto timer = timer_.get();

	if (timer->is_running())
	{
		if (timer->remaining_ > 0) 
		{
			timer->remaining_--;

			fibers::fiber([=] {
				timer->timer_fn_();
			}).detach();
		}
		else if (!timer->auto_canceling_requested_) {
			timer->auto_canceling_requested_ = true;
			timer->cancel();
		}
	}

	delete this;
}


class WakeupMessage : public Message {
	Reactor& engine_;
	HANDLE timer_fd_{INVALID_HANDLE_VALUE};

	FiberContext* context_;

public:
	WakeupMessage(int cpu) :
		Message(cpu, false),
		engine_(engine())
	{
		return_ = true;
		context_ = fibers::context::active();
	}

	virtual ~WakeupMessage() = default;

	virtual void process() noexcept {};
	virtual void finish() 
	{
		if (!DeleteTimerQueueTimer(this->timer_queue(), this->timer_fd(), nullptr))
		{
			DumpErrorMessage("Can't cancel sleep timer", GetLastError());
		}

		engine().scheduler()->resume(context_);
		delete this;
	};

	void sleep() {
		engine().scheduler()->suspend(context_);
	}

	void submit() {
		engine_.send_message(this);
	}

	HANDLE timer_fd() {
		return timer_fd_;
	}

	void set_timer_fd(HANDLE timer_fd) {
		this->timer_fd_ = timer_fd;
	}

	HANDLE timer_queue() {
		return engine_.io_poller().timer_queue();
	}

	virtual std::string describe() { return "SleepMessage"; }
};


VOID CALLBACK WakeupCallback(
	_In_ PVOID   wakeupMsgPtr,
	_In_ BOOLEAN
)
{
	auto msg = ptr_cast<WakeupMessage>(wakeupMsgPtr);
	msg->submit();
}

void IOPoller::sleep_for(const std::chrono::milliseconds& time)
{
	HANDLE timer_fd{};

	WakeupMessage* msg = new WakeupMessage(engine().cpu());

	if (!CreateTimerQueueTimer(
		&timer_fd, engine().io_poller().timer_queue(),
		WakeupCallback, 
		msg,
		time.count(),
		0,
		WT_EXECUTEDEFAULT)
	)
	{
		delete msg;
		MMA_THROW(SystemException()) << WhatCInfo("Can't create a new timer");
	}

	msg->set_timer_fd(timer_fd);

	msg->sleep();
}


}}
