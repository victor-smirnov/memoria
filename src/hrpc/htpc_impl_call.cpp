
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

#include "hrpc_impl_call.hpp"
#include "hrpc_impl_connection.hpp"

namespace memoria::hrpc {

PoolSharedPtr<Connection> HRPCCallImpl::connection() {
    return connection_;
}

HRPCCallImpl::HRPCCallImpl(const PoolSharedPtr<HRPCConnectionImpl>& connection, Request request):
    connection_(connection),
    request_(request)
{
    call_id_ = connection_->next_call_id();

    for (size_t c = 0; c < request.input_streams(); c++) {
        input_streams_.push_back(make_istream(c));
    }

    for (size_t c = 0; c < request.output_streams(); c++) {
        output_streams_.push_back(make_ostream(c));
    }
}



Response HRPCCallImpl::response() {
    return response_;
}

void HRPCCallImpl::wait() {

}

void HRPCCallImpl::on_complete(CallCompletionFn fn) {}

void HRPCCallImpl::cancel()
{
    connection_->send_message(
        MessageType::CANCEL_CALL,
        call_id_,
        0
    );
}

void HRPCCallImpl::set_response(Response rs) {

}

void HRPCCallImpl::new_message(StreamBatch&& msg, StreamCode code)
{
    if (code < input_streams_.size() && !input_streams_[code].is_null()) {
        input_streams_[code]->new_message(std::move(msg));
    }
}

void HRPCCallImpl::close_stream(bool input, StreamCode code)
{
    if (input) {
        if (code < input_streams_.size() && !input_streams_[code].is_null()) {
            input_streams_[code]->do_close_stream();
        }
    }
    else {
        if (code < output_streams_.size() && !output_streams_[code].is_null()) {
            output_streams_[code]->do_close_stream();
        }
    }
}

InputStreamImplPtr HRPCCallImpl::make_istream(StreamCode code) {
    static thread_local pool::SimpleObjectPool<HRPCInputStreamImpl> pool;
    return pool.allocate_shared(connection_, call_id_, code, true);
}

OutputStreamImplPtr HRPCCallImpl::make_ostream(StreamCode code) {
    static thread_local pool::SimpleObjectPool<HRPCOutputStreamImpl> pool;
    return pool.allocate_shared(connection_, call_id_, code, true);
}

}
