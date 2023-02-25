
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

#include "hrpc_impl_output_channel.hpp"
#include "hrpc_impl_session.hpp"

namespace memoria::hrpc {

HRPCOutputChannelImpl::HRPCOutputChannelImpl(
    const SessionImplPtr& session,
    CallID call_id,
    ChannelCode code,
    uint64_t batch_size_limit,
    bool call_side
):
    session_(session),
    call_id_(call_id),
    code_(code),
    closed_(),
    call_side_(call_side),
    batch_size_limit_(batch_size_limit)
{}


PoolSharedPtr<Session> HRPCOutputChannelImpl::session() {
    return session_;
}

void HRPCOutputChannelImpl::push(const Message& msg)
{
    if (!closed_)
    {
        if (batch_size_ >= batch_size_limit_)
        {
            flow_control_.wait([&](){
                return batch_size_ >= batch_size_limit_;
            }).get();
        }

        MessageType msg_type = call_side_ ? MessageType::CALL_CHANNEL_MESSAGE : MessageType::CONTEXT_CHANNEL_MESSAGE;
        batch_size_ += session_->send_message(
            msg.object().ctr(),
            [&](MessageHeader& header){
                header.set_message_type(msg_type);
                header.set_call_id(call_id_);
                header.set_channel_code(code_);
            }
        );
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Channel {} for {} has been closed", code_, call_id_);
    }
}

void HRPCOutputChannelImpl::close()
{
    MessageType msg_type = call_side_ ? MessageType::CALL_CLOSE_INPUT_CHANNEL : MessageType::CONTEXT_CLOSE_INPUT_CHANNEL;

    session_->send_message(
        [&](MessageHeader& header){
            header.set_message_type(msg_type);
            header.set_call_id(call_id_);
            header.set_channel_code(code_);
        }
    );

    do_close_channel();
}

void HRPCOutputChannelImpl::do_close_channel() {
    closed_ = true;
    reset_buffer_size();
}

void HRPCOutputChannelImpl::reset_buffer_size() {
    batch_size_ = 0;
    flow_control_.broadcast();
}


}
