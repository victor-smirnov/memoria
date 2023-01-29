
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

#include <memoria/core/hrpc/common.hpp>
#include <memoria/core/hrpc/schema.hpp>
#include <memoria/core/hrpc/exceptions.hpp>

#include <memoria/fiber/future.hpp>

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/tools/optional.hpp>

#include <functional>

namespace memoria::hrpc {

using RequestHandlerFn = std::function<Response (PoolSharedPtr<Context>)>;


class HRPCService {
public:
    virtual void add_handler(EndpointID endpoint_id, RequestHandlerFn handler) = 0;
    virtual void remove_handler(EndpointID endpoint_id) = 0;
    virtual Optional<RequestHandlerFn> get_handler(EndpointID endpoint_id) = 0;

    static PoolSharedPtr<HRPCService> make();
};


class ClientSocket {
public:    
    virtual ~ClientSocket() noexcept = default;
    virtual PoolSharedPtr<Connection> open() = 0;
};


class ServerSocket {
public:
    virtual ~ServerSocket() noexcept = default;
    virtual void listen() = 0;
    virtual PoolSharedPtr<Connection> accept() = 0;
};


class OutputChannel {
public:
    virtual ~OutputChannel() noexcept = default;

    virtual ChannelCode code() = 0;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual void push(const Message& msg) = 0;
    virtual void close() = 0;
    virtual bool is_closed() = 0;
};


class InputChannel {
public:
    virtual ~InputChannel() noexcept = default;

    virtual ChannelCode code() = 0;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual bool is_closed() = 0;
    virtual void close() = 0;

    virtual bool pop(Message& msg) = 0;
};




using CallCompletionFn = std::function<void (const Response&)>;


class HRPCCall {
public:
    virtual ~HRPCCall() noexcept = default;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual CallID call_id()  = 0;

    virtual Request request()   = 0;
    virtual Response response() = 0;

    virtual void wait() = 0;

    virtual void on_complete(CallCompletionFn fn) = 0;
    virtual void cancel() = 0;

    virtual size_t input_channels() = 0;
    virtual size_t output_channels() = 0;

    virtual PoolSharedPtr<InputChannel> input_channel(size_t idx)  = 0;
    virtual PoolSharedPtr<OutputChannel> output_channel(size_t idx) = 0;
};

using CancelCallListenerFn = std::function<void()>;

class Context {
public:
    virtual ~Context() noexcept = default;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual Request request() = 0;

    virtual hermes::Object get(NamedCode name) = 0;
    virtual void set(NamedCode name, hermes::Object object) = 0;

    virtual size_t input_channels() = 0;
    virtual size_t output_channels() = 0;

    virtual PoolSharedPtr<InputChannel> input_channel(size_t idx)  = 0;
    virtual PoolSharedPtr<OutputChannel> output_channel(size_t idx) = 0;

    virtual bool is_cancelled() = 0;

    virtual void set_cancel_listener(CancelCallListenerFn fn) = 0;
};




class Connection {
public:
    virtual ~Connection() noexcept = default;

    virtual PoolSharedPtr<HRPCService> service() = 0;

    virtual PoolSharedPtr<HRPCCall> call(
            Request request,
            CallCompletionFn completion_fn = CallCompletionFn{}
    ) = 0;

    virtual void close() = 0;

    virtual void handle_messages() = 0;
};


PoolSharedPtr<ClientSocket> make_tcp_client_socket(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<HRPCService>& service
);

PoolSharedPtr<ServerSocket> make_tcp_server_socket(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<HRPCService>& service
);

}
