
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

namespace memoria::hrpc {

class HRPCContextImpl final:
        public Context,
        public pool::enable_shared_from_this<HRPCContextImpl>
{
    ConnectionImplPtr connection_;
    CallID call_id_;
    Request request_;

    std::vector<InputChannelImplPtr> input_channels_;
    std::vector<OutputChannelImplPtr> output_channels_;

    uint64_t batch_size_limit_{1024*1024};

    bool cancelled_{};

    CancelCallListenerFn cancel_listener_;

public:
    HRPCContextImpl(ConnectionImplPtr connection, CallID call_id, Request request);

    PoolSharedPtr<Connection> connection() override;

    Request request() override {
        return request_;
    }

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
                "Channel code is out of range: {}, max is {}",
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
                "Channel code is out of range: {}, max is {}",
                idx,
                output_channels_.size()
            ).do_throw();
        }
    }

    hermes::Object get(NamedCode name) override {
        return {};
    }

    void set(NamedCode name, hermes::Object object) override {}

    void new_message(Message&& msg, ChannelCode code);

    void close_channel(bool input, ChannelCode code);

    void cancel_call();

    void reset_output_channel_buffer(ChannelCode code);

    InputChannelImplPtr  make_input_channel(ChannelCode code);
    OutputChannelImplPtr make_output_channel(ChannelCode code);

    bool is_cancelled() override {
        return cancelled_;
    }

    void set_cancel_listener(CancelCallListenerFn fn) override
    {
        if (cancelled_) {
            fn();
        }
        else {
            cancel_listener_ = fn;
        }
    }
};

}
