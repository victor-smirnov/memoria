
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

#include <memoria/seastar/hrpc/tcp.hpp>
#include <memoria/seastar/hrpc/session.hpp>
#include <memoria/seastar/hrpc/hrpc.hpp>


namespace memoria::seastar::hrpc {

PoolSharedPtr<st::Session> open_tcp_session(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarHRPCSession>>();

    auto conn = SeastarTCPClientMessageProvider::make_instance(cfg);
    return pool->allocate_shared(endpoints, conn, cfg, SessionSide::CLIENT);
}


PoolSharedPtr<st::MessageProvider> SeastarTCPClientMessageProvider::
    make_instance(TCPClientSocketConfig config)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarTCPClientMessageProvider>>();

    auto socket = ss::engine().connect(ss::socket_address(
        ss::ipv4_addr(config.host().to_std_string(), config.port())
    )).get();

    return pool->allocate_shared(std::move(socket));
}


PoolSharedPtr<st::Server> make_tcp_server(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
) {
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarServerSocket>>();

    return pool->allocate_shared(cfg, endpoints);
}



PoolSharedPtr<st::Session> SeastarServerSocket::new_session()
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarHRPCSession>>();

    auto conn_data = socket_.accept().get();
    auto conn = SeastarTCPServerMessageProvider::make_instance(this->shared_from_this(), std::move(conn_data));

    return pool->allocate_shared(endpoints_, conn, cfg_, SessionSide::SERVER);
}


PoolSharedPtr<st::MessageProvider> SeastarTCPServerMessageProvider::
    make_instance(
        SeastarServerSocketPtr socket,
        ss::accept_result conn_data
    )
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarTCPServerMessageProvider>>();

    return pool->allocate_shared(socket, std::move(conn_data));
}

SeastarTCPClientMessageProvider::
SeastarTCPClientMessageProvider(ss::connected_socket socket):
    TCPMessageProviderBase(socket.input(), socket.output()),
    socket_(std::move(socket))
{
}


RawMessagePtr TCPMessageProviderBase::read_message()
{
    constexpr size_t basic_header_size = MessageHeader::basic_size();

    auto header_buf = input_stream_.read_exactly(basic_header_size).get();

    if (header_buf.size() < basic_header_size) {
        return RawMessagePtr{nullptr, ::free};
    }

    MessageHeader* header = ptr_cast<MessageHeader>(header_buf.get());
    auto buffer = allocate_system<uint8_t>(header->message_size());

    std::memcpy(buffer.get(), header_buf.get(), basic_header_size);

    size_t msg_size = header->message_size() - basic_header_size;
    auto data_buf = input_stream_.read_exactly(msg_size).get();

    std::memcpy(buffer.get() + basic_header_size, data_buf.get(), msg_size);

    if (data_buf.size() < msg_size) {
        return RawMessagePtr{nullptr, ::free};
    }

    return buffer;
}


void TCPMessageProviderBase::write_message(
        const MessageHeader& header,
        const uint8_t* data
) {
    output_stream_.write(ptr_cast<char>(data), header.message_size()).get();
    output_stream_.flush().get();
}


SeastarTCPServerMessageProvider::
SeastarTCPServerMessageProvider(
    SeastarServerSocketPtr socket,
    ss::accept_result conn_data
):
    socket_(socket),
    connection_(std::move(conn_data))
{
    input_stream_ = connection_.connection.input();
    output_stream_ = connection_.connection.output();
}

}
