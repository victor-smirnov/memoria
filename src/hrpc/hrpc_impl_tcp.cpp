
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

#include "hrpc_impl_tcp.hpp"
#include "hrpc_impl_session.hpp"


namespace memoria::hrpc {

PoolSharedPtr<Session> make_tcp_client_socket(
    const TCPClientSocketConfig& cfg,
    const PoolSharedPtr<Service>& service
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCSessionImpl>>();

    auto conn = TCPClientMessageProviderImpl::make_instance(cfg);
    return pool->allocate_shared(service, conn, cfg, SessionSide::CLIENT);
}


PoolSharedPtr<MessageProvider> TCPClientMessageProviderImpl::
    make_instance(TCPClientSocketConfig config)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<TCPClientMessageProviderImpl>>();

    auto addr = reactor::IPAddress(config.host().data());
    return pool->allocate_shared(reactor::ClientSocket(addr, config.port()));
}




PoolSharedPtr<ServerSocket> make_tcp_server_socket(
    const TCPServerSocketConfig& cfg,
    const PoolSharedPtr<Service>& service
) {
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCServerSocketImpl>>();

    return pool->allocate_shared(cfg, service);
}



PoolSharedPtr<Session> HRPCServerSocketImpl::accept()
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCSessionImpl>>();

    reactor::SocketConnectionData conn_data = socket_.accept();
    auto conn = TCPServerMessageProviderImpl::make_instance(this->shared_from_this(), std::move(conn_data));

    return pool->allocate_shared(service_, conn, cfg_, SessionSide::SERVER);
}


PoolSharedPtr<MessageProvider> TCPServerMessageProviderImpl::
    make_instance(
        ServerSocketImplPtr socket,
        reactor::SocketConnectionData&& conn_data
    )
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<TCPServerMessageProviderImpl>>();

    return pool->allocate_shared(socket, std::move(conn_data));
}

TCPClientMessageProviderImpl::
TCPClientMessageProviderImpl(reactor::ClientSocket socket):
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
        this_fiber::yield();
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

TCPServerMessageProviderImpl::
TCPServerMessageProviderImpl(
    ServerSocketImplPtr socket,
    reactor::SocketConnectionData&& conn_data
):
    socket_(socket),
    connection_(reactor::ServerSocketConnection(std::move(conn_data)))
{
    input_stream_ = connection_.input();
    output_stream_ = connection_.output();
}




bool ASIOSocketMessageProvider::read(uint8_t* buf, size_t size)
{
    size_t cnt = 0;
    while (cnt < size)
    {
        size_t to_read = size - cnt;
        auto result = socket_.read_some(
            boost::asio::mutable_buffer(buf + cnt, to_read)
        );

        if (MMA_LIKELY(result == to_read)) {
            return true;
        }
        else if (MMA_LIKELY(!socket_.is_open())) {
            return false;
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
        auto result = socket_.write_some(
            boost::asio::buffer(buf + cnt, to_write)
        );

        if (MMA_LIKELY(result == to_write)) {
            return;
        }
        else if (MMA_UNLIKELY(!socket_.is_open())) {
            MEMORIA_MAKE_GENERIC_ERROR("MessageProvider::write: socket closed").do_throw();
        }
        else {
            cnt += result;
        }
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


PoolSharedPtr<MessageProvider> ASIOSocketMessageProvider::make_instance(
    net::tcp::socket&& socket
)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<ASIOSocketMessageProvider>>();

    return pool->allocate_shared(std::move(socket));
}

}
