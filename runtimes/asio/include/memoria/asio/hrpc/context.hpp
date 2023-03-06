
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

#include <memoria/hrpc/hrpc_impl_context.hpp>

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/fiber.hpp>

namespace memoria::asio::hrpc {

using namespace memoria::hrpc;

class AsioHRPCContext final: public st::HRPCContextImpl {

    using Base = st::HRPCContextImpl;

    boost::fibers::mutex mutex_;
    boost::fibers::condition_variable waiter_;

public:
    AsioHRPCContext(
            st::SessionImplPtr session,
            CallID call_id,
            const EndpointID& endpoint_id,
            Request request
    ):
        Base(session, call_id, endpoint_id, request)
    {}

    void run_async(std::function<void()> fn) override {
        fn();
    }

    st::InputChannelImplPtr make_input_channel(ChannelCode code) override;
    st::OutputChannelImplPtr make_output_channel(ChannelCode code) override;
};

}
