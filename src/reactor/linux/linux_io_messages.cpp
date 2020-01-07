
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
#include "linux_timer.hpp"

#include <boost/assert.hpp>

namespace memoria {
namespace v1 {
namespace reactor {


void SocketIOMessage::finish()
{
    if (context_)
    {
        auto* fiber_context = context_;
        context_ = nullptr;
        engine().scheduler()->resume(fiber_context);
    }
}    




void SocketIOMessage::wait_for()
{
    reset();

    BOOST_ASSERT(context_ == nullptr);

    context_ = fibers::context::active();
    engine().scheduler()->suspend(context_);

    // Currently we use only read() == 0 for closed connection indication
    //connection_closed_ = connection_closed_ || (flags_ & ~(EPOLLRDHUP | EPOLLHUP | EPOLLERR));
}


void TimerMessage::finish()
{
    timer_->on_firing(fired_times_);
}

void TimerMessage::on_receive(const epoll_event& event)
{
    uint64_t counts;
    ::read(fd_, &counts, sizeof (counts));

    fired_times_ = counts;
}
    
}}}
