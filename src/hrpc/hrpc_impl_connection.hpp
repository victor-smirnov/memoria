
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

#include <memoria/core/flat_map/flat_hash_map.hpp>
#include <memoria/reactor/socket.hpp>
#include <memoria/reactor/reactor.hpp>

#include <memoria/fiber/fiber.hpp>

#include "hrpc_impl_client.hpp"
#include "hrpc_impl_server.hpp"
#include "hrpc_impl_context.hpp"
#include "hrpc_impl_call.hpp"
#include "hrpc_impl_input_channel.hpp"
#include "hrpc_impl_output_channel.hpp"

namespace memoria::hrpc {

class HRPCConnectionImpl final:
        public Connection,
        public pool::enable_shared_from_this<HRPCConnectionImpl>
{
    PoolSharedPtr<HRPCService> service_;
    PoolSharedPtr<StreamsProvider> streams_;

    BinaryInputStream  input_stream_;
    BinaryOutputStream output_stream_;

    ska::flat_hash_map<CallID, CallImplWeakPtr> calls_;
    ska::flat_hash_map<CallID, ContextImplPtr> contexts_;

    bool closed_{false};
    ConnectionSide connection_side_;

    CallID call_id_cnt_;

    uint64_t channel_buffer_size_; // 1MB

public:
    HRPCConnectionImpl(
        PoolSharedPtr<HRPCService> service,
        PoolSharedPtr<StreamsProvider> streams,
        ConnectionSide connection_side
    ):
        service_(service),
        streams_(streams),
        connection_side_(connection_side),
        channel_buffer_size_(streams->config().channel_buffer_size())
    {
        input_stream_  = streams->input_stream();
        output_stream_ = streams->output_stream();

        if (connection_side_ == ConnectionSide::CLIENT)
        {
            call_id_cnt_ = 0;
            send_connection_metadata();
        }
        else {
            call_id_cnt_ = 1;
        }
    }

    uint64_t channel_buffer_size() const {
        return channel_buffer_size_;
    }

    PoolSharedPtr<HRPCService> service() override {
        return service_;
    }

    PoolSharedPtr<HRPCCall> call(
            Request request,
            CallCompletionFn completion_fn = CallCompletionFn{}
    ) override {
        static thread_local auto pool = boost::make_local_shared<
            pool::SimpleObjectPool<HRPCCallImpl>
        >();

        CallID call_id = next_call_id();
        send_message(MessageType::CALL, call_id, request.object().ctr());

        auto call = pool->allocate_shared(shared_from_this(), call_id, request, completion_fn);

        calls_[call_id] = CallImplWeakPtr(call);

        return call;
    }

    bool read_header(MessageHeader& header)
    {
        uint8_t* buf = ptr_cast<uint8_t>(&header);
        size_t size = sizeof(MessageHeader);

        size_t cnt = 0;
        while (cnt < size) {
            size_t rr = input_stream_.read(buf + cnt, size - cnt);
            if (cnt == 0 && rr == 0) {
                return false;
            }

            cnt += rr;
        }

        return true;
    }

    virtual void handle_messages() override
    {
        try {
            while (!closed_)
            {
                MessageHeader header;
                if (!read_header(header)) {
                    this_fiber::yield();
                    continue;
                }

                switch (header.message_type()) {
                case MessageType::CONNECTION: {
                    if (connection_side_ == ConnectionSide::SERVER) {
                        prepare_server_connection(header);
                        send_connection_metadata();
                    }
                    else {
                        prepare_client_connection(header);
                    }
                    break;
                }
                case MessageType::CALL: {
                    invoke_handler(header);
                    break;
                }
                case MessageType::CALL_CHANNEL_MESSAGE: {
                    handle_call_side_stream_message(header);
                    break;
                }
                case MessageType::CONTEXT_CHANNEL_MESSAGE: {
                    handle_context_side_stream_message(header);
                    break;
                }
                case MessageType::CALL_CLOSE_INPUT_CHANNEL: {
                    handle_call_close_stream(header, true);
                    break;
                }
                case MessageType::CONTEXT_CLOSE_INPUT_CHANNEL: {
                    handle_context_close_stream(header, true);
                    break;
                }
                case MessageType::CALL_CLOSE_OUTPUT_CHANNEL: {
                    handle_call_close_stream(header, false);
                    break;
                }
                case MessageType::CONTEXT_CLOSE_OUTPUT_CHANNEL: {
                    handle_context_close_stream(header, false);
                    break;
                }
                case MessageType::CANCEL_CALL: {
                    handle_cancel_call(header);
                    break;
                }
                case MessageType::RETURN: {
                    handle_return(header);
                    break;
                }
                case MessageType::CALL_CHANNEL_BUFFER_RESET: {
                    handle_call_stream_buffer_reset(header);
                    break;
                }
                case MessageType::CONTEXT_CHANNEL_BUFFER_RESET: {
                    handle_context_stream_buffer_reset(header);
                    break;
                }
                }
            }
        }
        catch (...) {
            try {
                do_close_connection();
            }
            catch (...) {
                println("Can't close HRPC connection");
            }

            do_cleanup_connection();
            throw;
        }
    }

    void close() override {
        closed_ = true;
    }

    size_t send_message(
            MessageType type,
            CallID call_id,
            const hermes::HermesCtr& msg,
            ChannelCode code = 0
    ){
        MessageHeader header;
        auto ctr = msg.compactify(true);

        header.set_message_type(type);
        header.set_call_id(call_id);
        header.set_channel_code(code);

        auto data = ctr.span();
        header.set_message_size(data.size());

        output_stream_.write(ptr_cast<uint8_t>(&header), sizeof(header));
        output_stream_.write(data.data(), data.size());

        return data.size();
    }


    void send_message(
            MessageType type,
            CallID call_id,
            ChannelCode stream_code = 0
    ){
        MessageHeader header;
        header.set_message_type(type);
        header.set_call_id(call_id);
        header.set_channel_code(stream_code);

        output_stream_.write(ptr_cast<uint8_t>(&header), sizeof(header));
    }


    CallID next_call_id()
    {
        call_id_cnt_ += 2;
        return call_id_cnt_;
    }


    void unblock_output_channel(CallID call_id, ChannelCode code, bool call_side)
    {
        MessageHeader header;
        header.set_call_id(call_id);
        header.set_channel_code(code);
        if (call_side) {
            header.set_message_type(MessageType::CALL_CHANNEL_BUFFER_RESET);
        }
        else {
            header.set_message_type(MessageType::CONTEXT_CHANNEL_BUFFER_RESET);
        }

        output_stream_.write(ptr_cast<uint8_t>(&header), sizeof(header));
    }


private:
    void do_cleanup_connection()
    {
        if (!closed_) {
            for (auto& call: calls_) {
                auto ptr = call.second.lock();
                if (ptr) {
                    ptr->close_channels();
                }
            }

            for (auto& ctx: contexts_) {
                ctx.second->cancel_call();
            }
        }
    }

    void handle_context_side_stream_message(const MessageHeader& header)
    {
        auto msg = read_message(header.message_size());

        auto ii = calls_.find(header.call_id());
        if (ii != calls_.end())
        {
            auto ptr = ii->second.lock();
            if (!ptr.is_null()) {
                Message rq(msg.root().as_tiny_object_map());
                ptr->new_message(std::move(rq), header.channel_code());
            }
            else {
                calls_.erase(header.call_id());
            }
        }
    }

    void handle_call_side_stream_message(const MessageHeader& header)
    {
        auto msg = read_message(header.message_size());
        auto ctx = context(header.call_id());
        if (!ctx.is_null()) {
            Message rq(msg.root().as_tiny_object_map());
            ctx->new_message(std::move(rq), header.channel_code());
        }
    }

    void handle_call_close_stream(const MessageHeader& header, bool input)
    {
        auto ctx = context(header.call_id());
        if (!ctx.is_null()) {
            ctx->close_channel(input, header.channel_code());
        }
    }

    void handle_context_close_stream(const MessageHeader& header, bool input)
    {
        auto ii = calls_.find(header.call_id());
        if (ii != calls_.end()) {
            auto ptr = ii->second.lock();
            if (!ptr.is_null()) {
                ptr->close_channel(input, header.channel_code());
            }
        }
    }

    void handle_context_stream_buffer_reset(const MessageHeader& header)
    {
        auto ii = calls_.find(header.call_id());
        if (ii != calls_.end()) {
            auto ptr = ii->second.lock();
            if (!ptr.is_null()) {
                ptr->reset_output_channel_buffer(header.channel_code());
            }
        }
    }

    void handle_call_stream_buffer_reset(const MessageHeader& header)
    {
        auto ii = contexts_.find(header.call_id());
        if (ii != contexts_.end()) {
            ii->second->reset_output_channel_buffer(header.channel_code());
        }
    }

    void handle_cancel_call(const MessageHeader& header)
    {
        auto ii = contexts_.find(header.call_id());
        if (ii != contexts_.end()) {
            ii->second->cancel_call();
        }
    }



    void send_connection_metadata()
    {
        ConnectionMetadata meta(hermes::HermesCtr::make_pooled());
        meta.set_channel_buffer_size(channel_buffer_size_);

        auto meta_imm = meta.compactify();

        MessageHeader header;
        header.set_message_type(MessageType::CONNECTION);
        header.set_call_id(PROTOCOL_VERSION.value());

        auto data = meta_imm.object().ctr().span();
        header.set_message_size(data.size());
        output_stream_.write(ptr_cast<uint8_t>(&header), sizeof(header));
        output_stream_.write(data.data(), data.size());
    }

    void handle_return(const MessageHeader& header);

    hermes::HermesCtr read_message(size_t size)
    {
        auto buf = allocate_system<uint8_t>(size);

        size_t cnt = 0;
        while (cnt < size) {
            auto rr = input_stream_.read(buf.get() + cnt, size - cnt);
            cnt += rr;
        }

        return hermes::HermesCtr::from_buffer(std::move(buf), size);
    }

    ContextImplPtr context(CallID id)
    {
        auto ii = contexts_.find(id);
        if (ii != contexts_.end()) {
            return ii->second;
        }
        return {};
    }

    ContextImplPtr make_context(CallID call_id, Request rq)
    {
        static thread_local auto pool
            = boost::make_local_shared<
                pool::SimpleObjectPool<HRPCContextImpl>
            >();

        return pool->allocate_shared(this->shared_from_this(), call_id, rq);
    }

    void prepare_server_connection(const MessageHeader& header)
    {
        ProtocolVersion version(header.call_id());
        if (version != PROTOCOL_VERSION) {
            do_close_connection();
        }

        auto msg = read_message(header.message_size());
        ConnectionMetadata meta(msg.root().as_tiny_object_map());

        auto buf_size = meta.channel_buffer_size();
        if (buf_size < channel_buffer_size_) {
            channel_buffer_size_ = buf_size;
        }
    }

    void prepare_client_connection(const MessageHeader& header)
    {
        ProtocolVersion version(header.call_id());
        if (version != PROTOCOL_VERSION) {
            do_close_connection();
        }

        auto msg = read_message(header.message_size());
        ConnectionMetadata meta(msg.root().as_tiny_object_map());

        auto buf_size = meta.channel_buffer_size();
        if (buf_size < channel_buffer_size_) {
            channel_buffer_size_ = buf_size;
        }
    }

    void invoke_handler(const MessageHeader& header)
    {
        auto msg = read_message(header.message_size());
        Request rq(msg.root().as_tiny_object_map());
        auto ctx = context(header.call_id());
        if (ctx.is_null())
        {
            auto endpoint_id = rq.endpoint();
            auto handler = service_->get_handler(endpoint_id);
            if (handler.has_value())
            {
                ctx = make_context(header.call_id(), rq);
                contexts_[header.call_id()] = ctx;
                run_handler(handler.get(), ctx, header.call_id());
            }
        }
    }


    struct CtxCleaner {
        ConnectionImplPtr conn;
        CallID call_id;

        ~CtxCleaner() {
            conn->contexts_.erase(call_id);
        }
    };
    friend struct CtxCleaner;


    void run_handler(RequestHandlerFn handler, const ContextImplPtr& ctx, CallID call_id)
    {
        auto fiber = reactor::engine().in_fiber([=](){
            CtxCleaner cleaner{shared_from_this(), call_id};

            this_fiber::yield();
            try {
                hrpc::Response rs = handler(ctx);
                send_response(rs, call_id);
            }
            catch (...) {
                hrpc::Response rs = Response::error0();
                send_response(rs, call_id);
            }
        });

        fiber.detach();
    }

    void send_response(Response rs, CallID call_id)
    {
        Response imm_rs = rs.compactify();

        auto data = imm_rs.object().ctr().span();

        MessageHeader header;
        header.set_message_size(data.size());
        header.set_message_type(MessageType::RETURN);
        header.set_call_id(call_id);

        output_stream_.write(ptr_cast<uint8_t>(&header), sizeof(header));
        output_stream_.write(data.data(), data.size());
    }

    void do_close_connection() {
        streams_->close();
    }
};

}
