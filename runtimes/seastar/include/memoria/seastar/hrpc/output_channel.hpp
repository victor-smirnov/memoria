
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

#include <memoria/hrpc/hrpc_impl_output_channel.hpp>
#include <seastar/core/condition-variable.hh>

namespace memoria::hrpc::ss {

class SeastarHRPCOutputChannel final: public st::HRPCOutputChannelImpl {

    using Base = st::HRPCOutputChannelImpl;

    seastar::condition_variable flow_control_;

public:
    SeastarHRPCOutputChannel(
        const st::SessionImplPtr& session,
        CallID call_id, ChannelCode code,
        uint64_t batch_size_limit, bool call_side
    ):
        Base(session, call_id, code, batch_size_limit, call_side)
    {}

    void wait_for_lease() {
        flow_control_.wait([&](){
            return batch_size_ >= batch_size_limit_;
        }).get();
    }

    void notify_lease_ready() {
        flow_control_.broadcast();
    }
};

}
