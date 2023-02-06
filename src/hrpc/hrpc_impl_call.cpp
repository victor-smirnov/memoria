
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
#include "hrpc_impl_session.hpp"

namespace memoria::hrpc {

PoolSharedPtr<Session> HRPCCallImpl::session() {
    return session_;
}

HRPCCallImpl::HRPCCallImpl(
        const SessionImplPtr& session,
        CallID call_id,
        Request request,
        CallCompletionFn completion_fn
):
    session_(session),
    request_(request),
    call_id_(call_id),
    batch_size_limit_(session->channel_buffer_size()),
    completion_fn_(completion_fn)
{
    for (size_t c = 0; c < request.input_channels(); c++) {
        input_channels_.push_back(make_input_channel(c));
    }

    for (size_t c = 0; c < request.output_channels(); c++) {
        output_channels_.push_back(make_output_channel(c));
    }
}



Response HRPCCallImpl::response() {
    return response_;
}

void HRPCCallImpl::wait()
{
    if (response_.is_null())
    {
        std::unique_lock<fibers::mutex> lk(mutex_);
        waiter_.wait(lk, [&](){
            return response_.is_not_null();
        });
    }
}

void HRPCCallImpl::on_complete(CallCompletionFn fn) {
    completion_fn_ = fn;
}

void HRPCCallImpl::cancel()
{
    session_->send_message(
        0,
        [&](MessageHeader& header){
            header.set_message_type(MessageType::CANCEL_CALL);
            header.set_call_id(call_id_);
        }
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

void HRPCCallImpl::new_message(Message&& msg, ChannelCode code)
{
    if (code < input_channels_.size() && !input_channels_[code].is_null()) {
        input_channels_[code]->new_message(std::move(msg));
    }
}

void HRPCCallImpl::close_channel(bool input, ChannelCode code)
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

void HRPCCallImpl::reset_output_channel_buffer(ChannelCode code)
{
    if (code < output_channels_.size() && !output_channels_[code].is_null()) {
        output_channels_[code]->reset_buffer_size();
    }
}

InputChannelImplPtr HRPCCallImpl::make_input_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCInputChannelImpl>>();

    return pool->allocate_shared(session_, call_id_, code, batch_size_limit_, true);
}

OutputChannelImplPtr HRPCCallImpl::make_output_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<HRPCOutputChannelImpl>>();

    return pool->allocate_shared(session_, call_id_, code, batch_size_limit_, true);
}


}
