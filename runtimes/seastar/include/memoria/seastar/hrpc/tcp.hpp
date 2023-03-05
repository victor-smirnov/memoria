
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

#include <memoria/hrpc/hrpc_impl_common.hpp>

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/core/tools/optional.hpp>

#include <seastar/core/seastar.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/thread.hh>
#include <seastar/core/shared_mutex.hh>
#include <seastar/net/api.hh>

#include <boost/asio.hpp>

namespace memoria::hrpc::ss {

namespace net = boost::asio::ip;
namespace bsys = boost::system;
namespace ss = seastar;

//class SeastarClientSocket;
//using SeastarClientSocketPtr = PoolSharedPtr<SeastarClientSocket>;

class SeastarServerSocket;
using SeastarServerSocketPtr = PoolSharedPtr<SeastarServerSocket>;

class SeastarServerSocket final:
        public st::Server,
        public pool::enable_shared_from_this<SeastarServerSocket>
{
    TCPServerSocketConfig cfg_;
    PoolSharedPtr<st::EndpointRepository> endpoints_;

    seastar::server_socket socket_;

    static seastar::server_socket make_socket(U8String host, int16_t port)
    {
        seastar::listen_options opts;
        opts.reuse_address = true;
        return seastar::listen(
            seastar::socket_address(
                seastar::ipv4_addr(host.to_std_string(), port)
            ),
            opts
        );
    }

public:
    SeastarServerSocket(
            const TCPServerSocketConfig& cfg,
            PoolSharedPtr<st::EndpointRepository> endpoints
    ):
        cfg_(cfg), endpoints_(endpoints),
        socket_(make_socket(cfg_.host().data(), cfg_.port()))
    {
    }




    PoolSharedPtr<st::EndpointRepository> service() {
        return endpoints_;
    }

    const TCPServerSocketConfig& cfg() const {
        return cfg_;
    }

    void listen() override {}

    PoolSharedPtr<st::Session> new_session() override;
};





class TCPMessageProviderBase: public st::MessageProvider  {
protected:
    ss::input_stream<char> input_stream_;
    ss::output_stream<char> output_stream_;
    bool closed_{};
public:
    TCPMessageProviderBase(
        ss::input_stream<char> input_stream,
        ss::output_stream<char> output_stream
    ):
        input_stream_(std::move(input_stream)),
        output_stream_(std::move(output_stream))
    {
    }

    TCPMessageProviderBase() {}

    bool is_closed() override {
        return closed_;
    }

    bool needs_session_id() override {
        return false;
    }

    RawMessagePtr read_message() override;
    void write_message(const MessageHeader& header, const uint8_t* data) override;
};

/*
class ASIOSocketMessageProvider final: public st::MessageProvider  {
protected:
    net::tcp::socket socket_;
public:
    ASIOSocketMessageProvider(net::tcp::socket&& socket):
        socket_(std::move(socket))
    {}

    bool needs_session_id() override {
        return false;
    }

    bool read(uint8_t* buf, size_t size);
    void write(const uint8_t* buf, size_t size);

    RawMessagePtr read_message() override;
    void write_message(const MessageHeader& header, const uint8_t* data) override;

    void close() noexcept override
    {
        try {
            socket_.close();
        }
        catch (...) {
            println("Exception while closing ASIO socket");
        }
    }

    bool is_closed() override {
        return !socket_.is_open();
    }

    static PoolSharedPtr<st::MessageProvider> make_instance(
        net::tcp::socket&& socket
    );
};
*/



class SeastarTCPClientMessageProvider final: public TCPMessageProviderBase {
    seastar::connected_socket socket_;

public:
    SeastarTCPClientMessageProvider(seastar::connected_socket socket);

    void close() noexcept override {
        try {
            socket_.shutdown_input();
            socket_.shutdown_output();
            closed_ = true;
        }
        catch (...) {
            println("Exception closing TCP server connection");
        }
    }


    static PoolSharedPtr<st::MessageProvider> make_instance(TCPClientSocketConfig config);
};


class SeastarTCPServerMessageProvider final: public TCPMessageProviderBase {
    SeastarServerSocketPtr socket_;
    ss::accept_result connection_;

public:
    SeastarTCPServerMessageProvider(
        SeastarServerSocketPtr socket,
        ss::accept_result connection
    );

    void close() noexcept override {
        try {
            connection_.connection.shutdown_input();
            connection_.connection.shutdown_output();
            closed_ = true;
        }
        catch (...) {
            println("Exception closing TCP server connection");
        }
    }

    static PoolSharedPtr<MessageProvider> make_instance(
        SeastarServerSocketPtr socket,
        ss::accept_result connection
    );
};


}
