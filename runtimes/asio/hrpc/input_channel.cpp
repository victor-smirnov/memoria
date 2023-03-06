
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

#include <memoria/asio/hrpc/input_channel.hpp>
#include <memoria/hrpc/hrpc_impl_session.hpp>

namespace memoria::asio::hrpc {


PoolSharedPtr<st::Session> AsioHRPCInputChannel::session() {
    return session_;
}

bool AsioHRPCInputChannel::is_closed() {
    return channel_.is_closed();
}

bool AsioHRPCInputChannel::pop(Message& msg)
{
    bool success = channel_.pop(msg);
    if (success)
    {
        size_t msg_size = msg.object().ctr().memory_size();
        batch_size_ += msg_size;
        if (batch_size_ >= batch_size_limit_ / 2) {
            session_->unblock_output_channel(call_id_, channel_code_, call_side_);
        }
    }
    return success;
}

void AsioHRPCInputChannel::close()
{
    channel_.clean_and_close();

    MessageType type;
    if (call_side_) {
        type = MessageType::CALL_CLOSE_OUTPUT_CHANNEL;
    }
    else {
        type = MessageType::CONTEXT_CLOSE_OUTPUT_CHANNEL;
    }

    session_->send_message(
        0,
        [&](MessageHeader& header) {
            header.set_message_type(type);
            header.set_call_id(call_id_);
            header.set_channel_code(channel_code_);
        }
    );
}

void AsioHRPCInputChannel::new_message(Message&& msg)
{
    if (!channel_.is_closed()) {
        channel_.push(std::move(msg));
    }
}

void AsioHRPCInputChannel::do_close_channel() {
    channel_.close();
}

}
