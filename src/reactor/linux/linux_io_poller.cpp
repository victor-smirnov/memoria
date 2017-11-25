
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


#include <memoria/v1/reactor/linux/linux_io_poller.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include "linux_io_messages.hpp"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>



namespace memoria {
namespace v1 {
namespace reactor {

int io_setup(unsigned nr, aio_context_t *ctxp)
{
    return syscall(__NR_io_setup, nr, ctxp);
}    
    
int io_destroy(aio_context_t ctx) 
{
    return syscall(__NR_io_destroy, ctx);
}

int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp) 
{
    return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

int io_getevents(aio_context_t ctx, long min_nr, long max_nr, struct io_event *events, struct timespec *timeout)
{
    return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}    

void assert_ok(int result, const char* msg)
{
    if (result < 0)
    {
        std::cout << msg << " -- " << strerror(errno) << std::endl;
        std::terminate();
    }
}

IOPoller::IOPoller(int cpu, IOBuffer& buffer):
    cpu_(cpu), buffer_(buffer)
{
    int result = io_setup(BATCH_SIZE, &aio_context_);
    if (result < 0)
    {
        std::cout << "Can't initialize file AIO subsystem: " << result << std::endl;
        std::terminate();
    }
    
    epoll_fd_ = epoll_create(BATCH_SIZE);
    
    assert_ok(epoll_fd_, "Can't initialize file EPOLL subsystem");
    
    event_fd_ = eventfd(0, EFD_NONBLOCK); //
    
    assert_ok(event_fd_, "Can't initialize file EVENTFD subsystem");
    
    epoll_event ev0;
    ev0.events = EPOLLIN | EPOLLOUT ; //| EPOLLET
    ev0.data.ptr = &event_fd_;
    
    assert_ok(
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd_, &ev0),
        "Can't configure EPOLLFD"
    );
}

IOPoller::~IOPoller() 
{
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_fd_, nullptr) == -1) 
    {
        tools::report_perror(SBuf() << "Can't stop watching eventfd events");
    }
    
    ::close(event_fd_);
    
    if (io_destroy(aio_context_) < 0) {
        tools::report_perror(SBuf() << "Can't stop watching file AIO events");
    }
    
    //FIXME: How to close/destroy epoll_fd_?
}

void IOPoller::poll() 
{
    int buffer_capacity = buffer_.capacity_i();
    if (buffer_capacity > 0)
    {
        epoll_event eevents[BATCH_SIZE];
        
        sigset_t sigmask;
        
        int max_events = std::min(buffer_capacity, (int)BATCH_SIZE);
        
        int epoll_result = epoll_pwait(epoll_fd_, eevents, max_events, 0, &sigmask);
        
        if (epoll_result >= 0)
        {
            for (int c = 0; c < epoll_result; c++) 
            {    
                if (eevents[c].data.ptr == &event_fd_) 
                {
                    if (read_eventfd()) {
                        poll_file_events(buffer_capacity, epoll_result - 1); // FIXME take timerfd into account too!!! 
                    }
                }
                else if (eevents[c].data.ptr)
                {
                    EPollIOMessage* msg = tools::ptr_cast<EPollIOMessage>(eevents[c].data.ptr);
                    msg->on_receive(eevents[c]);
                    buffer_.push_front(msg);
                }
            }
        }
        else {
            std::cout << "epoll_pwait failed: " << epoll_result << std::endl;
            std::terminate();
        }
    }
}

void IOPoller::poll_file_events(int buffer_capacity, int other_events)
{
    struct io_event events[BATCH_SIZE];

    int max_events = std::min(buffer_capacity - other_events, (int)BATCH_SIZE);
    
    timespec timeout{0, 0}; 
    int e_num = io_getevents(aio_context_, 1, max_events, events, &timeout);
    
    if (e_num >= 0) 
    {
        for (int c = 0; c < e_num; c++)
        {
            FileIOMessage* msg = tools::ptr_cast<FileIOMessage>((void*)events[c].data);
            msg->process();
            msg->report(&events[c]);
            buffer_.push_front(msg);
        }
    }
    else {
        std::cout << "io_getevents failed: " << e_num << std::endl;
        std::terminate();
    }
}
    
ssize_t IOPoller::read_eventfd() 
{
    return true;//for now
    
    /*char data[8];
    ssize_t res = ::read(event_fd_, data, sizeof(data));
    
    if (res == 8) {
        return true;
    }
    else if (errno == EAGAIN) {
        return false;
    }
    else {
        tools::report_perror(tools::SBuf() << "Can't read counters form eventfd. Aborting. ");
        std::terminate();
    }*/
}

    
}}}
