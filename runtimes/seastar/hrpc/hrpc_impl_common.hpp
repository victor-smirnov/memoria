
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

#include <memoria/core/tools/iostreams.hpp>

#include <seastar/core/seastar.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/net/api.hh>

namespace memoria::hrpc {

namespace ss = seastar;

class HRPCClientSocketImpl;
using ClientSocketImplPtr = PoolSharedPtr<HRPCClientSocketImpl>;

class HRPCServerSocketImpl;
using ServerSocketImplPtr = PoolSharedPtr<HRPCServerSocketImpl>;

class HRPCEndpointRepositoryImpl;
using EndpointRepositoryImplPtr = PoolSharedPtr<EndpointRepository>;

class HRPCContextImpl;
using ContextImplPtr = PoolSharedPtr<HRPCContextImpl>;
using ContextImplWeakPtr = pool::WeakPtr<HRPCContextImpl>;


class HRPCSessionBase;
using SessionImplPtr = PoolSharedPtr<HRPCSessionBase>;
using SessionImplWeakPtr = pool::WeakPtr<HRPCSessionBase>;


class HRPCCallImpl;
using CallImplPtr = PoolSharedPtr<HRPCCallImpl>;
using CallImplWeakPtr = pool::WeakPtr<HRPCCallImpl>;


class HRPCInputChannelImpl;
using InputChannelImplPtr       = PoolSharedPtr<HRPCInputChannelImpl>;
using InputChannelImplWeakPtr   = pool::WeakPtr<HRPCInputChannelImpl>;


class HRPCOutputChannelImpl;
using OutputChannelImplPtr      = PoolSharedPtr<HRPCOutputChannelImpl>;
using OutputChannelImplWeakPtr  = pool::WeakPtr<HRPCOutputChannelImpl>;

using MessageProviderPtr     = PoolSharedPtr<MessageProvider>;
using MessageProviderWeakPtr = pool::WeakPtr<MessageProvider>;

}
