
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

#include "hrpc_impl_server.hpp"
#include "hrpc_impl_connection.hpp"

namespace memoria::hrpc {

PoolSharedPtr<ServerSocket> make_tcp_server_socket(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<HRPCService>& service
) {
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCServerSocketImpl>>();

    return pool->allocate_shared(cfg, service);
}



PoolSharedPtr<Connection> HRPCServerSocketImpl::accept()
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCConnectionImpl>>();

    reactor::SocketConnectionData conn_data = socket_.accept();

    return pool->allocate_shared(this->shared_from_this(), std::move(conn_data));
}

}
