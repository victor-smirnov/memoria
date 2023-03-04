
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


#include <memoria/reactor/linux/linux_socket_impl.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/core/tools/perror.hpp>

#include "linux_socket.hpp"

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <memory>

#include <boost/assert.hpp>

namespace memoria {
namespace reactor {

ServerSocketConnectionImpl::ServerSocketConnectionImpl(SocketConnectionData&& data):
        SocketConnectionImpl(data.take_fd()),
        ip_address_(data.ip_address()),
        ip_port_(data.ip_port()),
        fiber_io_message_(engine().cpu(), "::server_socket_connection")
{
    if (fd_ >= 0)
    {
        int flags = ::fcntl(fd_, F_GETFL, 0);

        if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            MMA_THROW(SystemException()) << format_ex("Can't configure StreamSocketConnection for AIO {}:{}:{}", ip_address_, ip_port_, fd_);
        }


        epoll_event event = tools::make_zeroed<epoll_event>();

        event.data.ptr = &fiber_io_message_;

        event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;

        int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, fd_, &event);
        if (res < 0)
        {
            int32_t err_code = errno;
            ::close(fd_);
            MMA_THROW(SystemException(err_code)) << format_ex("Can't configure poller for connection {}:{}:{}", ip_address_, ip_port_, fd_);
        }
    }
    else {
        MMA_THROW(RuntimeException()) << WhatCInfo("Connection has been already created for this SocketConnectionData");
    }
}

ServerSocketConnectionImpl::~ServerSocketConnectionImpl() noexcept
{
    close();
}


size_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size)
{
    if (op_closed_ || data_closed_) {
        return 0;
    }

    while (true) 
    {
        ssize_t result = ::recv(fd_, data, size, MSG_NOSIGNAL);
        if (result >= 0) {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {            
            ctx_ = boost::fibers::context::active();
            fiber_io_message_.wait_for();

            if (op_closed_ || data_closed_) {
                return 0;
            }

            ctx_ = nullptr;
        }
        else if (errno == ECONNRESET || errno == ECONNABORTED || errno == EBADF) {
            data_closed_ = true;
            return 0;
        }
        else {
            MMA_THROW(SystemException()) << format_ex("Error reading from socket connection for {}:{}:{}", ip_address_, ip_port_, fd_);
        }
    }
}

size_t ServerSocketConnectionImpl::write_(const uint8_t* data, size_t size)
{
    while (true) 
    {
        ssize_t result = ::send(fd_, data, size, MSG_NOSIGNAL);
        
        if (result >= 0) {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            ctx_ = boost::fibers::context::active();
            fiber_io_message_.wait_for();

            if (op_closed_ || data_closed_) {
                return 0;
            }
            ctx_ = nullptr;
        }
        else if (errno == ECONNRESET || errno == ECONNABORTED) {
            data_closed_ = true;
            return 0;
        }
        else {
            MMA_THROW(SystemException()) << format_ex("Error writing to socket connection for {}:{}:{}", ip_address_, ip_port_, fd_);
        }
    }
}


void ServerSocketConnectionImpl::close()
{
    if (!op_closed_)
    {
        op_closed_ = true;

        int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, fd_, nullptr);
        if (res < 0)
        {
            int32_t err_code = errno;
            ::shutdown(fd_, SHUT_RDWR);
            MMA_THROW(SystemException(err_code)) << format_ex("Can't remove epoller for connection {}:{}:{}", ip_address_, ip_port_, fd_);
        }
        engine().drain_pending_io_events(&fiber_io_message_);

        if (::shutdown(fd_, SHUT_RDWR) < 0)
        {
            MMA_THROW(SystemException()) << format_ex("Can't close connection {}:{}:{}", ip_address_, ip_port_, fd_);
        }

        if (ctx_) {
            engine().scheduler()->resume(ctx_);
        }
    }
}



}}
