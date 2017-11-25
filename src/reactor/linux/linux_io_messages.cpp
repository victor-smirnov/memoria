
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


#include <memoria/v1/reactor/message/fiber_io_message.hpp>
#include <memoria/v1/reactor/reactor.hpp>

#include "linux_io_messages.hpp"

#include <boost/assert.hpp>

namespace memoria {
namespace v1 {
namespace reactor {


void SocketIOMessage::finish()
{
    if (!iowait_queue_.empty())
    {
        auto* fiber_context = &iowait_queue_.front();
        iowait_queue_.pop_front();
        engine().scheduler()->resume(fiber_context);
    }
    else {
        std::cout << "Empty iowait_queue for " << describe() << ". Aborting." << std::endl;
        std::terminate();
    }
}    




void SocketIOMessage::wait_for()
{
    reset();

    FiberContext* fiber_context = fibers::context::active();
    
    fiber_context->iowait_link(iowait_queue_);
    
    engine().scheduler()->suspend(fiber_context);

    connection_closed_ = connection_closed_ || (flags_ & (EPOLLRDHUP | EPOLLHUP | EPOLLERR));
}


void TimerMessage::finish()
{
    auto fired_times = fired_times_;

    for (auto c = 0; c < fired_times; c++)
    {
        fibers::fiber([&]{
            timer_fn_();
        }).detach();
    }

    fired_times_ -= fired_times;
}

void TimerMessage::on_receive(const epoll_event& event)
{
    uint64_t counts;
    ::read(fd_, &counts, sizeof (counts));

    fired_times_ += counts;
}
    
}}}
