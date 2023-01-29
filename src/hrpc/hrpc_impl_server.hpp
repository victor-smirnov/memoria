
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

#include "hrpc_impl_common.hpp"

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/reactor/socket.hpp>

namespace memoria::hrpc {

class HRPCServerSocketImpl final:
        public ServerSocket,
        public pool::enable_shared_from_this<HRPCServerSocketImpl>
{
    TCPServerSocketConfig cfg_;
    PoolSharedPtr<HRPCService> service_;

    reactor::ServerSocket socket_;

public:
    HRPCServerSocketImpl(
            const TCPServerSocketConfig& cfg,
            PoolSharedPtr<HRPCService> service
    ):
        cfg_(cfg), service_(service),
        socket_(reactor::IPAddress(cfg_.host().data()), cfg_.port())
    {
    }

    PoolSharedPtr<HRPCService> service() {
        return service_;
    }

    const TCPServerSocketConfig& cfg() const {
        return cfg_;
    }

    void listen() {
        socket_.listen();
    }

    PoolSharedPtr<Connection> accept();
};


class TCPServerSocketStreamsProviderImpl final: public StreamsProvider {
    ServerSocketImplPtr socket_;
    reactor::ServerSocketConnection connection_;
public:
    TCPServerSocketStreamsProviderImpl(
        ServerSocketImplPtr socket,
        reactor::SocketConnectionData&& conn_data
    );

    BinaryInputStream input_stream() override {
        return connection_.input();
    }

    BinaryOutputStream output_stream() override {
        return connection_.output();
    }

    void close() override {
        connection_.close();
    }

    ProtocolConfig config() override {
        return socket_->cfg();
    }

    static PoolSharedPtr<StreamsProvider> make_instance(
        ServerSocketImplPtr socket,
        reactor::SocketConnectionData&& conn_data
    );
};



}
