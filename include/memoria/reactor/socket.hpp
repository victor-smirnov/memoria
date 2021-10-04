
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

#ifdef __linux__
#include "linux/linux_socket_impl.hpp"
#elif __APPLE__
#include "macosx/macosx_socket_impl.hpp"
#elif _WIN32
#include "msvc/msvc_socket_impl.hpp"
#else 
#error "Unsupported platform"
#endif 

#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/tools/iostreams.hpp>

#include <string>

namespace memoria {
namespace reactor {

class SocketImpl;
class ServerSocketImpl;
class ClientSocketImpl;
class ConnectionImpl;
class SocketConnectionImpl;
class DummySocketImpl;


class ServerSocketConnectionImpl;
class ClientSocketConnectionImpl;

class ServerSocketConnection;
class Socket;
class ServerSocket;
class ClientSocket;


class ServerSocket final : public PimplBase<ServerSocketImpl>  {
    using Base = PimplBase<ServerSocketImpl>;
public:
    ServerSocket(const IPAddress& ip_address, uint16_t ip_port);
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ServerSocket)
    
    void listen();
    int fd() const;
    void close();
    bool is_closed();

    uint16_t port() const;
    const IPAddress& address() const;
    
    SocketConnectionData accept();
};



class SocketConnection final: public PimplBase<SocketConnectionImpl> {
    using Base = PimplBase<SocketConnectionImpl>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(SocketConnection)
    
    Socket socket();
    
    BinaryInputStream input();
    BinaryOutputStream output();

    void close();
    bool is_closed();
    
    template <typename T> T cast_to() const;
};



class ServerSocketConnection final : public PimplBase<ServerSocketConnectionImpl> {
    using Base = PimplBase<ServerSocketConnectionImpl>;

public:
    using typename Base::PtrType;
    
    ServerSocketConnection(SocketConnectionData&& data);

    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ServerSocketConnection)
    
    int fd() const;

    BinaryInputStream  input();
    BinaryOutputStream output();
    
    void close();
    bool is_closed();
    
    operator SocketConnection() const;
    
    template <typename T> T cast_to() const;
};


class ClientSocket final : public PimplBase<ClientSocketImpl> {
    using Base = PimplBase<ClientSocketImpl>;
public:
    ClientSocket(const IPAddress& ip_address, uint16_t ip_port);
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(ClientSocket)

    BinaryInputStream  input();
    BinaryOutputStream output();

    void close();
    bool is_closed();
};

IPAddress parse_ipv4(U8StringView str);
IPAddress parse_ipv6(U8StringView str);

void InitSockets();
void DestroySockets();

}}
