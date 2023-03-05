
// Copyright 2023 Victor Smirnov
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

#include <memoria/hrpc/hrpc_impl_common.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/reactor/socket.hpp>

namespace memoria::reactor::hrpc {

using namespace memoria::hrpc;

class ReactorServerSocket;
using ReactorServerSocketPtr = PoolSharedPtr<ReactorServerSocket>;

class ReactorServerSocket final:
        public st::Server,
        public pool::enable_shared_from_this<ReactorServerSocket>
{
    TCPServerSocketConfig cfg_;
    PoolSharedPtr<st::EndpointRepository> endpoints_;

    reactor::ServerSocket socket_;

public:
    ReactorServerSocket(
            const TCPServerSocketConfig& cfg,
            PoolSharedPtr<st::EndpointRepository> endpoints
    ):
        cfg_(cfg), endpoints_(endpoints),
        socket_(reactor::IPAddress(cfg_.host().data()), cfg_.port())
    {
    }

    PoolSharedPtr<st::EndpointRepository> service() {
        return endpoints_;
    }

    const TCPServerSocketConfig& cfg() const {
        return cfg_;
    }

    void listen() override {
        socket_.listen();
    }

    PoolSharedPtr<st::Session> new_session() override;
};





class TCPMessageProviderBase: public st::MessageProvider  {
protected:
    BinaryInputStream input_stream_;
    BinaryOutputStream output_stream_;
public:
    TCPMessageProviderBase(
        BinaryInputStream input_stream,
        BinaryOutputStream output_stream
    ):
        input_stream_(input_stream),
        output_stream_(output_stream)
    {}

    TCPMessageProviderBase() {}

    bool needs_session_id() override {
        return false;
    }

    RawMessagePtr read_message() override;
    void write_message(const MessageHeader& header, const uint8_t* data) override;
};



class ReactorTCPClientMessageProvider final: public TCPMessageProviderBase {
    reactor::ClientSocket socket_;

public:
    ReactorTCPClientMessageProvider(reactor::ClientSocket socket);

    void close() noexcept override
    {
        try {
            socket_.close();
        }
        catch (...) {
            println("Exception closing TCP client socket");
        }
    }


    bool is_closed() override {
        return socket_.is_closed();
    }

    static PoolSharedPtr<MessageProvider> make_instance(TCPClientSocketConfig config);

};


class ReactorTCPServerMessageProvider final: public TCPMessageProviderBase {
    ReactorServerSocketPtr socket_;
    reactor::ServerSocketConnection connection_;

public:
    ReactorTCPServerMessageProvider(
        ReactorServerSocketPtr socket,
        reactor::SocketConnectionData&& conn_data
    );

    void close() noexcept override {
        try {
            connection_.close();
        }
        catch (...) {
            println("Exception closing TCP server connection");
        }
    }

    bool is_closed() override {
        return connection_.is_closed();
    }

    static PoolSharedPtr<MessageProvider> make_instance(
        ReactorServerSocketPtr socket,
        reactor::SocketConnectionData&& conn_data
    );
};


}
