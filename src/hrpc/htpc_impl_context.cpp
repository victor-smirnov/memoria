
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

#include "hrpc_impl_context.hpp"
#include "hrpc_impl_connection.hpp"


namespace memoria::hrpc {

HRPCContextImpl::HRPCContextImpl(ConnectionImplPtr connection, CallID call_id, Request request):
    connection_(connection),
    call_id_(call_id),
    request_(request),
    batch_size_limit_(connection_->stream_buffer_size())
{
    // Output stream at the Call side becomes input stream
    // here at the Context die and vice versa.
    for (size_t c = 0; c < request.output_streams(); c++) {
        input_streams_.push_back(make_istream(c));
    }

    for (size_t c = 0; c < request.input_streams(); c++) {
        output_streams_.push_back(make_ostream(c));
    }
}

PoolSharedPtr<Connection> HRPCContextImpl::connection() {
    return connection_;
}

void HRPCContextImpl::close_stream(bool input, StreamCode code)
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

void HRPCContextImpl::cancel_call()
{
    for (auto i_s: input_streams_) {
        i_s->do_close_stream();
    }

    for (auto o_s: output_streams_) {
        o_s->do_close_stream();
    }

    cancelled_ = true;
    if (cancel_listener_) {
        reactor::in_fiber(cancel_listener_).detach();
    }
}

void HRPCContextImpl::new_message(StreamMessage&& msg, StreamCode code)
{
    if (code < input_streams_.size() && !input_streams_[code].is_null()) {
        input_streams_[code]->new_message(std::move(msg));
    }
}

void HRPCContextImpl::reset_ostream_buffer(StreamCode code)
{
    if (code < output_streams_.size() && !output_streams_[code].is_null()) {
        output_streams_[code]->reset_buffer_size();
    }
}

InputStreamImplPtr HRPCContextImpl::make_istream(StreamCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCInputStreamImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, false);
}

OutputStreamImplPtr HRPCContextImpl::make_ostream(StreamCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCOutputStreamImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, false);
}

}
