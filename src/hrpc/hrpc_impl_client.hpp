
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

class HRPCClientSocketImpl final:
        public ClientSocket,
        public pool::enable_shared_from_this<HRPCClientSocketImpl>
{
    TCPClientSocketConfig cfg_;
    PoolSharedPtr<HRPCService> service_;

public:
    HRPCClientSocketImpl(
            const TCPClientSocketConfig& cfg,
            PoolSharedPtr<HRPCService> service
    ):
        cfg_(cfg), service_(service)
    {
    }

    PoolSharedPtr<HRPCService> service() {
        return service_;
    }

    const TCPClientSocketConfig& cfg() const {
        return cfg_;
    }

    PoolSharedPtr<Connection> open();
};


class TCPClientSocketStreamsProviderImpl final: public StreamsProvider {
    TCPClientSocketConfig config_;
    reactor::ClientSocket socket_;
public:
    TCPClientSocketStreamsProviderImpl(TCPClientSocketConfig config);

    BinaryInputStream input_stream() override {
        return socket_.input();
    }

    BinaryOutputStream output_stream() override {
        return socket_.output();
    }

    void close() override {
        socket_.close();
    }

    ProtocolConfig config() override {
        return config_;
    }

    static PoolSharedPtr<StreamsProvider> make_instance(TCPClientSocketConfig config);
};


}
