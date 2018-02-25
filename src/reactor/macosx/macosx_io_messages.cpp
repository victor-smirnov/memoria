
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

#include "macosx_io_messages.hpp"
#include "macosx_timer.hpp"

#include <boost/assert.hpp>




namespace memoria {
namespace v1 {
namespace reactor {


void SocketIOMessage::finish()
{
    while (!iowait_queue_.empty())
    {
        auto* fiber_context = &iowait_queue_.front();
        iowait_queue_.pop_front();
        engine().scheduler()->resume(fiber_context);
    }
}    




void SocketIOMessage::wait_for()
{
    reset();

    FiberContext* fiber_context = fibers::context::active();
    
    fiber_context->iowait_link(iowait_queue_);
    
    engine().scheduler()->suspend(fiber_context);
}


void TimerMessage::finish()
{
    timer_->on_firing(fired_times_);
}

void TimerMessage::on_receive(const kevent64_s& event)
{
    fired_times_ = event.data;
}
    
}}}
