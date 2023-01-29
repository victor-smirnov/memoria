
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

HRPCCallImpl::HRPCCallImpl(
        const PoolSharedPtr<HRPCConnectionImpl>& connection,
        CallID call_id,
        Request request,
        CallCompletionFn completion_fn
):
    connection_(connection),
    request_(request),
    call_id_(call_id),
    batch_size_limit_(connection->stream_buffer_size()),
    completion_fn_(completion_fn)
{
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

void HRPCCallImpl::wait()
{
    std::unique_lock<fibers::mutex> lk(mutex_);
    waiter_.wait(lk, [&](){
        return response_.is_null();
    });
}

void HRPCCallImpl::on_complete(CallCompletionFn fn) {
    completion_fn_ = fn;
}

void HRPCCallImpl::cancel()
{
    connection_->send_message(
        MessageType::CANCEL_CALL,
        call_id_
    );
}

void HRPCCallImpl::set_response(Response rs)
{
    response_ = rs;
    waiter_.notify_all();

    if (completion_fn_) {
        reactor::engine().in_fiber(completion_fn_, response_).detach();
    }
}

void HRPCCallImpl::new_message(StreamMessage&& msg, StreamCode code)
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

void HRPCCallImpl::reset_ostream_buffer(StreamCode code)
{
    if (code < output_streams_.size() && !output_streams_[code].is_null()) {
        output_streams_[code]->reset_buffer_size();
    }
}

InputStreamImplPtr HRPCCallImpl::make_istream(StreamCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCInputStreamImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, true);
}

OutputStreamImplPtr HRPCCallImpl::make_ostream(StreamCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCOutputStreamImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, true);
}


}
