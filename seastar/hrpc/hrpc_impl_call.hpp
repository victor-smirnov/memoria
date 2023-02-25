
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

#pragma once

#include "hrpc_impl_common.hpp"

#include <vector>

namespace memoria::hrpc {

class HRPCCallImpl final:
        public Call,
        public pool::enable_shared_from_this<HRPCCallImpl>
{
    SessionImplPtr session_;
    Request request_;
    Response response_;
    CallID call_id_;

    uint64_t batch_size_limit_;

    std::vector<InputChannelImplPtr> input_channels_;
    std::vector<OutputChannelImplPtr> output_channels_;

    CallCompletionFn completion_fn_;

    seastar::condition_variable waiter_;

public:
    HRPCCallImpl(
            const SessionImplPtr& session,
            CallID call_id,
            Request request,
            CallCompletionFn completion_fn
    );

    PoolSharedPtr<Session> session() override;

    CallID call_id() override {
        return call_id_;
    }

    Request request() override {
        return request_;
    }

    Response response() override;

    void wait() override;

    void on_complete(CallCompletionFn fn) override;

    void cancel() override;

    void set_response(Response rs);

    size_t input_channels() override {
        return input_channels_.size();
    }

    size_t output_channels() override {
        return output_channels_.size();
    }

    PoolSharedPtr<InputChannel> input_channel(size_t idx) override
    {
        if (idx < input_channels_.size()) {
            return input_channels_[idx];
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Stream code is out of range: {}, max is {}",
                idx,
                input_channels_.size()
            ).do_throw();
        }
    }

    PoolSharedPtr<OutputChannel> output_channel(size_t idx) override
    {
        if (idx < output_channels_.size()) {
            return output_channels_[idx];
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Stream code is out of range: {}, max is {}",
                idx,
                output_channels_.size()
            ).do_throw();
        }
    }

    void new_message(Message&& msg, ChannelCode code);
    void close_channel(bool input, ChannelCode code);

    void close_channels() {
        MEMORIA_MAKE_GENERIC_ERROR("Call::close_channels() is not implemented").do_throw();
    }

    void reset_output_channel_buffer(ChannelCode code);

private:
    InputChannelImplPtr make_input_channel(ChannelCode code);
    OutputChannelImplPtr make_output_channel(ChannelCode code);
};


}
