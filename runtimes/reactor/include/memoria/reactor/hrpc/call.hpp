
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

#include <memoria/hrpc/hrpc_impl_call.hpp>

#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/fiber.hpp>

namespace memoria::reactor::hrpc {

using namespace memoria::hrpc;

class ReactorHRPCCall final: public st::HRPCCallImpl {

    using Base = st::HRPCCallImpl;

    boost::fibers::mutex mutex_;
    boost::fibers::condition_variable waiter_;

public:
    ReactorHRPCCall(
            const st::SessionImplPtr& session,
            CallID call_id,
            Request request,
            st::CallCompletionFn completion_fn
    ): Base(session, call_id, std::move(request), completion_fn)
    {}


    void wait_for_response() override {
        std::unique_lock<boost::fibers::mutex> lock(mutex_);
        waiter_.wait(lock, [&]{
            return response_.is_not_null();
        });
    }

    void notify_response_ready() override {
        waiter_.notify_all();
    }

    void run_async(std::function<void()> fn) override {
        boost::fibers::fiber(fn).detach();
    }

    st::InputChannelImplPtr make_input_channel(ChannelCode code) override;
    st::OutputChannelImplPtr make_output_channel(ChannelCode code) override;
};

}
