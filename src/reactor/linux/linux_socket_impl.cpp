
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


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


ssize_t ServerSocketConnectionImpl::read(uint8_t* data, size_t size)
{
    while (true) 
    {
        ssize_t result = ::read(fd_, data, size);
        
        if (result >= 0) {
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

ssize_t ServerSocketConnectionImpl::write(const uint8_t* data, size_t size)
{
    while (true) 
    {
        ssize_t result = ::write(fd_, data, size);
        
        if (result >= 0) {
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

        if (::close(fd_) < 0)
        {
            tools::rise_perror(SBuf() << "Can't close connection " << ip_address_ << ":" << ip_port_ << ":" << fd_);
        }
    }
}





    







ServerSocketImpl::ServerSocketImpl(const IPAddress& ip_address, uint16_t ip_port):
    SocketImpl(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)),
    ip_address_(ip_address),
    ip_port_(ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()},
    fiber_io_message_(engine().cpu())
{
    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");

    if (fd_ < 0)
    {
        tools::rise_perror(SBuf() << "Can't create socket for " << ip_address_);
    }

    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = htons(ip_port_);

    int bres = ::bind(fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));

    if (bres < 0)
    {
        ::close(fd_);
        tools::rise_perror(SBuf() << "Can't bind socket to " << ip_address_ << ":" << ip_port_);
    }

    epoll_event event = tools::make_zeroed<epoll_event>();

    event.data.ptr = &fiber_io_message_;

    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, fd_, &event);
    if (sres < 0) {
        tools::rise_perror(SBuf() << "Can't configure poller for " << ip_address_ << ":" << ip_port_);
    }
}



ServerSocketImpl::~ServerSocketImpl() noexcept {
    close();
}

void ServerSocketImpl::close()
{
    if (!closed_)
    {
        closed_ = true;
        int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, fd_, nullptr);
        if (res < 0)
        {
            ::close(fd_);
            tools::rise_perror(SBuf() << "Can't remove epoller for socket " << ip_address_ << ":" << ip_port_);
        }

        if (::close(fd_) < 0)
        {
            tools::rise_perror(SBuf() << "Can't close socket " << ip_address_ << ":" << ip_port_);
        }
    }
}

void ServerSocketImpl::listen()
{
    int res = ::listen(fd_, 5);
    if (res < 0)
    {
        tools::rise_perror(SBuf() << "Can't start listening on socket for " << ip_address_ << ":" << ip_port_);
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
            tools::rise_perror(SBuf() << "Can't start accepting connections for " << ip_address_ << ":" << ip_port_);
        }
    }
}




ClientSocketImpl::ClientSocketImpl(const IPAddress& ip_address, uint16_t ip_port):
    Base(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0)),
    ip_address_(ip_address),
    ip_port_(ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()},
    fiber_io_message_(engine().cpu())
{
    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");

    if (fd_ < 0){
        tools::rise_perror(SBuf() << "Can't create socket for " << ip_address_);
    }

    int flags = ::fcntl(fd_, F_GETFL, 0);

    if (::fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        tools::rise_perror(
                    SBuf() << "Can't configure ClientSocket for AIO "
                    << ip_address_ << ":" << ip_port_ << ":" << fd_
        );
    }

    epoll_event event = tools::make_zeroed<epoll_event>();

    event.data.ptr = &fiber_io_message_;

    event.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;

    int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, fd_, &event);
    if (sres < 0) {
        tools::rise_perror(SBuf() << "Can't configure poller for " << ip_address_ << ":" << ip_port_);
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
            tools::rise_perror(SBuf() << "Can't start connection to " << ip_address_ << ":" << ip_port_);
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
            ::close(fd_);
            tools::rise_perror(SBuf() << "Can't remove epoller for socket " << ip_address_ << ":" << ip_port_);
        }

        if (::close(fd_) < 0)
        {
            tools::rise_perror(SBuf() << "Can't close socket " << ip_address_ << ":" << ip_port_);
        }
    }
}

ssize_t ClientSocketImpl::read(uint8_t* data, size_t size)
{
    while (true)
    {
        ssize_t result = ::read(fd_, data, size);

        if (result >= 0) {
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

ssize_t ClientSocketImpl::write(const uint8_t* data, size_t size)
{
    while (true)
    {
        ssize_t result = ::write(fd_, data, size);

        if (result >= 0) {
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

}}}
