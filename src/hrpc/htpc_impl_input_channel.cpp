
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

#include "hrpc_impl_input_channel.hpp"
#include "hrpc_impl_connection.hpp"

#include <memoria/fiber/fiber.hpp>
#include <memoria/reactor/reactor.hpp>

namespace memoria::hrpc {

HRPCInputChannelImpl::HRPCInputChannelImpl(
        ConnectionImplPtr connection,
        CallID call_id,
        ChannelCode channel_code,
        uint64_t batch_size_limit,
        bool call_side
):
    connection_(connection), call_id_(call_id),
    channel_code_(channel_code), closed_(),
    call_side_(call_side),
    batch_size_limit_(batch_size_limit)
{}

PoolSharedPtr<Connection> HRPCInputChannelImpl::connection() {
    return connection_;
}

bool HRPCInputChannelImpl::is_closed() {
    return channel_.is_closed();
}

bool HRPCInputChannelImpl::pop(Message& msg)
{
    bool success = channel_.pop(msg);
    if (success)
    {
        size_t msg_size = msg.object().ctr().memory_size();
        batch_size_ += msg_size;
        if (batch_size_ >= batch_size_limit_ / 2) {
            connection_->unblock_output_channel(call_id_, channel_code_, call_side_);
        }
    }
    return success;
}

void HRPCInputChannelImpl::close()
{
    channel_.clean_and_close();

    MessageType type;
    if (call_side_) {
        type = MessageType::CALL_CLOSE_OUTPUT_CHANNEL;
    }
    else {
        type = MessageType::CONTEXT_CLOSE_OUTPUT_CHANNEL;
    }

    connection_->send_message(
        type, call_id_, channel_code_
    );
}

void HRPCInputChannelImpl::new_message(Message&& msg)
{
    if (!channel_.is_closed()) {
        channel_.push(std::move(msg));
    }
}

void HRPCInputChannelImpl::do_close_channel() {
    channel_.close();
}

}
