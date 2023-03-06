
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

#include <memoria/asio/hrpc/tcp.hpp>
#include <memoria/asio/hrpc/session.hpp>
#include <memoria/asio/hrpc/hrpc.hpp>

#include <memoria/asio/yield.hpp>

namespace memoria::asio::hrpc {

PoolSharedPtr<st::Session> open_tcp_session(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<AsioHRPCSession>>();

    auto conn = ASIOSocketMessageProvider::make_instance(cfg);
    return pool->allocate_shared(endpoints, conn, cfg, SessionSide::CLIENT);
}

PoolSharedPtr<st::Server> make_tcp_server(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
) {
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<AsioServerSocket>>();

    return pool->allocate_shared(cfg, endpoints);
}


/*
PoolSharedPtr<st::MessageProvider> AsioTCPClientMessageProvider::
    make_instance(TCPClientSocketConfig config)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<AsioTCPClientMessageProvider>>();

//    auto socket = ss::engine().connect(ss::socket_address(
//        ss::ipv4_addr(config.host().to_std_string(), config.port())
//    )).get();

//    return pool->allocate_shared(std::move(socket));

    auto addr = reactor::IPAddress(config.host().data());
    return pool->allocate_shared(reactor::ClientSocket(addr, config.port()));
}








PoolSharedPtr<st::MessageProvider> ReactorTCPServerMessageProvider::
    make_instance(
        ReactorServerSocketPtr socket,
        reactor::SocketConnectionData&& conn_data
    )
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ReactorTCPServerMessageProvider>>();

    return pool->allocate_shared(socket, std::move(conn_data));
}

ReactorTCPClientMessageProvider::
ReactorTCPClientMessageProvider(reactor::ClientSocket socket):
    TCPMessageProviderBase(socket.input(), socket.output()),
    socket_(socket)
{
}


RawMessagePtr TCPMessageProviderBase::read_message()
{
    constexpr size_t basic_header_size = MessageHeader::basic_size();

    alignas(MessageHeader)
    uint8_t header_buf[basic_header_size]{0,};

    size_t sz1 = input_stream_.read_fully(header_buf, sizeof(header_buf), []{
        boost::this_fiber::yield();
    });

    if (sz1 < sizeof(header_buf)) {
        return RawMessagePtr{nullptr, ::free};
    }

    MessageHeader* header = ptr_cast<MessageHeader>(header_buf);
    auto buffer = allocate_system<uint8_t>(header->message_size());

    std::memcpy(buffer.get(), header_buf, basic_header_size);

    size_t msg_size = header->message_size() - basic_header_size;
    size_t sz2 = input_stream_.read_fully(
        buffer.get() + basic_header_size,
        msg_size
    );

    if (sz2 < msg_size) {
        return RawMessagePtr{nullptr, ::free};
    }

    return buffer;
}


void TCPMessageProviderBase::write_message(
        const MessageHeader& header,
        const uint8_t* data
) {
    size_t sz = output_stream_.write_fully(data, header.message_size());
    if (sz < header.message_size()) {
        MEMORIA_MAKE_GENERIC_ERROR("write_fully: stream closed").do_throw();
    }
}


AsioTCPServerMessageProvider::
AsioTCPServerMessageProvider(
        AsioServerSocketPtr socket,
        reactor::SocketConnectionData&& conn_data
):
    socket_(socket),
    connection_(reactor::ServerSocketConnection(std::move(conn_data)))
{
    input_stream_ = connection_.input();
    output_stream_ = connection_.output();
}

*/


PoolSharedPtr<st::Session> AsioServerSocket::new_session()
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<AsioHRPCSession>>();

    net::tcp::socket socket(io_context());
    boost::system::error_code ec;
    acceptor_.async_accept(socket, memoria::asio::yield[ec]);
    if (ec) {
        throw boost::system::system_error(ec); //some other error
    }

    auto conn = ASIOSocketMessageProvider::make_instance(std::move(socket));
    return pool->allocate_shared(endpoints_, conn, cfg_, SessionSide::SERVER);
}


bool ASIOSocketMessageProvider::read(uint8_t* buf, size_t size)
{
    size_t cnt = 0;
    while (cnt < size)
    {
        size_t to_read = size - cnt;

        boost::system::error_code ec;
        auto result = socket_.async_read_some(
            boost::asio::mutable_buffer(buf + cnt, to_read),
            memoria::asio::yield[ec]
        );

        if (ec == boost::asio::error::eof) {
            return false; //connection closed cleanly by peer
        }
        else if (ec) {
            if (ec.value() == 125) {
                return false;
            }
            else {
                throw boost::system::system_error(ec); //some other error
            }
        }

        if (MMA_LIKELY(result == to_read)) {
            return true;
        }
        else {
            cnt += result;
        }
    }

    return true;
}

void ASIOSocketMessageProvider::write(const uint8_t* buf, size_t size)
{
    size_t cnt = 0;
    while(cnt < size)
    {
        size_t to_write = size - cnt;

        boost::system::error_code ec;
        auto result = socket_.async_write_some(
            boost::asio::buffer(buf + cnt, to_write),
            memoria::asio::yield[ec]
        );

        if (ec == boost::asio::error::eof)
        {
            //connection closed cleanly by peer
            if (cnt + result < size) {
                MEMORIA_MAKE_GENERIC_ERROR("MessageProvider::write: socket closed").do_throw();
            }
            else {
                return;
            }
        }
        else if (ec) {
            println("Error writing to connection");
            throw boost::system::system_error(ec); //some other error
        }

        cnt += result;
    }
}


RawMessagePtr ASIOSocketMessageProvider::read_message()
{
    constexpr size_t basic_header_size = MessageHeader::basic_size();

    alignas(MessageHeader)
    uint8_t header_buf[basic_header_size]{0,};

    if (!read(header_buf, sizeof(header_buf))) {
        return RawMessagePtr(nullptr, ::free);
    }

    MessageHeader* header = ptr_cast<MessageHeader>(header_buf);
    auto buffer = allocate_system<uint8_t>(header->message_size());

    std::memcpy(buffer.get(), header_buf, basic_header_size);

    if (!read(
        buffer.get() + basic_header_size,
        header->message_size() - basic_header_size
    )) {
        return RawMessagePtr(nullptr, ::free);
    }

    return buffer;
}

void ASIOSocketMessageProvider::write_message(
        const MessageHeader& header,
        const uint8_t* data
) {
    write(data, header.message_size());
}


PoolSharedPtr<st::MessageProvider> ASIOSocketMessageProvider::make_instance(
    net::tcp::socket&& socket
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ASIOSocketMessageProvider>>();

    return pool->allocate_shared(std::move(socket));
}

PoolSharedPtr<st::MessageProvider> ASIOSocketMessageProvider::make_instance(
    const TCPClientSocketConfig& cfg
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ASIOSocketMessageProvider>>();

    net::tcp::socket socket(io_context());

    net::tcp::resolver resolver(io_context());
    net::tcp::resolver::query query(net::tcp::v4(), cfg.host().to_std_string(), std::to_string(cfg.port()));
    net::tcp::resolver::iterator iterator = resolver.resolve( query);
    net::tcp::socket s(io_context());

    boost::system::error_code ec;
    boost::asio::async_connect(socket, iterator, memoria::asio::yield[ec]);

    return pool->allocate_shared(std::move(socket));
}

}
