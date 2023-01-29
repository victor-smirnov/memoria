
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

#include "hrpc_impl_output_stream.hpp"
#include "hrpc_impl_connection.hpp"

namespace memoria::hrpc {

HRPCOutputStreamImpl::HRPCOutputStreamImpl(
    const ConnectionImplPtr& conn,
    CallID call_id,
    StreamCode stream_code,
    uint64_t batch_size_limit,
    bool call_side
):
    connection_(conn),
    call_id_(call_id),
    stream_code_(stream_code),
    closed_(),
    call_side_(call_side),
    batch_size_limit_(batch_size_limit)
{}


PoolSharedPtr<Connection> HRPCOutputStreamImpl::connection() {
    return connection_;
}

void HRPCOutputStreamImpl::push(const StreamMessage& msg)
{
    if (!closed_)
    {
        if (batch_size_ >= batch_size_limit_)
        {
            std::unique_lock<fibers::mutex> lk(mutex_);
            flow_control_.wait(lk, [&](){
                return batch_size_ >= batch_size_limit_;
            });
        }

        MessageType msg_type = call_side_ ? MessageType::CALL_STREAM_MESSAGE : MessageType::CONTEXT_STREAM_MESSAGE;
        batch_size_ += connection_->send_message(
            msg_type,
            call_id_,
            msg.object().ctr(),
            stream_code_
        );
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Stream {} for {} has been closed", (int)stream_code_, call_id_);
    }
}

void HRPCOutputStreamImpl::close()
{
    MessageType msg_type = call_side_ ? MessageType::CALL_CLOSE_INPUT_STREAM : MessageType::CONTEXT_CLOSE_INPUT_STREAM;

    connection_->send_message(
        msg_type,
        call_id_,
        stream_code_
    );

    do_close_stream();
}

void HRPCOutputStreamImpl::do_close_stream() {
    closed_ = true;
    reset_buffer_size();
}

void HRPCOutputStreamImpl::reset_buffer_size() {
    batch_size_ = 0;
    flow_control_.notify_all();
}


}
