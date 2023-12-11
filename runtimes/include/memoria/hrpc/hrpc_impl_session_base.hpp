
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

#include <memoria/hrpc/hrpc_impl_context.hpp>
#include <memoria/hrpc/hrpc_impl_call.hpp>
#include <memoria/hrpc/hrpc_impl_input_channel.hpp>
#include <memoria/hrpc/hrpc_impl_output_channel.hpp>

#include <boost/exception/exception.hpp>

namespace memoria::hrpc::st {

class HRPCSessionBase: public Session {
protected:
    EndpointRepositoryImplPtr endpoints_;
    ProtocolConfig config_;

    ska::flat_hash_map<CallID, CallImplWeakPtr> calls_;
    ska::flat_hash_map<CallID, ContextImplPtr> contexts_;

    bool closed_{false};
    SessionSide session_side_;

    CallID call_id_cnt_;

    uint64_t channel_buffer_size_; // 1MB

    uint32_t default_header_opt_fields_{};

    using HeaderFn = std::function<void(MessageHeader&)>;


    bool negotiated_{};
    bool set_session_id_attr_;

    SessionID session_id_;

public:
    HRPCSessionBase(
        EndpointRepositoryImplPtr endpoints,
        ProtocolConfig config,
        SessionSide session_side,
        bool set_session_id_attr
    ):
        endpoints_(endpoints),
        config_(config),
        session_side_(session_side),
        channel_buffer_size_(config.channel_buffer_size()),
        set_session_id_attr_(set_session_id_attr)
    {
        session_id_ = SessionID::make_random();

        if (session_side_ == SessionSide::CLIENT) {
            call_id_cnt_ = 0;
        }
        else {
            call_id_cnt_ = 1;
        }

        if (set_session_id_attr_) {
            default_header_opt_fields_ = default_header_opt_fields_ | HeaderOptF::SESSION_ID;
        }
    }

    virtual void wait_for_negotiation() = 0;
    virtual void notify_negotiated() = 0;
    virtual void run_async(std::function<void()> fn) = 0;
    virtual void write_message(const MessageHeader& header, const uint8_t* data) = 0;
    virtual bool is_transport_closed() = 0;

    virtual CallImplPtr create_call(
            CallID call_id,
            Request request,
            CallCompletionFn completion_fn
    ) = 0;


    virtual void start_session() {
        if (session_side_ == SessionSide::CLIENT) {
            send_session_metadata();
        }
    }

    virtual bool is_closed() override {
        return closed_ || is_transport_closed();
    }


    uint64_t channel_buffer_size() const {
        return channel_buffer_size_;
    }

    PoolSharedPtr<EndpointRepository> endpoints() override {
        return endpoints_;
    }


    PoolSharedPtr<Call> call(
            const EndpointID& endpoint_id,
            Request request,
            Optional<ShardID> shard_id,
            CallCompletionFn completion_fn
    ) override {
        CallID call_id = next_call_id();

        HeaderOptF shard = shard_id ? HeaderOptF::SHARD_ID : HeaderOptF::NONE;

        send_message(HeaderOptF::ENDPOINT_ID | shard | 0, request.object().ctr(), [&](MessageHeader& header){
            header.set_message_type(MessageType::CALL);
            header.set_call_id(call_id);

            if (shard_id) {
                header.set_opt_shard_id(shard_id.value());
            }

            header.set_opt_endpoint_id(endpoint_id);
        });

        auto call = create_call(call_id, request, completion_fn);
        calls_[call_id] = CallImplWeakPtr(call);

        return call;
    }

    hermes::HermesCtr extract_ctr(MessageHeader* header, RawMessagePtr&& buffer)
    {
        size_t header_size  = header->header_size();
        size_t message_size = header->message_size();

        if (message_size >= header_size + hermes::HermesCtr::minimum_ctr_size())
        {
            return hermes::HermesCtr::from_buffer(std::move(buffer), message_size, header_size);
        }
        else if (message_size == header_size) {
            return {};
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid message_size: {}, header_size = ", message_size, header_size).do_throw();
        }
    }

    virtual void handle_message(RawMessagePtr&& raw_msg)
    {
        MessageHeader* header = ptr_cast<MessageHeader>(raw_msg.get());
        auto msg = extract_ctr(header, std::move(raw_msg));

//        if (msg.is_not_empty()) {
//            println("Incoming MSG: {} :: {}", (int)header->message_type(), msg.to_pretty_string());
//        }
//        else {
//            println("Incoming MSG: {}", (int)header->message_type());
//        }

        switch (header->message_type()) {
        case MessageType::SESSION_START: {
            if (session_side_ == SessionSide::SERVER) {
                prepare_server_session(*header, std::move(msg));
                send_session_metadata();
            }
            else {
                prepare_client_session(*header, std::move(msg));
            }
            break;
        }
        case MessageType::SESSION_CLOSE: {
            do_session_close();
            do_session_cleanup();
            break;
        }
        case MessageType::CALL: {
            invoke_handler(*header, std::move(msg));
            break;
        }
        case MessageType::CALL_CHANNEL_MESSAGE: {
            handle_call_side_stream_message(*header, std::move(msg));
            break;
        }
        case MessageType::CONTEXT_CHANNEL_MESSAGE: {
            handle_context_side_stream_message(*header, std::move(msg));
            break;
        }
        case MessageType::CALL_CLOSE_INPUT_CHANNEL: {
            handle_call_close_stream(*header, true);
            break;
        }
        case MessageType::CONTEXT_CLOSE_INPUT_CHANNEL: {
            handle_context_close_stream(*header, true);
            break;
        }
        case MessageType::CALL_CLOSE_OUTPUT_CHANNEL: {
            handle_call_close_stream(*header, false);
            break;
        }
        case MessageType::CONTEXT_CLOSE_OUTPUT_CHANNEL: {
            handle_context_close_stream(*header, false);
            break;
        }
        case MessageType::CANCEL_CALL: {
            handle_cancel_call(*header);
            break;
        }
        case MessageType::RETURN: {
            handle_return(*header, std::move(msg));
            break;
        }
        case MessageType::CALL_CHANNEL_BUFFER_RESET: {
            handle_call_stream_buffer_reset(*header);
            break;
        }
        case MessageType::CONTEXT_CHANNEL_BUFFER_RESET: {
            handle_context_stream_buffer_reset(*header);
            break;
        }
        }
    }

    void close() override
    {
        if (!closed_)
        {
            if (!is_transport_closed())
            {
                send_message([&](MessageHeader& header){
                    header.set_message_type(MessageType::SESSION_CLOSE);
                });

                do_session_close();
            }
            closed_ = true;
        }
    }


    size_t send_message(const hermes::HermesCtr& ctr, HeaderFn header_fn) {
        return send_message(0, ctr, header_fn);
    }

    size_t send_message(uint32_t optionals, const hermes::HermesCtr& ctr, HeaderFn header_fn, bool force = false)
    {
        if (MMA_UNLIKELY((!negotiated_) && !force))
        {
            wait_for_negotiation();
        }

        optionals |= default_header_opt_fields_;

        size_t header_size = MessageHeader::header_size_for(optionals);
        auto ctr_imm = ctr.compactify(true, header_size);

        auto data = ctr_imm.span();

        MessageHeader* header = new (data.data()) MessageHeader(optionals);
        header->set_message_size(data.size());

        if (set_session_id_attr_) {
            header->set_opt_session_id(session_id_);
        }

        header_fn(*header);

        write_message(*header, data.data());
        return data.size();
    }

    void send_message(HeaderFn header_fn) {
        return send_message(0, header_fn);
    }

    void send_message(uint32_t optionals, HeaderFn header_fn)
    {
        optionals |= default_header_opt_fields_;

        size_t header_size = MessageHeader::header_size_for(optionals);
        auto buffer = allocate_system_zeroed<uint8_t>(header_size);
        MessageHeader* header = new (buffer.get()) MessageHeader(optionals);
        header->set_message_size(header_size);

        if (set_session_id_attr_) {
            header->set_opt_session_id(session_id_);
        }

        header_fn(*header);
        write_message(*header, buffer.get());
    }



    CallID next_call_id()
    {
        call_id_cnt_ += 2;
        return call_id_cnt_;
    }


    void unblock_output_channel(CallID call_id, ChannelCode code, bool call_side)
    {
        MessageType type;
        if (call_side) {
            type = MessageType::CALL_CHANNEL_BUFFER_RESET;
        }
        else {
            type = MessageType::CONTEXT_CHANNEL_BUFFER_RESET;
        }

        send_message([&](MessageHeader& header){
            header.set_message_type(type);
            header.set_call_id(call_id);
            header.set_channel_code(code);
        });
    }


protected:
    virtual SessionImplPtr self() = 0;

    virtual void do_session_close() noexcept = 0;

    void do_session_cleanup()
    {
        if (!is_closed())
        {
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

    void handle_context_side_stream_message(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        auto ii = calls_.find(header.call_id());
        if (ii != calls_.end())
        {
            auto ptr = ii->second.lock();
            if (!ptr.is_null()) {
                Message rq(msg.root().value().as_tiny_object_map());
                ptr->new_message(std::move(rq), header.channel_code());
            }
            else {
                calls_.erase(header.call_id());
            }
        }
    }

    void handle_call_side_stream_message(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        auto ctx = context(header.call_id());
        if (!ctx.is_null()) {
            Message rq(msg.root().value().as_tiny_object_map());
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

    void write_to(const MessageHeader* header, void* data) {
        std::memcpy(data, header, header->header_size());
    }

    void send_session_metadata()
    {
        ConnectionMetadata meta(hermes::HermesCtr::make_pooled());
        meta.set_channel_buffer_size(channel_buffer_size_);

        send_message(0, meta.object().ctr(), [](MessageHeader& header){
            header.set_message_type(MessageType::SESSION_START);
            header.set_call_id(PROTOCOL_VERSION.value());
        }, true);
    }

    void handle_return(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        Response rs(msg.root().value().as_tiny_object_map());

        auto ii = calls_.find(header.call_id());
        if (ii != calls_.end())
        {
            auto ptr = ii->second.lock();
            if (!ptr.is_null()) {
                ptr->set_response(rs);
            }
            calls_.erase(ii);
        }
    }

    ContextImplPtr context(CallID id)
    {
        auto ii = contexts_.find(id);
        if (ii != contexts_.end()) {
            return ii->second;
        }
        return {};
    }

    virtual ContextImplPtr make_context(CallID call_id, const EndpointID& endpoint_id, Request rq) = 0;
//    {
//        static thread_local auto pool
//            = boost::make_local_shared<
//                pool::SimpleObjectPool<HRPCContextImpl>
//            >();

//        return pool->allocate_shared(self(), call_id, endpoint_id, rq);
//    }

    void prepare_server_session(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        ProtocolVersion version(header.call_id());
        if (version != PROTOCOL_VERSION) {
            do_session_close();
        }

        ConnectionMetadata meta(msg.root().value().as_tiny_object_map());

        auto buf_size = meta.channel_buffer_size();
        if (buf_size < channel_buffer_size_) {
            channel_buffer_size_ = buf_size;
        }

        // TODO: Handle protocoll mismatches gracefully!
        negotiated_ = true;
        notify_negotiated();
    }

    void prepare_client_session(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        ProtocolVersion version(header.call_id());
        if (version != PROTOCOL_VERSION) {
            do_session_close();
        }

        ConnectionMetadata meta(msg.root().value().as_tiny_object_map());

        auto buf_size = meta.channel_buffer_size();
        if (buf_size < channel_buffer_size_) {
            channel_buffer_size_ = buf_size;
        }

        // TODO: Handle protocoll mismatches gracefully!
        negotiated_ = true;
        notify_negotiated();
    }

    void invoke_handler(const MessageHeader& header, hermes::HermesCtr&& msg)
    {
        Request rq(msg.root().value().as_tiny_object_map());
        auto ctx = context(header.call_id());
        if (ctx.is_null())
        {
            auto endpoint_id = header.endpoint_id();
            auto handler = endpoints_->get_handler(endpoint_id);
            if (handler.has_value())
            {
                ctx = make_context(header.call_id(), endpoint_id, rq);
                contexts_[header.call_id()] = ctx;
                run_handler(handler.value(), ctx, header.call_id());
            }
            else {
                Response rs = Response::error0();
                HrpcError error = rs.set_hrpc_error(HrpcErrors::INVALID_ENDPOINT);
                error.set_endpoint_id(endpoint_id);
                error.set_description(
                    format_u8("Invalid endpoin: {}", endpoint_id)
                );
                send_response(rs, header.call_id());
            }
        }
    }


    struct CtxCleaner {
        PoolSharedPtr<HRPCSessionBase> session;
        CallID call_id;

        ~CtxCleaner() {
            session->contexts_.erase(call_id);
        }
    };
    friend struct CtxCleaner;

    void run_handler(RequestHandlerFn handler, const ContextImplPtr& ctx, CallID call_id)
    {
        run_async([=, this](){
            CtxCleaner cleaner{self(), call_id};
            try {
                Response rs = handler(ctx);
                send_response(rs, call_id);
            }
            catch (const ResultException& err)
            {
                Response rs = Response::error0();
                rs.set_error(ErrorType::MEMORIA, err.what());
                send_response(rs, call_id);
            }
            catch (const MemoriaError& err)
            {
                Response rs = Response::error0();
                rs.set_error(ErrorType::MEMORIA, err.what());
                send_response(rs, call_id);
            }
            catch (const MemoriaThrowable& err)
            {
                Response rs = Response::error0();
                rs.set_error(ErrorType::MEMORIA, err.what());
                send_response(rs, call_id);
            }
            catch (const boost::exception& err)
            {
                Response rs = Response::error0();
                Error error = rs.set_error(ErrorType::BOOST);

                SBuf buf;
                buf << boost::diagnostic_information(err);
                error.set_description(buf.str());

                send_response(rs, call_id);
            }
            catch (const std::system_error& err)
            {
                Response rs = Response::error0();
                rs.set_error(err);
                send_response(rs, call_id);
            }
            catch (const std::exception& err)
            {
                Response rs = Response::error0();
                rs.set_error(hrpc::ErrorType::CXX_STD, err.what());
                send_response(rs, call_id);
            }
            catch (...) {
                Response rs = Response::error0();
                Error error = rs.set_error(ErrorType::UNKNOWN);
                error.set_description(boost::current_exception_diagnostic_information());
                send_response(rs, call_id);
            }
        });
    }

    void send_response(Response rs, CallID call_id)
    {
        send_message(rs.object().ctr(), [&](MessageHeader& header){
            header.set_message_type(MessageType::RETURN);
            header.set_call_id(call_id);
        });
    }
};

}
