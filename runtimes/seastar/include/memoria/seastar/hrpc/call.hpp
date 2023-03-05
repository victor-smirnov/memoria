
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

#include <seastar/core/condition-variable.hh>
#include <seastar/core/thread.hh>

namespace memoria::hrpc::ss {

class SeastarHRPCCall final: public st::HRPCCallImpl {

    using Base = st::HRPCCallImpl;

    seastar::condition_variable waiter_;

public:
    SeastarHRPCCall(
            const st::SessionImplPtr& session,
            CallID call_id,
            Request request,
            st::CallCompletionFn completion_fn
    ): Base(session, call_id, std::move(request), completion_fn)
    {}


    void wait_for_response() override {
        waiter_.wait([&](){
            return response_.is_not_null();
        }).get();
    }

    void notify_response_ready() override {
        waiter_.broadcast();
    }

    void run_async(std::function<void()> fn) override {
        (void)seastar::async(fn);
    }

    st::InputChannelImplPtr make_input_channel(ChannelCode code) override;
    st::OutputChannelImplPtr make_output_channel(ChannelCode code) override;
};

}
