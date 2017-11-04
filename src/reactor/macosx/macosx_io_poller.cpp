
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


#include <memoria/v1/reactor/macosx/macosx_io_poller.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/perror.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>

#include <memoria/v1/reactor/message/fiber_io_message.hpp>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif



#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>



namespace memoria {
namespace v1 {
namespace reactor {


void assert_ok(int result, const char* msg)
{
    if (result < 0)
    {
        std::cout << msg << " -- " << strerror(errno) << std::endl;
        std::terminate();
    }
}

IOPoller::IOPoller(IOBuffer& buffer):buffer_(buffer) 
{    
    queue_fd_ = ::kqueue();
    
    assert_ok(queue_fd_, "Can't initialize file kqueue subsystem");
}

IOPoller::~IOPoller() 
{
    ::close(queue_fd_);
}

void IOPoller::poll() 
{
    timespec zero_timeout = tools::make_zeroed<timespec>();
    
    int buffer_capacity = buffer_.capacity_i();
    if (buffer_capacity > 0)
    {
        struct kevent eevents[BATCH_SIZE];
        
        int max_events = std::min(buffer_capacity, (int)BATCH_SIZE);
        
        int kevent_result = kevent(queue_fd_, nullptr, 0, eevents, max_events, &zero_timeout);
        
        if (kevent_result >= 0)
        {
            for (int c = 0; c < kevent_result; c++) 
            {    
                if (eevents[c].udata && (eevents[c].filter == EVFILT_READ || eevents[c].filter == EVFILT_WRITE))
                {
                    FiberIOMessage* msg = tools::ptr_cast<FiberIOMessage>(eevents[c].udata);
                    msg->configure(eevents[c].flags & EV_EOF, eevents[c].data);
                    
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


    
}}}
