
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


#include <memoria/v1/reactor/linux/linux_socket_impl.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/core/tools/perror.hpp>

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
            tools::rise_perror(
                        SBuf() << "Can't configure StreamSocketConnection for AIO "
                        << ip_address_ << ":" << ip_port_ << ":" << fd_
            );
        }


        epoll_event event = tools::make_zeroed<epoll_event>();

        event.data.ptr = &fiber_io_message_;

        event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

        int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, fd_, &event);
        if (res < 0)
        {
            ::close(fd_);
            tools::rise_perror(
                        SBuf() << "Can't configure poller for connection "
                        << ip_address_ << ":" << ip_port_
                        << ":" << fd_
            );
        }
    }
    else {
        tools::rise_error(SBuf() << "Connection has been already created for this SocketConnectionData");
    }
}

ServerSocketConnectionImpl::~ServerSocketConnectionImpl() noexcept
{
    close();
}


size_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size)
{
    while (true) 
    {
        ssize_t result = ::read(fd_, data, size);
        
        if (result >= 0) {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            fiber_io_message_.wait_for();
        }
        else {
            tools::rise_perror(
                        SBuf() << "Error reading from socket connection for "
                        << ip_address_ << ":" << ip_port_ << ":" << fd_
            );
        }
    }
}

size_t ServerSocketConnectionImpl::write(const uint8_t* data, size_t size)
{
    while (true) 
    {
        ssize_t result = ::write(fd_, data, size);
        
        if (result >= 0) {
            data_closed_ = result == 0;
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            fiber_io_message_.wait_for();
        }
        else {
            tools::rise_perror(
                        SBuf() << "Error reading from socket connection for "
                        << ip_address_ << ":" << ip_port_ << ":"
                        << fd_
            );
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
            ::close(fd_);
            tools::rise_perror(SBuf() << "Can't remove epoller for connection " << ip_address_ << ":" << ip_port_ << ":" << fd_);
        }

        engine().drain_pending_io_events(&fiber_io_message_);

        if (::close(fd_) < 0)
        {
            tools::rise_perror(SBuf() << "Can't close connection " << ip_address_ << ":" << ip_port_ << ":" << fd_);
        }
    }
}



}}}
