
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




#include "linux_socket.hpp"

namespace memoria {
namespace v1 {
namespace reactor {


ServerSocket::ServerSocket(const IPAddress& ip_address, uint16_t ip_port):
    Base(std::make_shared<ServerSocketImpl>(ip_address, ip_port))
{}


void ServerSocket::listen() {
    this->ptr_->listen();
}

SocketConnectionData ServerSocket::accept() {
    return this->ptr_->accept();
}

int ServerSocket::fd() const {
    return ptr_->fd();
}

void ServerSocket::close() {
    return ptr_->close();
}

bool ServerSocket::is_closed() {
    return ptr_->is_closed();
}

ServerSocketConnection::ServerSocketConnection(SocketConnectionData&& data):
    Base(std::make_shared<ServerSocketConnectionImpl>(std::move(data)))
{}


int ServerSocketConnection::fd() const {
    return ptr_->fd();
}

void ServerSocketConnection::close() {
    return ptr_->close();
}

bool ServerSocketConnection::is_closed() {
    return ptr_->is_closed();
}


BinaryInputStream ServerSocketConnection::input() {
    return this->ptr_->input();
}

BinaryOutputStream ServerSocketConnection::output() {
    return this->ptr_->output();
}



ServerSocketConnection::operator SocketConnection() const {
    return std::static_pointer_cast<SocketConnectionImpl>(ptr_);
}


//template <typename T>
//T ServerSocketConnection::cast_to() const;


ClientSocket::ClientSocket(const IPAddress& ip_address, uint16_t ip_port):
    Base(std::make_shared<ClientSocketImpl>(ip_address, ip_port))
{}



BinaryInputStream ClientSocket::input() {
    return this->ptr_->input();
}

BinaryOutputStream ClientSocket::output() {
    return this->ptr_->output();
}

void ClientSocket::close() {
    this->ptr_->close();
}

bool ClientSocket::is_closed() {
    return ptr_->is_closed();
}


void InitSockets() {}
void DestroySockets() {}


}}}
