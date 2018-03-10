
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

#include <boost/assert.hpp>

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include <memory>


namespace memoria {
namespace v1 {
namespace reactor {
    


ClientSocketImpl::ClientSocketImpl(const IPAddress& ip_address, uint16_t ip_port):
    Base(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)),
    ip_address_(ip_address),
    ip_port_(ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()},
    fiber_io_message_(engine().cpu(), "::client_socket_connection")
{
    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");

    if (fd_ < 0)
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't create socket for {}", ip_address_);
    }

    int flags = ::fcntl(fd_, F_GETFL, 0);

    if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(
                                             u"Can't configure ClientSocket for AIO {}:{}:{}",
                                             ip_address_, ip_port_, fd_
                                         );
    }

    epoll_event event = tools::make_zeroed<epoll_event>();

    event.data.ptr = &fiber_io_message_;

    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;

    int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, fd_, &event);
    if (sres < 0) {
        MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't configure poller for {}:{}", ip_address_, ip_port_);
    }

    connect();
}



void ClientSocketImpl::connect()
{
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = htons(ip_port_);

    while(true)
    {
        int fd = ::connect(fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));

        if (fd >= 0)
        {
           break;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
        {
            fiber_io_message_.wait_for();
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't start connection with {}:{}", ip_address_, ip_port_);
        }
    }
}

ClientSocketImpl::~ClientSocketImpl() noexcept
{
    close();
}

void ClientSocketImpl::close()
{
    if (!op_closed_)
    {
        op_closed_ = true;
        int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, fd_, nullptr);
        if (res < 0)
        {
            int32_t err_code = errno;
            ::close(fd_);
            MMA1_THROW(SystemException(err_code)) << fmt::format_ex(u"Can't remove epoller for socket {}:{}", ip_address_, ip_port_);
        }

        engine().drain_pending_io_events(&fiber_io_message_);

        if (::close(fd_) < 0)
        {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't close socket {}:{}", ip_address_, ip_port_);
        }
    }
}

size_t ClientSocketImpl::read(uint8_t* data, size_t size)
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
        else if (errno == ECONNRESET || errno == ECONNABORTED) {
            data_closed_ = true;
            return 0;
        }
        else {            
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Error reading from socket connection for {}:{}:{}", ip_address_, ip_port_, fd_);
        }
    }
}

size_t ClientSocketImpl::write_(const uint8_t* data, size_t size)
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
        else if (errno == ECONNRESET || errno == ECONNABORTED) {
            data_closed_ = true;
            return 0;
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(u"Error writing to socket connection for {}:{}:{}", ip_address_, ip_port_, fd_);
        }
    }
}





}}}
