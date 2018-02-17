
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








    
ServerSocketImpl::ServerSocketImpl(const IPAddress& ip_address, uint16_t ip_port):
    SocketImpl(socket(AF_INET, SOCK_STREAM, 0)),
    ip_address_(ip_address),
    ip_port_(ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()},
    fiber_io_message_(engine().cpu())
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
            u"Can't configure StreamSocketConnection for AIO: {}:{}:{}",
            ip_address_, ip_port_, fd_
        );
    }
    
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = htons(ip_port_);
    
    int bres = ::bind(fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));
    
    if (bres < 0)
    {
        ::close(fd_);
        MMA1_THROW(SystemException()) << fmt::format_ex(
            u"Can't bind socket to {}:{}",
            ip_address_, ip_port_
        );
    }
    
    
    int queue_fd = engine().io_poller().queue_fd();
    
    timespec timeout = tools::make_zeroed<timespec>();
    
    struct kevent64_s event;
    
    EV_SET64(&event, fd_, EVFILT_READ, EV_ADD, 0, 0, (uint64_t)&fiber_io_message_, 0, 0);
    
    int sres = ::kevent64(queue_fd, &event, 1, nullptr, 0, 0, &timeout);
    if (sres < 0) {
        MMA1_THROW(SystemException()) << fmt::format_ex(
            u"Can't configure kqueue for {}:{}",
            ip_address_, ip_port_
        );
    }
}



    
ServerSocketImpl::~ServerSocketImpl() noexcept
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
}    


void ServerSocketImpl::close()
{
    if (!closed_)
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

        closed_ = true;
    }
}


void ServerSocketImpl::listen()
{
    int res = ::listen(fd_, 5);
    if (res < 0) 
    {
        MMA1_THROW(SystemException()) << fmt::format_ex(
            u"Can't start listening on socket for {}:{}",
            ip_address_, ip_port_
        );
    }
}




SocketConnectionData ServerSocketImpl::accept()
{
    sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);

    while(true) 
    {
        int fd = ::accept(fd_, tools::ptr_cast<sockaddr>(&client_addr), &cli_len);

        if (fd >= 0) 
        {
            return SocketConnectionData(fd, ip_address_, ip_port_);
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            fiber_io_message_.wait_for();
        }
        else {
            MMA1_THROW(SystemException()) << fmt::format_ex(
                u"Can't start accepting connections for {}:{}",
                ip_address_, ip_port_
            );
        }
    }
}


    
}}}
