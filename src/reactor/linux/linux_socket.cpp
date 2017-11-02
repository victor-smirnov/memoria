
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
    


StreamSocketConnection::StreamSocketConnection(int connection_fd, const std::shared_ptr<StreamSocket>& socket): 
        socket_(socket),
        connection_fd_(connection_fd),
        fiber_io_message_(engine().cpu())
{
    int flags = ::fcntl(connection_fd, F_GETFL, 0);
    
    if (::fcntl(connection_fd, F_SETFL, flags | O_NONBLOCK) < 0) 
    {
        tools::rise_perror(SBuf() << "Can't configure StreamSocketConnection for AIO " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
    }
    
    
    epoll_event event = tools::make_zeroed<epoll_event>();
    
    event.data.ptr = &fiber_io_message_;
    
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;

    int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, connection_fd_, &event);
    if (res < 0)
    {
        ::close(connection_fd_);
        tools::rise_perror(SBuf() << "Can't configure poller for connection " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
    }
}

StreamSocketConnection::~StreamSocketConnection() noexcept
{
    std::cout << "Closing connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_ << std::endl;
    
    int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, connection_fd_, nullptr);
    if (res < 0)
    {
        tools::report_perror(SBuf() << "Can't remove epoller for connection " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
    }
        
    if (::close(connection_fd_) < 0)
    {
        tools::report_perror(SBuf() << "Can't close connection " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
    } 
}


ssize_t StreamSocketConnection::read(char* data, size_t size) 
{
    while (true) 
    {
        ssize_t result = ::read(connection_fd_, data, size);
        
        if (result >= 0) {
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            fiber_io_message_.wait_for();
        }
        else {
            tools::rise_perror(SBuf() << "Error reading from socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
        }
    }
}

ssize_t StreamSocketConnection::write(const char* data, size_t size) 
{
    while (true) 
    {
        ssize_t result = ::write(connection_fd_, data, size);
        
        if (result >= 0) {
            return result;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            fiber_io_message_.wait_for();
        }
        else {
            tools::rise_perror(SBuf() << "Error reading from socket connection for " << socket_->address() << ":" << socket_->port() << ":" << connection_fd_);
        }
    }
}






    
StreamServerSocket::StreamServerSocket(const IPAddress& ip_address, uint16_t ip_port): 
    StreamSocket(ip_address, ip_port),
    sock_address_{tools::make_zeroed<sockaddr_in>()},
    fiber_io_message_(engine().cpu())
{
    BOOST_ASSERT_MSG(ip_address_.is_v4(), "Only IPv4 sockets are supported at the moment");
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK , 0);
    
    if (socket_fd_ < 0) 
    {
        tools::rise_perror(SBuf() << "Can't create socket for " << ip_address_);
    }
    
    sock_address_.sin_family        = AF_INET;
    sock_address_.sin_addr.s_addr   = ip_address_.to_in_addr().s_addr;
    sock_address_.sin_port          = htons(ip_port_);
    
    int bres = ::bind(socket_fd_, tools::ptr_cast<sockaddr>(&sock_address_), sizeof(sock_address_));
    
    if (bres < 0)
    {
        ::close(socket_fd_);
        tools::rise_perror(SBuf() << "Can't bind socket to " << ip_address_ << ":" << ip_port_);
    }
    
    epoll_event event = tools::make_zeroed<epoll_event>();
            
    event.data.ptr = &fiber_io_message_;
            
    event.events = EPOLLIN;
            
    int sres = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_ADD, socket_fd_, &event);
    if (sres < 0) {
        tools::rise_perror(SBuf() << "Can't configure poller for " << ip_address_ << ":" << ip_port_);
    }
}



    
StreamServerSocket::~StreamServerSocket() noexcept 
{    
    std::cout << "Closing socket for " << ip_address_ << ":" << ip_port_ << std::endl;
    int res = ::epoll_ctl(engine().io_poller().epoll_fd(), EPOLL_CTL_DEL, socket_fd_, nullptr);
    if (res < 0)
    {
        tools::report_perror(SBuf() << "Can't remove epoller for socket " << ip_address_ << ":" << ip_port_);
    }
        
    if (::close(socket_fd_) < 0) 
    {
        tools::report_perror(SBuf() << "Can't close socket " << ip_address_ << ":" << ip_port_);
    }   
}    



void StreamServerSocket::listen() 
{
    int res = ::listen(socket_fd_, 5);
    if (res < 0) 
    {
        tools::rise_perror(SBuf() << "Can't start listening on socket for " << ip_address_ << ":" << ip_port_);
    }
}




std::unique_ptr<StreamSocketConnection> StreamServerSocket::accept()
{
    sockaddr_in client_addr;
    socklen_t cli_len = sizeof(client_addr);

    while(true) 
    {
        int fd = ::accept(socket_fd_, tools::ptr_cast<sockaddr>(&client_addr), &cli_len);

        if (fd >= 0) 
        {
            return std::make_unique<StreamSocketConnection>(fd, this->shared_from_this());
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


    
}}}
