
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

#include <memoria/reactor/hrpc/tcp.hpp>
#include <memoria/reactor/hrpc/session.hpp>
#include <memoria/reactor/hrpc/hrpc.hpp>


namespace memoria::reactor::hrpc {

PoolSharedPtr<st::Session> open_tcp_session(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ReactorHRPCSession>>();

    auto conn = ReactorTCPClientMessageProvider::make_instance(cfg);
    return pool->allocate_shared(endpoints, conn, cfg, SessionSide::CLIENT);
}


PoolSharedPtr<st::MessageProvider> ReactorTCPClientMessageProvider::
    make_instance(TCPClientSocketConfig config)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ReactorTCPClientMessageProvider>>();

//    auto socket = ss::engine().connect(ss::socket_address(
//        ss::ipv4_addr(config.host().to_std_string(), config.port())
//    )).get();

//    return pool->allocate_shared(std::move(socket));

    auto addr = reactor::IPAddress(config.host().data());
    return pool->allocate_shared(reactor::ClientSocket(addr, config.port()));
}


PoolSharedPtr<st::Server> make_tcp_server(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<st::EndpointRepository>& endpoints
) {
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ReactorServerSocket>>();

    return pool->allocate_shared(cfg, endpoints);
}



PoolSharedPtr<st::Session> ReactorServerSocket::new_session()
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ReactorHRPCSession>>();

    reactor::SocketConnectionData conn_data = socket_.accept();
    auto conn = ReactorTCPServerMessageProvider::make_instance(this->shared_from_this(), std::move(conn_data));

    return pool->allocate_shared(endpoints_, conn, cfg_, SessionSide::SERVER);
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


ReactorTCPServerMessageProvider::
ReactorTCPServerMessageProvider(
        ReactorServerSocketPtr socket,
        reactor::SocketConnectionData&& conn_data
):
    socket_(socket),
    connection_(reactor::ServerSocketConnection(std::move(conn_data)))
{
    input_stream_ = connection_.input();
    output_stream_ = connection_.output();
}

}
