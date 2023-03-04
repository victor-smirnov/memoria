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

#pragma once

#include <memoria/core/tools/bzero_struct.hpp>
#include <memoria/reactor/message/fiber_io_message.hpp>
#include <memoria/reactor/macosx/macosx_socket_impl.hpp>
#include <memoria/reactor/macosx/macosx_socket_impl.hpp>

#include <memoria/reactor/socket.hpp>

#include <memoria/reactor/reactor.hpp>

#include "macosx_io_messages.hpp"

#include <memory>

namespace memoria {
namespace reactor {    
    
class SocketImpl {
protected:
    int fd_;
    bool closed_{false};
public:
    SocketImpl(int fd): fd_(fd) {}
    virtual ~SocketImpl() noexcept {}

    int fd() const {return fd_;}

    virtual void close() = 0;
    virtual bool is_closed() const noexcept {return closed_;}

    virtual const IPAddress& address() const = 0;
    virtual uint16_t port() const = 0;
};

class ServerSocketImpl: public SocketImpl {
protected:
    IPAddress ip_address_;
    uint16_t  ip_port_;

    sockaddr_in sock_address_;

    SocketIOMessage fiber_io_message_;
public:
    ServerSocketImpl(const IPAddress& ip_address, uint16_t ip_port);
    virtual ~ServerSocketImpl() noexcept;

    void listen();
    SocketConnectionData accept();

    virtual const IPAddress& address() const {return ip_address_;}
    virtual uint16_t port() const {return ip_port_;}

    virtual void close();
};


class ConnectionImpl {
protected:
    int fd_;
    bool op_closed_{false};

public:
    ConnectionImpl(int fd): fd_(fd) {}
    virtual ~ConnectionImpl() noexcept {}

    int fd() const {return fd_;}
    virtual void close() = 0;
    virtual bool is_closed() const = 0;

    virtual BinaryInputStream input()   = 0;
    virtual BinaryOutputStream output() = 0;
};

class SocketConnectionImpl: public ConnectionImpl {
public:
    SocketConnectionImpl(int fd): ConnectionImpl(fd) {}

    virtual BinaryInputStream input()   = 0;
    virtual BinaryOutputStream output() = 0;
};

class ServerSocketConnectionImpl:
        public SocketConnectionImpl,
        public IBinaryInputStream,
        public IBinaryOutputStream,
        public EnableSharedFromThis<ServerSocketConnectionImpl>
{
    IPAddress ip_address_;
    uint16_t ip_port_;
    bool data_closed_{false};

    SocketIOMessage fiber_io_read_message_;
    SocketIOMessage fiber_io_write_message_;
public:
    ServerSocketConnectionImpl(SocketConnectionData&& data);
    virtual ~ServerSocketConnectionImpl() noexcept;


    virtual BinaryInputStream input() {
        return BinaryInputStream(StaticPointerCast<IBinaryInputStream>(shared_from_this()));
    }

    virtual BinaryOutputStream output() {
        return BinaryOutputStream(StaticPointerCast<IBinaryOutputStream>(shared_from_this()));
    }

    virtual size_t read(uint8_t* data, size_t size);
    virtual size_t write(const uint8_t* data, size_t size) {
        size_t tt{};
        while (tt < size) {
            size_t written = write_(data + tt, size - tt);
            if (written > 0) {
                tt += written;
            }
            else {
                break;
            }
        }
        return tt;
    }

    size_t write_(const uint8_t* data, size_t size);



    virtual void flush() {}

    virtual void close();
    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
                //|| fiber_io_read_message_.connection_closed()
                //|| fiber_io_write_message_.connection_closed();
    }
};


class ClientSocketConnectionImpl:
        public SocketConnectionImpl,
        public IBinaryInputStream,
        public IBinaryOutputStream,
        public EnableSharedFromThis<ClientSocketImpl>
{
public:
    ClientSocketConnectionImpl(int fd): SocketConnectionImpl(fd) {}

    virtual BinaryInputStream input() {
        return BinaryInputStream(StaticPointerCast<IBinaryInputStream>(shared_from_this()));
    }
    virtual BinaryOutputStream output() {
        return BinaryOutputStream(StaticPointerCast<IBinaryOutputStream>(shared_from_this()));
    }
};

class ClientSocketImpl: public ClientSocketConnectionImpl {
    using Base = ClientSocketConnectionImpl;

    IPAddress ip_address_;
    uint16_t ip_port_;

    sockaddr_in sock_address_;

    SocketIOMessage fiber_io_read_message_;
    SocketIOMessage fiber_io_write_message_;

    bool data_closed_{};

public:
     ClientSocketImpl(const IPAddress& ip_address, uint16_t ip_port);
     virtual ~ClientSocketImpl() noexcept;

     virtual const IPAddress& address() const {return ip_address_;}
     virtual uint16_t port() const {return ip_port_;}

     virtual size_t read(uint8_t* data, size_t size);
     virtual size_t write(const uint8_t* data, size_t size) {
         size_t tt{};
         while (tt < size) {
             size_t written = write_(data + tt, size - tt);
             if (written > 0) {
                 tt += written;
             }
             else {
                 break;
             }
         }
         return tt;
     }

     size_t write_(const uint8_t* data, size_t size);

     virtual void flush() {}

     virtual void close();
     virtual bool is_closed() const {
         return op_closed_ || data_closed_;
                 //|| fiber_io_read_message_.connection_closed()
                 //|| fiber_io_write_message_.connection_closed();
     }

private:
     void connect();
};

void InitSockets();
void DestroySockets();



}}
