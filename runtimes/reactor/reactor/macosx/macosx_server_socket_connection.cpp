
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


#include <memoria/reactor/macosx/macosx_socket_impl.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/perror.hpp>

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
namespace reactor {

ServerSocketConnectionImpl::ServerSocketConnectionImpl(SocketConnectionData&& data):
        SocketConnectionImpl(data.take_fd()),
        ip_address_(data.ip_address()),
        ip_port_(data.ip_port()),
        fiber_io_read_message_(engine().cpu()),
        fiber_io_write_message_(engine().cpu())
{
    if (fd_ >= 0)
    {
        int flags = ::fcntl(fd_, F_GETFL, 0);
        if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA_THROW(SystemException()) << format_ex(
                "Can't configure StreamSocketConnection for AIO {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }

        int queue_fd = engine().io_poller().queue_fd();

        KEvent64(queue_fd, fd_, EVFILT_READ, EV_ADD | EV_ERROR, &fiber_io_read_message_);
        KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_ADD | EV_ERROR, &fiber_io_write_message_);
    }
    else {
        MMA_THROW(RuntimeException()) << WhatCInfo(
            "Connection has been already created for this SocketConnectionData"
        );
    }
}

ServerSocketConnectionImpl::~ServerSocketConnectionImpl() noexcept
{
    if (!op_closed_)
    {
        int queue_fd = engine().io_poller().queue_fd();

        KEvent64(queue_fd, fd_, EVFILT_READ, EV_DELETE);
        KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_DELETE);

        ::close(fd_);
    }
}


void ServerSocketConnectionImpl::close()
{
    if (!op_closed_)
    {
        int queue_fd = engine().io_poller().queue_fd();

        KEvent64(queue_fd, fd_, EVFILT_READ, EV_DELETE);
        KEvent64(queue_fd, fd_, EVFILT_WRITE, EV_DELETE);

        if (::close(fd_) < 0)
        {
            MMA_THROW(SystemException()) << format_ex(
                "Can't close socket {}:{}",
                ip_address_, ip_port_
            );
        }

        op_closed_ = true;
    }
}

size_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size)
{
    size_t available_size = size;
    
    while (!data_closed_)
    {
        ssize_t result = ::read(fd_, data, available_size);
        
        if (result >= 0)
        {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            // FIXME: handle eof flag properlyd
            fiber_io_read_message_.wait_for();

            size_t av = fiber_io_read_message_.available();
            available_size = av >= size ? size : av;
        }
        else if (errno == EPIPE || errno == ECONNRESET || errno == ECONNABORTED) {
            data_closed_ = true;
            return 0;
        }
        else {
            MMA_THROW(SystemException()) << format_ex(
                "Error reading from socket connection for {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }
    }

    return 0;
}

size_t ServerSocketConnectionImpl::write_(const uint8_t* data, size_t size)
{
    size_t available_size = size;
    while (!data_closed_)
    {
        ssize_t result = ::write(fd_, data, available_size);

        if (result >= 0) {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // FIXME: handle eof flag properly
            fiber_io_write_message_.wait_for();

            size_t av = fiber_io_write_message_.available();
            available_size = av >= size ? size : av;
        }
        else if (errno == EPIPE || errno == ECONNRESET || errno == ECONNABORTED) {
            data_closed_ = true;
            return 0;
        }
        else {
            MMA_THROW(SystemException()) << format_ex(
                "Error writing to socket connection for {}:{}:{}",
                ip_address_, ip_port_, fd_
            );
        }
    }

    return 0;
}

}}
