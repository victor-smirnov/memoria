
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

#include "hrpc_impl_client.hpp"
#include "hrpc_impl_connection.hpp"


namespace memoria::hrpc {

PoolSharedPtr<ClientSocket> make_tcp_client_socket(
    const TCPClinetSocketConfig& cfg,
    const PoolSharedPtr<HRPCService>& service
)
{
    static thread_local pool::SimpleObjectPool<HRPCClientSocketImpl> pool;
    return pool.allocate_shared(cfg, service);
}


PoolSharedPtr<Connection> HRPCClientSocketImpl::open()
{
    static thread_local pool::SimpleObjectPool<HRPCConnectionImpl> pool;
    return pool.allocate_shared(this->shared_from_this());
}

}
