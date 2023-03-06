
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

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/fiber.hpp>


namespace memoria::asio::hrpc {

using namespace memoria::hrpc;

class AsioHRPCOutputChannel final: public st::HRPCOutputChannelImpl {

    using Base = st::HRPCOutputChannelImpl;

    boost::fibers::mutex mutex_;
    boost::fibers::condition_variable flow_control_;

public:
    AsioHRPCOutputChannel(
        const st::SessionImplPtr& session,
        CallID call_id, ChannelCode code,
        uint64_t batch_size_limit, bool call_side
    ):
        Base(session, call_id, code, batch_size_limit, call_side)
    {}

    void wait_for_lease() {
        std::unique_lock<boost::fibers::mutex> lock(mutex_);
        flow_control_.wait(lock, [&](){
            return batch_size_ >= batch_size_limit_;
        });
    }

    void notify_lease_ready() {
        flow_control_.notify_all();
    }
};

}
