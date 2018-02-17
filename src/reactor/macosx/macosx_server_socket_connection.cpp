
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


#include <memoria/v1/reactor/macosx/macosx_socket_impl.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/core/tools/perror.hpp>

#include "macosx_socket.hpp"

#include <boost/assert.hpp>

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>


#include <memory>


namespace memoria {
namespace v1 {
namespace reactor {



ServerSocketConnectionImpl::ServerSocketConnectionImpl(SocketConnectionData&& data):
        SocketConnectionImpl(data.take_fd()),
        ip_address_(data.ip_address()),
        ip_port_(data.ip_port()),
        fiber_io_message_(engine().cpu())
{
    if (fd_ >= 0)
    {
        int flags = ::fcntl(fd_, F_GETFL, 0);
        if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Can't configure StreamSocketConnection for AIO {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }

        int queue_fd = engine().io_poller().queue_fd();

        timespec timeout = tools::make_zeroed<timespec>();

        struct kevent64_s event;

        EV_SET64(&event, fd_, EVFILT_READ, EV_ADD, 0, 0, (uint64_t)&fiber_io_message_, 0, 0);

        int res = ::kevent64(queue_fd, &event, 1, nullptr, 0, 0, &timeout);
        if (res < 0)
        {
            ::close(fd_);
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Can't configure poller for connection {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }
    }
    else {
        MMA1_THROW(RuntimeException()) << WhatCInfo(
            "Connection has been already created for this SocketConnectionData"
        );
    }
}

ServerSocketConnectionImpl::~ServerSocketConnectionImpl() noexcept
{
    int queue_fd = engine().io_poller().queue_fd();
    
    timespec timeout = tools::make_zeroed<timespec>();
    
    struct kevent64_s event;
    
    EV_SET64(&event, fd_, EVFILT_READ, EV_DELETE, 0, 0, 0, 0, 0);
    
    int res = ::kevent64(queue_fd, &event, 1, nullptr, 0, 0, &timeout);
    if (res < 0)
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(
            u"Can't remove kqueu event for connection {}:{}:{}",
            ip_address_, ip_port_, fd_
        );
    }

    if (::close(fd_) < 0)
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(
            u"Can't close connection {}:{}:{}",
            ip_address_, ip_port_, fd_
        );
    } 
}


void ServerSocketConnectionImpl::close()
{
    if (!op_closed_)
    {
        int queue_fd = engine().io_poller().queue_fd();

        timespec timeout = tools::make_zeroed<timespec>();

        struct kevent64_s event;

        EV_SET64(&event, fd_, EVFILT_READ, EV_DELETE, 0, 0, 0, 0, 0);

        int res = ::kevent64(queue_fd, &event, 1, nullptr, 0, 0, &timeout);
        if (res < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Can't remove kqueue event for socket {}:{}",
                ip_address_, ip_port_
            );
        }

        if (::close(fd_) < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Can't close socket {}:{}",
                ip_address_, ip_port_
            );
        }

        op_closed_ = true;
    }
}

size_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size)
{
    size_t available_size = size;
    
    while (true) 
    {
        ssize_t result = ::read(fd_, data, available_size);
        
        if (result >= 0) {
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            // FIXME: handle eof flag properlyd
            fiber_io_message_.wait_for();
            available_size = fiber_io_message_.available();
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Error reading from socket connection for {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }
    }
}

size_t ServerSocketConnectionImpl::write(const uint8_t* data, size_t size)
{
    size_t available_size = size;
    while (true) 
    {
        ssize_t result = ::write(fd_, data, available_size);
        
        if (result >= 0) {
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            // FIXME: handle eof flag properly
            fiber_io_message_.wait_for();
            available_size = fiber_io_message_.available();
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Error writing to socket connection for {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }
    }
}



    
}}}
