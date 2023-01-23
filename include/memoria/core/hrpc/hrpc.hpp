
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
    virtual PoolSharedPtr<Connection> accept() = 0;
};


class OutputStream {
public:
    virtual ~OutputStream() noexcept = default;

    virtual StreamCode stream_code() = 0;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual void push(const StreamBatch& batch) = 0;
    virtual void close() = 0;
    virtual bool is_closed() = 0;
};


class InputStream {
public:
    virtual ~InputStream() noexcept = default;

    virtual StreamCode stream_code() = 0;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual bool is_closed() = 0;
    virtual void close() = 0;

    virtual void next() = 0;

    virtual StreamBatch batch() = 0;
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

    virtual size_t input_streams() = 0;
    virtual size_t output_streams() = 0;

    virtual PoolSharedPtr<InputStream> input_stream(size_t idx)  = 0;
    virtual PoolSharedPtr<OutputStream> output_stream(size_t idx) = 0;
};


class Context {
public:
    virtual ~Context() noexcept = 0;

    virtual PoolSharedPtr<Connection> connection() = 0;

    virtual Request request() = 0;

    virtual hermes::Object get(NamedCode name) = 0;
    virtual void set(NamedCode name, hermes::Object object) = 0;

    virtual size_t input_streams() = 0;
    virtual size_t output_streams() = 0;

    virtual PoolSharedPtr<InputStream> input_stream(size_t idx)  = 0;
    virtual PoolSharedPtr<OutputStream> output_stream(size_t idx) = 0;
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
    const TCPClinetSocketConfig& cfg,
    const PoolSharedPtr<HRPCService>& service
);

}
