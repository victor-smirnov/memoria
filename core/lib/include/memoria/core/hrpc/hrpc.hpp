
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/tools/optional.hpp>

#include <boost/asio.hpp>

#include <functional>

namespace memoria::hrpc {

using RequestHandlerFn = std::function<Response (PoolSharedPtr<Context>)>;


class EndpointRepository {
public:
    virtual void add_handler(const EndpointID& endpoint_id, RequestHandlerFn handler) = 0;
    virtual void remove_handler(const EndpointID& endpoint_id) = 0;
    virtual Optional<RequestHandlerFn> get_handler(const EndpointID& endpoint_id) = 0;

    static PoolSharedPtr<EndpointRepository> make();
};


class Server {
public:
    virtual ~Server() noexcept = default;
    virtual void listen() = 0;
    virtual PoolSharedPtr<Session> new_session() = 0;
};

class Client {
public:
    virtual ~Client() noexcept = default;
    virtual PoolSharedPtr<Session> open_session() = 0;
};

class OutputChannel {
public:
    virtual ~OutputChannel() noexcept = default;

    virtual ChannelCode code() = 0;

    virtual PoolSharedPtr<Session> session() = 0;

    virtual void push(const Message& msg) = 0;
    virtual void close() = 0;
    virtual bool is_closed() = 0;
};


class InputChannel {
public:
    virtual ~InputChannel() noexcept = default;

    virtual ChannelCode code() = 0;

    virtual PoolSharedPtr<Session> session() = 0;

    virtual bool is_closed() = 0;
    virtual void close() = 0;

    virtual bool pop(Message& msg) = 0;
};




using CallCompletionFn = std::function<void (const Response&)>;


class Call {
public:
    virtual ~Call() noexcept = default;

    virtual PoolSharedPtr<Session> session() = 0;

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

    virtual PoolSharedPtr<Session> session() = 0;

    virtual EndpointID endpoint_id() = 0;

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




class Session {
public:
    virtual ~Session() noexcept = default;

    virtual PoolSharedPtr<EndpointRepository> endpoints() = 0;

    virtual PoolSharedPtr<Call> call(
            const EndpointID& endpoint_id,
            Request request,
            Optional<ShardID> shard_id,
            CallCompletionFn completion_fn
    ) = 0;

    PoolSharedPtr<Call> call(
            const EndpointID& endpoint_id,
            Request request
    ) {
        return call(endpoint_id, request, Optional<ShardID>{}, CallCompletionFn{});
    }

    PoolSharedPtr<Call> call(
            const EndpointID& endpoint_id,
            Request request,
            CallCompletionFn completion_fn
    ) {
        return call(endpoint_id, request, Optional<ShardID>{}, completion_fn);
    }

    PoolSharedPtr<Call> call(
            const EndpointID& endpoint_id,
            Request request,
            Optional<ShardID> shard_id
    ) {
        return call(endpoint_id, request, shard_id, CallCompletionFn{});
    }

    virtual void close() = 0;
    virtual bool is_closed() = 0;

    virtual void handle_messages() = 0;
};


class MessageProvider {
public:
    virtual ~MessageProvider() = default;

    virtual bool needs_session_id() = 0;

    virtual RawMessagePtr read_message() = 0;
    virtual void write_message(const MessageHeader& header, const uint8_t* data) = 0;

    virtual void close() noexcept = 0;
    virtual bool is_closed() = 0;
};


PoolSharedPtr<Session> open_tcp_session(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<EndpointRepository>& endpoints
);

PoolSharedPtr<Server> make_tcp_server(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<EndpointRepository>& endpoints
);

}
