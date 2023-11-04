
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

#include <memoria/hrpc/hrpc.hpp>

namespace memoria::reactor::hrpc {

using namespace memoria::hrpc;

PoolSharedPtr<st::Session> open_tcp_session(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
);

PoolSharedPtr<st::Server> make_tcp_server(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
);


}