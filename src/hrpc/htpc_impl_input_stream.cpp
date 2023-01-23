
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

#include "hrpc_impl_input_stream.hpp"
#include "hrpc_impl_connection.hpp"

#include <memoria/fiber/fiber.hpp>

namespace memoria::hrpc {

HRPCInputStreamImpl::HRPCInputStreamImpl(
        ConnectionImplPtr connection,
        CallID call_id,
        StreamCode stream_code,
        bool call_side
):
    connection_(connection), call_id_(call_id),
    stream_code_(stream_code), closed_(),
    call_side_(call_side)
{}

PoolSharedPtr<Connection> HRPCInputStreamImpl::connection() {
    return connection_;
}

bool HRPCInputStreamImpl::is_closed() {
    return closed_;
}

void HRPCInputStreamImpl::next()
{
    if (head_.is_null()) {
        while (head_.is_null() && !closed_) {
            auto ctx = fibers::context::active();
            reactor::engine().suspend_fiber(ctx);
        }
    }
}

StreamBatch HRPCInputStreamImpl::batch()
{
    while (head_.is_null() && !closed_) {
        auto ctx = fibers::context::active();
        reactor::engine().suspend_fiber(ctx);
    }

    if (head_.is_null()) {
        MEMORIA_MAKE_GENERIC_ERROR("Stream {} for {} has been closed.", stream_code_, call_id_).do_throw();
    }

    return head_->batch;
}

void HRPCInputStreamImpl::close() {
    closed_ = true;
}

void HRPCInputStreamImpl::new_message(StreamBatch&& msg) {

}

void HRPCInputStreamImpl::do_close_stream() {
    closed_ = true;
}

}
