
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

#include <memoria/core/hrpc/hrpc.hpp>

#include <memoria/fiber/fiber.hpp>
#include <memoria/fiber/condition_variable.hpp>

#include <memoria/core/tools/iostreams.hpp>

namespace memoria::hrpc {

class HRPCClientSocketImpl;
using ClientSocketImplPtr = PoolSharedPtr<HRPCClientSocketImpl>;

class HRPCServerSocketImpl;
using ServerSocketImplPtr = PoolSharedPtr<HRPCServerSocketImpl>;

class HRPCServiceImpl;
using ServiceImplPtr = PoolSharedPtr<HRPCService>;

class HRPCContextImpl;
using ContextImplPtr = PoolSharedPtr<HRPCContextImpl>;
using ContextImplWeakPtr = pool::WeakPtr<HRPCContextImpl>;


class HRPCConnectionImpl;
using ConnectionImplPtr = PoolSharedPtr<HRPCConnectionImpl>;
using ConnectionImplWeakPtr = pool::WeakPtr<HRPCConnectionImpl>;


class HRPCCallImpl;
using CallImplPtr = PoolSharedPtr<HRPCCallImpl>;
using CallImplWeakPtr = pool::WeakPtr<HRPCCallImpl>;


class HRPCInputChannelImpl;
using InputChannelImplPtr = PoolSharedPtr<HRPCInputChannelImpl>;
using InputChannelImplWeakPtr = pool::WeakPtr<HRPCInputChannelImpl>;


class HRPCOutputChannelImpl;
using OutputChannelImplPtr = PoolSharedPtr<HRPCOutputChannelImpl>;
using OutputChannelImplWeakPtr = pool::WeakPtr<HRPCOutputChannelImpl>;

class StreamsProvider {
public:
    virtual ~StreamsProvider() = default;

    virtual BinaryInputStream input_stream() = 0;
    virtual BinaryOutputStream output_stream() = 0;
    virtual void close() = 0;
    virtual ProtocolConfig config() = 0;
};

class ConnectionProvider {
public:
    virtual ~ConnectionProvider() = default;

    virtual PoolSharedPtr<StreamsProvider> new_connection() = 0;
    virtual ProtocolConfig config() = 0;
};



}
