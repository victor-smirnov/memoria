
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

class HRPCOutputChannelImpl final:
        public OutputChannel,
        public pool::enable_shared_from_this<HRPCOutputChannelImpl>
{
    SessionImplPtr session_;
    CallID call_id_;
    ChannelCode code_;
    bool closed_;
    bool call_side_;

    uint64_t batch_size_limit_{};
    uint64_t batch_size_{};

    seastar::condition_variable flow_control_;


public:
    HRPCOutputChannelImpl(
        const SessionImplPtr& session,
        CallID call_id, ChannelCode code,
        uint64_t batch_size_limit, bool call_side
    );

    ChannelCode code() override {
        return code_;
    }

    PoolSharedPtr<Session> session() override;

    void push(const Message& msg) override;

    void close() override;
    void do_close();

    bool is_closed() override {
        return closed_;
    }

    void do_close_channel();

    void reset_buffer_size();
};

}
