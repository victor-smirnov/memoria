
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
    batch_size_limit_(connection_->channel_buffer_size())
{
    // Output stream at the Call side becomes input stream
    // here at the Context die and vice versa.
    for (size_t c = 0; c < request.output_channels(); c++) {
        input_channels_.push_back(make_input_channel(c));
    }

    for (size_t c = 0; c < request.input_channels(); c++) {
        output_channels_.push_back(make_output_channel(c));
    }
}

PoolSharedPtr<Connection> HRPCContextImpl::connection() {
    return connection_;
}

void HRPCContextImpl::close_channel(bool input, ChannelCode code)
{
    if (input) {
        if (code < input_channels_.size() && !input_channels_[code].is_null()) {
            input_channels_[code]->do_close_channel();
        }
    }
    else {
        if (code < output_channels_.size() && !output_channels_[code].is_null()) {
            output_channels_[code]->do_close_channel();
        }
    }
}

void HRPCContextImpl::cancel_call()
{
    for (auto i_s: input_channels_) {
        i_s->do_close_channel();
    }

    for (auto o_s: output_channels_) {
        o_s->do_close_channel();
    }

    cancelled_ = true;
    if (cancel_listener_) {
        reactor::in_fiber(cancel_listener_).detach();
    }
}

void HRPCContextImpl::new_message(Message&& msg, ChannelCode code)
{
    if (code < input_channels_.size() && !input_channels_[code].is_null()) {
        input_channels_[code]->new_message(std::move(msg));
    }
}

void HRPCContextImpl::reset_output_channel_buffer(ChannelCode code)
{
    if (code < output_channels_.size() && !output_channels_[code].is_null()) {
        output_channels_[code]->reset_buffer_size();
    }
}

InputChannelImplPtr HRPCContextImpl::make_input_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCInputChannelImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, false);
}

OutputChannelImplPtr HRPCContextImpl::make_output_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCOutputChannelImpl>>();

    return pool->allocate_shared(connection_, call_id_, code, batch_size_limit_, false);
}

}
