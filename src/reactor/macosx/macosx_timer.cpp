
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


#include <memoria/v1/reactor/reactor.hpp>

#include "macosx_timer.hpp"

#include <sys/event.h>


namespace memoria {
namespace v1 {
namespace reactor {


TimerImpl::TimerImpl(Timer::TimeUnit start_after, Timer::TimeUnit repeat_after, uint64_t count, TimerFn timer_fn):
    timer_fd_(engine().io_poller().new_timer_fd()),
    timer_message_(engine().cpu(), this),
    count_(count),
    timer_fn_(timer_fn),
    start_after_(start_after),
    repeat_after_(repeat_after)
{
    kevent64_s event = tools::make_zeroed<kevent64_s>();

    EV_SET64(
                &event,
                timer_fd_,
                EVFILT_TIMER,
                EV_ADD,
                NOTE_USECONDS,
                start_after.count() * 1000,
                (uint64_t)&timer_message_,
                0,0
    );

    timespec zero_timeout = tools::make_zeroed<timespec>();

    if (kevent64(engine().io_poller().queue_fd(), &event, 1, nullptr, 0, 0, &zero_timeout) < 0)
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't add kevent for Timer {}", timer_fd_);
    }
}

TimerImpl::~TimerImpl() noexcept
{
    cancel();
}

void TimerImpl::cancel()
{
    if (!stopped_)
    {
        stopped_ = true;

        kevent64_s event = tools::make_zeroed<kevent64_s>();

        EV_SET64(
                    &event,
                    timer_fd_,
                    EVFILT_TIMER,
                    EV_DELETE,
                    0,0,
                    0,0,
                    0
        );

        timespec zero_timeout = tools::make_zeroed<timespec>();

        if (kevent64(engine().io_poller().queue_fd(), &event, 1, nullptr, 0, 0, &zero_timeout) < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't remove kevent for Timer {}", timer_fd_);
        }
    }
}

void TimerImpl::on_firing(uint64_t fired_times)
{
    if (!stopped_ && (count_ > 0))
    {
        if (fired_times_ == 0 && start_after_ != repeat_after_)
        {
            kevent64_s event = tools::make_zeroed<kevent64_s>();

            EV_SET64(
                &event,
                timer_fd_,
                EVFILT_TIMER,
                EV_ADD,
                NOTE_USECONDS,
                repeat_after_.count() * 1000,
                (uint64_t)&timer_message_,
                0,0
            );

            timespec zero_timeout = tools::make_zeroed<timespec>();

            if (kevent64(engine().io_poller().queue_fd(), &event, 1, nullptr, 0, 0, &zero_timeout) < 0)
            {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't remove kevent for Timer {}", timer_fd_);
            }
        }

        fired_times_++;

        fibers::fiber([&]{
            timer_fn_();
        }).detach();

        if (fired_times <= count_) {
            count_ -= fired_times;
        }
        else {
            count_ = 0;
        }

        if (count_ == 0) {
            cancel();
        }
    }
}


class WakeupMessage : public KEventIOMessage {

    FiberContext* context_;

public:
    WakeupMessage(int cpu) :
        KEventIOMessage(cpu)
    {
        context_ = fibers::context::active();
    }

    virtual ~WakeupMessage() = default;

    virtual void process() noexcept {}
    virtual void finish()
    {
        engine().scheduler()->resume(context_);
    };

    void wait_for() {
        engine().scheduler()->suspend(context_);
    }

    virtual void on_receive(const kevent64_s& event) {}


    virtual std::string describe() { return "WakeupMessage"; }
};



void IOPoller::sleep_for(const std::chrono::milliseconds& time)
{
    if (time != std::chrono::milliseconds::zero())
    {
        uint64_t tfd = engine().io_poller().new_timer_fd();
        if (tfd > 0)
        {
            kevent64_s event = tools::make_zeroed<kevent64_s>();

            WakeupMessage message(engine().cpu());

            EV_SET64(
                &event,
                tfd,
                EVFILT_TIMER,
                EV_ADD | EV_ONESHOT,
                NOTE_USECONDS,
                time.count() * 1000,
                (uint64_t)&message,
                0,0
            );

            timespec zero_timeout = tools::make_zeroed<timespec>();

            if (kevent64(engine().io_poller().queue_fd(), &event, 1, nullptr, 0, 0, &zero_timeout) < 0)
            {
                MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't configure kevent for Timer {}", tfd);
            }

            message.wait_for();
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't create timer for engine.sleep_for()");
        }
    }
}




}}}
