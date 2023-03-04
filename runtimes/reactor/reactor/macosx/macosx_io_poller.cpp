
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


#include <memoria/reactor/macosx/macosx_io_poller.hpp>
#include <memoria/core/memory/ptr_cast.hpp>
#include <memoria/core/tools/perror.hpp>
#include <memoria/core/tools/bzero_struct.hpp>

#include "macosx_io_messages.hpp"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>

namespace memoria {
namespace reactor {

void assert_ok(int result, const char* msg)
{
    if (result < 0)
    {
        std::cout << msg << " -- " << strerror(errno) << std::endl;
        std::terminate();
    }
}

IOPoller::IOPoller(int cpu, IOBuffer& buffer):
    buffer_(buffer),
    cpu_(cpu)
{    
    queue_fd_ = ::kqueue();
    
    assert_ok(queue_fd_, "Can't initialize file kqueue subsystem");
}

IOPoller::~IOPoller() 
{
    ::close(queue_fd_);
}

void IOPoller::poll(uint64_t timeout_ms)
{
    timespec timeout = tools::make_zeroed<timespec>();
    timeout.tv_nsec = timeout_ms * 1000000;
    
    int buffer_capacity = buffer_.capacity_i();
    if (buffer_capacity > 0)
    {
        struct kevent64_s eevents[BATCH_SIZE];
        
        int max_events = std::min(buffer_capacity, (int)BATCH_SIZE);
        
        int kevent_result = kevent64(queue_fd_, nullptr, 0, eevents, max_events, 0, &timeout);
        
        if (kevent_result >= 0)
        {
            for (int c = 0; c < kevent_result; c++) 
            {    
                if (eevents[c].udata)
                {
                    KEventIOMessage* msg = ptr_cast<KEventIOMessage>((char*)eevents[c].udata);
                    msg->on_receive(eevents[c]);
                    
                    buffer_.push_front(msg);
                }
            }
        }
        else {
            std::cout << "kevent failed: " << kevent_result << std::endl;
            std::terminate();
        }
    }
}

void KEvent64(int poller_fd, int fd, int filter, int flags, void * fiber_message_ptr, bool throw_ex)
{
    timespec timeout = tools::make_zeroed<timespec>();

    struct kevent64_s event;

    EV_SET64(&event, fd, filter, flags, 0, 0, value_cast<uint64_t>(fiber_message_ptr), 0, 0);

    int res = ::kevent64(poller_fd, &event, 1, nullptr, 0, 0, &timeout);
    if (res < 0)
    {
        ::close(fd);

        if (throw_ex)
        {
            MMA_THROW(SystemException()) << format_ex(
                "Can't configure poller for connection {}", fd
            );
        }
    }

}
    
}}
